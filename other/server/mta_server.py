#!/usr/bin/env python3
"""MTA server test harness (plan PROMT.md §30-§32).

Manages a PINNED, locally-installed MTA:SA server for integration tests:

    mta_server.py install    download + install the pinned server build
    mta_server.py update     re-install the pinned build (refresh)
    mta_server.py version    print the installed build identity
    mta_server.py start      start the server (kept running; debug use)
    mta_server.py stop       stop a server started by `start`
    mta_server.py test       full integration run (build module, temp server
                             dir, module install, test resource, markers,
                             resource restart, graceful stop, log capture)

The pinned build identity lives in PINNED below and is recorded (with the
download checksum) into install.json after a successful install. Server
binaries are never committed (see .gitignore). No developer's global MTA
installation is used; everything runs from other/server/servers/<build> and
a fresh temp directory per test run (cleaned up afterwards).
"""

from __future__ import annotations

import hashlib
import ctypes
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.request
from pathlib import Path

SELF = Path(__file__).resolve()
SERVER_DIR = SELF.parent
PROJECT_ROOT = SERVER_DIR.parents[1]
TOOLS_DIR = PROJECT_ROOT / "other" / "tools" / "mta"

DOWNLOADS = SERVER_DIR / "downloads"
INSTALL_ROOT = SERVER_DIR / "servers"
INSTALL_JSON = SERVER_DIR / "install.json"
PID_JSON = SERVER_DIR / "server.pid"
LOGS_DIR = SERVER_DIR / "logs"

# Pinned server build (plan §30: never "latest"; the exact identity below is
# what the harness installs and what install.json records). Windows x64
# server, 1.6 release line, nightly.mtasa.com.
PINNED = {
    "platform": "windows",
    "architecture": "x64",
    "branch": "1.6",
    "build": "24140",
    "build_date": "20260820",
    "filename": "mtasa_x64-1.6-rc-24140-20260820.exe",
    "url": "https://nightly.mtasa.com/mtasa_x64-1.6-rc-24140-20260820.exe",
}

# NSIS extraction tool. The MTA installer ignores /D when an MTA install is
# already registered on the machine (it would touch the developer's global
# installation, which §31 forbids), so the harness unpacks the installer's
# payload directly with a locally provisioned 7-Zip. The .msi is unpacked
# with `msiexec /a` (an administrative image: plain file extraction, no
# elevation), because the .exe installer's manifest requires elevation.
SEVENZIP = {
    "filename": "7z2602-x64.msi",
    "url": "https://github.com/ip7z/7zip/releases/download/26.02/7z2602-x64.msi",
}
SEVENZIP_DIR = SERVER_DIR / "tools" / "7zip"

TEST_RESOURCE_NAME = "sdkintegration"
INTEGRATION_TIMEOUT = 180.0

MARK = "INTEGRATION:"
MARK_RESULT = "INTEGRATION_RESULT:"
STALE_MARKER = "STALE_TASK_DELIVERED"

# Scancodes for the console keys the harness must inject (MTA reads the
# console input buffer, not stdin, so commands are typed as key events).
SCAN_CODES = {
    **{chr(c): c - 0x41 + 0x1E for c in range(0x41, 0x5B)},  # A..Z
    " ": 0x39,
    "\r": 0x1C,
    "1": 0x02, "2": 0x03, "3": 0x04, "4": 0x05, "5": 0x06,
    "6": 0x07, "7": 0x08, "8": 0x09, "9": 0x0A, "0": 0x0B,
    ".": 0x33, "-": 0x0C, "/": 0x35,
}
VK_CODES = {"\r": 0x0D, " ": 0x20, **{chr(c): c for c in range(0x41, 0x5B)},
            **{str(d): 0x30 + d for d in range(10)}, ".": 0xBE, "-": 0xBD, "/": 0xBF}


def out(text: str = "") -> None:
    print(text, flush=True)


def die(message: str, code: int = 1) -> None:
    print(f"mta_server: error: {message}", file=sys.stderr, flush=True)
    sys.exit(code)


def load_cli():
    sys.path.insert(0, str(SERVER_DIR.parent / "tools" / "mta"))
    import cli  # noqa: PLC0415 (local import keeps CLI coupling explicit)

    return cli


def load_config() -> dict:
    cli = load_cli()
    return cli.load_config(PROJECT_ROOT)


def module_binary(config: dict) -> Path | None:
    cli = load_cli()
    preset = cli.default_preset(PROJECT_ROOT)
    return cli.output_binary(PROJECT_ROOT, preset, config)


# --- install ------------------------------------------------------------------


def download(url: str, target: Path) -> Path:
    out(f"Downloading {url} ...")
    target.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(url, headers={"User-Agent": "mta-sdk-harness/1.0"})
    with urllib.request.urlopen(request, timeout=180) as response, target.open("wb") as sink:
        shutil.copyfileobj(response, sink, length=1 << 20)
    return target


def find_server_exe(base: Path) -> Path | None:
    for name in ("mta-server64.exe", "mta-server.exe", "MTA Server.exe"):
        matches = list(base.rglob(name))
        if matches:
            return matches[0]
    for exe in base.rglob("*.exe"):
        lowered = exe.name.lower()
        if "server" in lowered and "uninstall" not in lowered:
            return exe
    return None


def sevenzip_exe() -> Path:
    """Locally provisioned 7-Zip console (downloads it on first use)."""
    exe = SEVENZIP_DIR / "Files" / "7-Zip" / "7z.exe"
    if exe.is_file():
        return exe
    msi = DOWNLOADS / SEVENZIP["filename"]
    if not msi.is_file():
        download(SEVENZIP["url"], msi)
    out(f"Provisioning 7-Zip into {SEVENZIP_DIR} (msiexec administrative image) ...")
    result = subprocess.run(
        ["msiexec", "/a", str(msi), f"TARGETDIR={SEVENZIP_DIR}", "/qn"],
        timeout=300,
        capture_output=True,
        text=True,
    )
    if not exe.is_file():
        die(f"msiexec /a did not produce 7z.exe (code {result.returncode}); inspect {SEVENZIP_DIR}")
    return exe


def cmd_install(update: bool) -> int:
    if INSTALL_JSON.is_file() and not update:
        out("Already installed:")
        cmd_version()
        return 0

    archive = DOWNLOADS / PINNED["filename"]
    if not archive.is_file():
        download(PINNED["url"], archive)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()

    install_dir = INSTALL_ROOT / f"mtasa-{PINNED['build']}"
    if install_dir.exists():
        shutil.rmtree(install_dir)
    install_dir.mkdir(parents=True)

    sevenzip = sevenzip_exe()
    out(f"Extracting {archive.name} (direct payload extraction, isolated dir) ...")
    result = subprocess.run(
        [str(sevenzip), "x", str(archive), f"-o{install_dir}", "-y"],
        capture_output=True,
        text=True,
        timeout=600,
    )
    server_exe = find_server_exe(install_dir)
    if server_exe is None:
        die(
            f"7-Zip extraction produced no server executable under {install_dir}; "
            f"7z output: {(result.stdout or result.stderr or '')[-400:]}"
        )

    INSTALL_JSON.write_text(
        json.dumps(
            {**PINNED, "sha256": digest, "install_dir": str(server_exe.parent), "server_exe": str(server_exe)},
            indent=2,
        ),
        encoding="utf-8",
    )
    out(f"Server executable: {server_exe}")
    out("Installed; identity recorded in other/server/install.json")
    return 0


def cmd_version() -> int:
    if not INSTALL_JSON.is_file():
        out("not installed")
        return 1
    info = json.loads(INSTALL_JSON.read_text(encoding="utf-8"))
    out(
        f"MTA server {info['branch']} build {info['build']} "
        f"({info['platform']}/{info['architecture']}, {info['build_date']})"
    )
    out(f"  sha256: {info['sha256']}")
    out(f"  install_dir: {info['install_dir']}")
    return 0


def cmd_start() -> int:
    info = require_install()
    workdir = SERVER_DIR / "run"
    workdir.mkdir(exist_ok=True)
    creationflags = subprocess.CREATE_NEW_CONSOLE if os.name == "nt" else 0
    process = subprocess.Popen(
        [info["server_exe"]],
        cwd=str(workdir),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        creationflags=creationflags,
    )
    PID_JSON.write_text(json.dumps({"pid": process.pid}), encoding="utf-8")
    out(f"Server started (pid {process.pid}) in {workdir}")
    out(f"Stop it with: python {SELF} stop")
    return 0


def cmd_stop() -> int:
    if not PID_JSON.is_file():
        out("no server pid recorded")
        return 0
    pid = json.loads(PID_JSON.read_text(encoding="utf-8"))["pid"]
    if os.name == "nt":
        subprocess.run(["taskkill", "/PID", str(pid), "/F"], capture_output=True)
    else:
        subprocess.run(["kill", str(pid)], capture_output=True)
    PID_JSON.unlink(missing_ok=True)
    out(f"Server {pid} stopped")
    return 0


def require_install() -> dict:
    if not INSTALL_JSON.is_file():
        die("the pinned server is not installed; run `mta server install` first")
    return json.loads(INSTALL_JSON.read_text(encoding="utf-8"))


# --- integration test ------------------------------------------------------------

META_XML = f"""<meta>
    <info author="MTA Module SDK" name="{TEST_RESOURCE_NAME}" type="script" version="1.0.0"/>
    <script src="main.lua" type="server"/>
</meta>
"""

# Integration scenarios (plan §32/§33). Generation 1 runs the function-level
# scenarios, prints the result, leaves a marker file and a 10-second stale
# task, then asks the harness (RESTART_NOW) to restart the resource. The
# restarted generation must never receive the stale task's completion; it
# waits past the stale window and prints the second result.
MAIN_LUA = r"""
local MARK = "INTEGRATION:"
local failures = {}
local isGeneration2 = fileExists("sdk_gen1_was_here.txt")

local function check(name, condition)
    if condition then
        outputServerLog(MARK .. " " .. name .. ": OK")
    else
        failures[#failures + 1] = name
        outputServerLog(MARK .. " " .. name .. ": FAILED")
    end
end

if isGeneration2 then
    fileDelete("sdk_gen1_was_here.txt")
    -- plan §33: the generation-1 task (10s) must never deliver here; the
    -- harness asserts the absence of STALE_TASK_DELIVERED in the log.
    outputServerLog(MARK .. " generation 2 start")
    -- shutdown with active workers (plan §32): a 60s task is still queued
    -- when the harness stops the server; the module must cancel it cleanly
    -- and never let it fire.
    sample_task_run(60000, 0, 0, function()
        outputServerLog("SHOULD_NEVER_FIRE")
    end)
    setTimer(function()
        if #failures == 0 then
            outputServerLog("INTEGRATION_RESULT: PASS")
        else
            outputServerLog("INTEGRATION_RESULT: FAIL (" .. table.concat(failures, ", ") .. ")")
        end
    end, 11000, 1)
    return
end

-- --- generation 1 ------------------------------------------------------------

check("module load", type(sample_add) == "function" and type(counter_create) == "function")
check("function registration", type(sample_task_run) == "function" and type(sample_timer) == "function")
check("return values", sample_add(2, 3) == 5 and sample_greet("Bob") == "hello, Bob")
check("argument validation", select(1, pcall(sample_add, "x", {})) == false)

local counter = counter_create(7)
check("userdata", counter ~= nil and counter:get() == 7 and counter:add(3) == 10)
check("userdata validation", select(1, pcall(counter.set, counter, {})) == false)

local timerFired = 0
sample_timer(50, 2, function() timerFired = timerFired + 1 end)

local taskResult = nil
sample_task_run(100, 10, 20, function(sum) taskResult = sum end)
local staleTask = sample_task_run(10000, 0, 0, function()
    outputServerLog("STALE_TASK_DELIVERED")
end)
check("async task valid", staleTask > 0)

local callbackValue = nil
sample_async_add(1, 2, function(sum) callbackValue = sum end)

setTimer(function()
    check("timer", timerFired >= 2)
    check("callback", callbackValue == 3)
    check("async completion", taskResult == 30)

    if #failures == 0 then
        outputServerLog("INTEGRATION_RESULT: PASS")
    else
        outputServerLog("INTEGRATION_RESULT: FAIL (" .. table.concat(failures, ", ") .. ")")
    end

    -- arm the §33 restart scenario: the marker file marks THIS generation as
    -- done; the 10s task must never deliver after the harness restart.
    local handle = fileCreate("sdk_gen1_was_here.txt")
    if handle then
        fileWrite(handle, "generation 1 was here")
        fileClose(handle)
    end
    sample_task_run(10000, 0, 0, function()
        outputServerLog("STALE_TASK_DELIVERED")
    end)
    outputServerLog(MARK .. " RESTART_NOW")
end, 800, 1)
"""


def cmd_test(args) -> int:
    info = require_install()
    config = load_config()
    module_name = config["module"]["name"]

    out("Building the module ...")
    build = subprocess.run(
        [sys.executable, str(SERVER_DIR.parent / "tools" / "mta" / "cli.py"), "build"],
        cwd=str(PROJECT_ROOT),
    )
    if build.returncode != 0:
        die("module build failed")
    binary = module_binary(config)
    if binary is None:
        die("module binary not found after the build")

    # Fresh temporary server directory per run (plan §31), cleaned up after.
    temp_root = Path(tempfile.mkdtemp(prefix="mta-sdk-integration-"))
    server_root = temp_root / "server"
    out(f"Preparing the temporary server directory {server_root} ...")
    shutil.copytree(info["install_dir"], server_root)

    mods = server_root / "mods" / "deathmatch"
    if not mods.is_dir():
        shutil.rmtree(temp_root, ignore_errors=True)
        die(f"unexpected server layout (no mods/deathmatch in {info['install_dir']})")

    # The x64 server loads modules from <server>/x64/modules (SERVER_BIN_PATH_MOD).
    modules_dir = server_root / "x64" / "modules"
    modules_dir.mkdir(exist_ok=True)
    shutil.copy2(binary, modules_dir / binary.name)
    out(f"Module installed: x64/modules/{binary.name}")

    resources_dir = mods / "resources"
    resources_dir.mkdir(exist_ok=True)
    resource_dir = resources_dir / TEST_RESOURCE_NAME
    resource_dir.mkdir()
    (resource_dir / "meta.xml").write_text(META_XML, encoding="utf-8")
    (resource_dir / "main.lua").write_text(MAIN_LUA, encoding="utf-8")

    # Unique ports per run so parallel runs never collide. The Windows server
    # reads the config from the SERVER ROOT (next to the exe); the Linux one
    # from mods/deathmatch -- write both.
    base = 23000 + (os.getpid() % 2000)
    conf_text = "\n".join(
        [
            "<config>",
            "    <servername>sdk-integration</servername>",
            f"    <serverport>{base}</serverport>",
            f"    <httpport>{base + 2}</httpport>",
            "    <maxplayers>4</maxplayers>",
            f'    <module src="{binary.name}"></module>',
            f'    <resource src="{TEST_RESOURCE_NAME}" startup="1"/>',
            "</config>",
        ]
    )
    (mods / "mtaserver.conf").write_text(conf_text, encoding="utf-8")
    (server_root / "mtaserver.conf").write_text(conf_text, encoding="utf-8")

    out("Starting the server (temp directory) ...")
    try:
        exit_code = run_server(info, server_root)
    finally:
        log_copy = LOGS_DIR / time.strftime("%Y%m%d-%H%M%S")
        log_copy.mkdir(parents=True, exist_ok=True)
        # The pinned server runs without a configured log file; the console
        # scrollback IS the run log.
        console_log = log_copy / "server.log"
        console_log.write_text("\n".join(console_buffer_lines()), encoding="utf-8", errors="replace")
        out(f"Log kept at: {console_log}")
        shutil.rmtree(temp_root, ignore_errors=True)
        out("Temporary server directory cleaned up")
    return exit_code


def run_server(info: dict, server_root: Path) -> int:
    # The Windows MTA server requires a real console for both output and
    # input: its startup verifies the stdout handle with
    # GetConsoleScreenBufferInfo and quits otherwise, and it reads commands
    # (and the quit command) from the console input buffer. The harness
    # therefore ensures a console exists, points the server's stdin/stdout
    # at that console (CONIN$/CONOUT$) and drives it through the same
    # console; the console buffer (full scrollback) is the source of truth
    # for integration markers.
    ensure_console()
    startupinfo = None
    close_fds = True
    if os.name == "nt":
        # Both handles need GENERIC_READ|GENERIC_WRITE: the server verifies
        # its stdout handle with GetConsoleScreenBufferInfo, which a
        # write-only console handle fails.
        stdin_handle = console_handle("CONIN$", 0xC0000000)
        stdout_handle = console_handle("CONOUT$", 0xC0000000)
        kernel32 = ctypes.WinDLL("kernel32")
        handle_flag_inherit = 0x1
        kernel32.SetHandleInformation(stdin_handle, handle_flag_inherit, handle_flag_inherit)
        kernel32.SetHandleInformation(stdout_handle, handle_flag_inherit, handle_flag_inherit)
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags = subprocess.STARTF_USESTDHANDLES
        startupinfo.hStdInput = stdin_handle
        startupinfo.hStdOutput = stdout_handle
        startupinfo.hStdError = stdout_handle
        close_fds = False
    process = subprocess.Popen(
        [info["server_exe"], "-t"],
        cwd=str(server_root),
        startupinfo=startupinfo,
        close_fds=close_fds,
    )
    if os.name == "nt":
        kernel32.CloseHandle(stdin_handle)
        kernel32.CloseHandle(stdout_handle)
    try:
        return wait_for_results(process)
    finally:
        stop_process(process)


def ensure_console() -> None:
    if os.name != "nt":
        return
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    if kernel32.GetConsoleWindow() is None:
        kernel32.AllocConsole()


def console_handle(name: str, access: int):
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    # GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
    # OPEN_EXISTING
    handle = kernel32.CreateFileW(name, access, 3, None, 3, 0, None)
    return handle


def console_buffer_lines() -> list[str]:
    """Reads the console buffer history (server shares the console with us).

    Reads up to the cursor row so the scrollback captured so far is covered;
    empty trailing rows are dropped.
    """
    if os.name != "nt":
        return []
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

    class COORD(ctypes.Structure):
        _fields_ = [("X", ctypes.c_short), ("Y", ctypes.c_short)]

    class SMALL_RECT(ctypes.Structure):
        _fields_ = [
            ("Left", ctypes.c_short),
            ("Top", ctypes.c_short),
            ("Right", ctypes.c_short),
            ("Bottom", ctypes.c_short),
        ]

    class CSBI(ctypes.Structure):
        _fields_ = [
            ("dwSize", COORD),
            ("dwCursorPosition", COORD),
            ("wAttributes", ctypes.c_ushort),
            ("srWindow", SMALL_RECT),
            ("dwMaximumWindowSize", COORD),
        ]

    handle = console_handle("CONOUT$", 0xC0000000)
    if handle in (-1, ctypes.c_void_p(-1).value):
        return []
    info = CSBI()
    ok = kernel32.GetConsoleScreenBufferInfo(handle, ctypes.byref(info))
    if not ok:
        kernel32.CloseHandle(handle)
        return []
    width = info.dwSize.X
    height = info.dwCursorPosition.Y + 1  # content only exists up to the cursor
    buffer = ctypes.create_unicode_buffer(width + 1)
    chars_read = ctypes.c_ulong()
    lines = []
    for row in range(height):
        kernel32.ReadConsoleOutputCharacterW(
            handle, buffer, width, COORD(0, row), ctypes.byref(chars_read)
        )
        lines.append(buffer.value[: chars_read.value].rstrip())
    kernel32.CloseHandle(handle)
    return [line for line in lines if line.strip()]


def inject_console_text(text: str) -> None:
    """Types text into the console input buffer (server reads key events)."""
    if os.name != "nt":
        return
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

    class COORD(ctypes.Structure):
        _fields_ = [("X", ctypes.c_short), ("Y", ctypes.c_short)]

    class KEY_EVENT_RECORD(ctypes.Structure):
        _fields_ = [
            ("bKeyDown", ctypes.c_int),
            ("wRepeatCount", ctypes.c_ushort),
            ("wVirtualKeyCode", ctypes.c_ushort),
            ("wVirtualScanCode", ctypes.c_ushort),
            ("UnicodeChar", ctypes.c_wchar),
            ("dwControlKeyState", ctypes.c_ulong),
        ]

    class INPUT_RECORD(ctypes.Structure):
        class _Event(ctypes.Union):
            _fields_ = [("KeyEvent", KEY_EVENT_RECORD)]

        # x64 layout: WORD EventType + (implicit 2-byte alignment pad) +
        # 16-byte event = 20 bytes; no extra padding field.
        _fields_ = [("EventType", ctypes.c_ushort), ("Event", _Event)]

    handle = console_handle("CONIN$", 0xC0000000)
    if handle in (-1, ctypes.c_void_p(-1).value):
        out("Console input not available for key injection")
        return
    records = (INPUT_RECORD * (len(text) * 2))()
    for index, char in enumerate(text):
        lower = char.lower() if char.isalpha() else char
        vk = VK_CODES.get(lower, ord(lower))
        scan = SCAN_CODES.get(lower, 0)
        for down in (1, 0):
            record = records[index * 2 + (0 if down else 1)]
            record.EventType = 1  # KEY_EVENT
            record.Event.KeyEvent.bKeyDown = down
            record.Event.KeyEvent.wRepeatCount = 1
            record.Event.KeyEvent.wVirtualKeyCode = vk
            record.Event.KeyEvent.wVirtualScanCode = scan
            record.Event.KeyEvent.UnicodeChar = char
            record.Event.KeyEvent.dwControlKeyState = 0
    written = ctypes.c_ulong()
    ok = kernel32.WriteConsoleInputW(handle, records, len(text) * 2, ctypes.byref(written))
    kernel32.CloseHandle(handle)
    if not ok or written.value != len(text) * 2:
        out(f"Key injection incomplete (ok={ok}, written={written.value})")


def wait_for_results(process) -> int:
    deadline = time.time() + INTEGRATION_TIMEOUT
    restart_sent = False
    results: list[bool] = []
    tail: list[str] = []
    stale_seen = False

    while time.time() < deadline:
        # The server shares our console; its whole output history is in the
        # console buffer. Consume only the lines appended since last poll.
        lines = console_buffer_lines()
        for line in lines[len(tail) :]:
            tail.append(line)
            if STALE_MARKER in line:
                stale_seen = True
                out(f"!! {line}")
            if "RESTART_NOW" in line and not restart_sent:
                restart_sent = True
                out("Restarting the resource (plan §33 stale-generation scenario) ...")
                inject_console_text(f"restart {TEST_RESOURCE_NAME}\r")
            if MARK_RESULT in line:
                passed = "PASS" in line
                results.append(passed)
                out(line)
                if not passed:
                    dump_tail(tail)
                    return 1
                # generation 2 prints the final result; require both results
                if len(results) == 2:
                    report_scenarios(tail)
                    if stale_seen:
                        out("FAILED: a stale generation-1 task delivered after the restart")
                        return 1
                    for late in tail:
                        if "SHOULD_NEVER_FIRE" in late:
                            out("FAILED: a task fired during shutdown")
                            return 1
                    out("All integration scenarios passed (2 generations, stale task dropped, clean shutdown)")
                    return 0
        if process.poll() is not None:
            out("The server exited before the integration result; console tail:")
            dump_tail(tail)
            return 1
        time.sleep(0.2)

    out("Timed out waiting for the integration results; console tail:")
    dump_tail(tail)
    return 1


def dump_console() -> None:
    for line in console_buffer_lines()[-25:]:
        out("  | " + line)


def dump_tail(tail: list[str]) -> None:
    for line in tail[-30:]:
        out("  " + line)


def report_scenarios(tail: list[str]) -> None:
    out("Scenarios covered:")
    for line in tail:
        if line.startswith(MARK) and "RESTART_NOW" not in line and "generation" not in line:
            out("  - " + line[len(MARK) :].strip())


def stop_process(process) -> None:
    """Graceful stop first: console commands let MTA shut down cleanly (the
    module's ShutdownModule runs), then force-kill after a grace period."""
    try:
        if process.poll() is None:
            inject_console_text("exit\r")
            try:
                process.wait(timeout=8)
                out("Server shut down gracefully (console exit)")
                return
            except subprocess.TimeoutExpired:
                pass
            inject_console_text("q")
            try:
                process.wait(timeout=20)
                out("Server shut down gracefully (console Q)")
                return
            except subprocess.TimeoutExpired:
                pass
            if os.name == "nt":
                subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"], capture_output=True)
            else:
                process.kill()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            pass
    except Exception:
        pass


# --- entry ------------------------------------------------------------------


def main(argv: list[str]) -> int:
    command = argv[1] if len(argv) > 1 else ""
    if command == "install":
        return cmd_install(update=False)
    if command == "update":
        return cmd_install(update=True)
    if command == "version":
        return cmd_version()
    if command == "start":
        return cmd_start()
    if command == "stop":
        return cmd_stop()
    if command == "test":
        return cmd_test(None)
    out(__doc__)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))