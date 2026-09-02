#!/usr/bin/env python3
"""mta -- the MTA Module SDK project CLI (plan PROMT.md §24-§30).

Commands:
    mta init <name>            scaffold a new module project
    mta new function <name>    minimal compile-ready function (name verbatim)
    mta new object <name>      skeleton native object (stable MTA_OBJECT id)
    mta build [--preset P]     configure + build via CMake presets
    mta test [unit|lua|integration]
    mta docs [--output FILE]   registry documentation from signature metadata
    mta doctor                 real environment checks
    mta package                copy the built module into dist/
    mta server <install|update|version|start|stop>  (delegates to other/server)

Only the Python standard library is used (tomllib on 3.11+).
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import platform as py_platform
from pathlib import Path

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python < 3.11
    tomllib = None

SELF = Path(__file__).resolve()
SDK_ROOT = SELF.parents[3]  # other/tools/mta/cli.py -> the SDK checkout root

OK, WARN, FAIL = "OK", "WARN", "FAIL"


def out(text: str = "") -> None:
    print(text)


def die(message: str, code: int = 1) -> None:
    print(f"mta: error: {message}", file=sys.stderr)
    sys.exit(code)


# --- project configuration (config/module.toml) -------------------------------


def load_config(root: Path) -> dict:
    path = root / "config" / "module.toml"
    if not path.is_file():
        die(f"no module project here ({path} not found); run inside a module project or use `mta init`")
    if tomllib is None:
        die("Python 3.11+ (tomllib) is required to read config/module.toml")
    try:
        with path.open("rb") as handle:
            return tomllib.load(handle)
    except Exception as exc:  # noqa: BLE001 - report parse errors verbatim
        die(f"config/module.toml is not valid TOML: {exc}")


def require_module_table(config: dict) -> dict:
    module = config.get("module")
    if not isinstance(module, dict) or not module.get("name"):
        die("config/module.toml has no [module] name")
    return module


# --- subprocess helpers -------------------------------------------------------


def toolchain_dirs(root: Path) -> list[Path]:
    """Local, git-ignored toolchains (plan: build/toolchain)."""
    base = root / "build" / "toolchain"
    if not base.is_dir():
        return []
    dirs = [d for d in base.iterdir() if d.is_dir()]
    bins = []
    for d in dirs:
        # e.g. build/toolchain/mingw/mingw64/bin
        for candidate in sorted(d.glob("*/*/bin")) + sorted(d.glob("*/bin")):
            if candidate.is_dir():
                bins.append(candidate)
    return bins


def make_env(root: Path) -> dict:
    env = os.environ.copy()
    prepends = [str(d) for d in toolchain_dirs(root)]
    if prepends:
        env["PATH"] = os.pathsep.join(prepends + [env.get("PATH", "")])
    return env


def which(name: str, root: Path) -> str | None:
    found = shutil.which(name)
    if found:
        return found
    for d in toolchain_dirs(root):
        for suffix in (".exe", ""):
            candidate = d / (name + suffix)
            if candidate.is_file():
                return str(candidate)
    return None


def run_tool(cmd: list[str], root: Path, cwd: Path | None = None, capture: bool = True) -> subprocess.CompletedProcess:
    env = make_env(root)
    # Resolve the executable against the CUSTOM env PATH: Windows CreateProcess
    # searches the caller's PATH, not the child environment's.
    first = cmd[0]
    if not Path(first).is_absolute() and os.sep not in first and "/" not in first:
        resolved = shutil.which(first, path=env.get("PATH", ""))
        if resolved is None:
            resolved = shutil.which(first + ".exe", path=env.get("PATH", ""))
        if resolved is not None:
            cmd = [resolved, *cmd[1:]]
    return subprocess.run(
        cmd,
        cwd=str(cwd or root),
        env=env,
        capture_output=capture,
        text=True,
        encoding="utf-8",
        errors="replace",
    )


def probe_version(name: str, args: list[str], root: Path) -> tuple[str | None, str]:
    """Returns (location, first line of output) or (None, error text)."""
    location = which(name, root)
    if location is None:
        return None, f"{name} not found in PATH or build/toolchain"
    result = run_tool([location, *args], root)
    if result.returncode != 0:
        return None, f"{name} exited with code {result.returncode}"
    line = (result.stdout or result.stderr or "").strip().splitlines()
    return location, (line[0] if line else "")


# --- default preset / platform ------------------------------------------------


def default_preset(root: Path) -> str:
    system = py_platform.system()
    if system == "Windows":
        return "win-mingw"
    if system == "Linux":
        return "linux-gcc"
    return "linux-gcc"


def preset_names(root: Path) -> list[str]:
    presets_path = root / "CMakePresets.json"
    if not presets_path.is_file():
        return []
    try:
        import json

        data = json.loads(presets_path.read_text(encoding="utf-8"))
        return [p["name"] for p in data.get("configurePresets", [])]
    except Exception:  # noqa: BLE001
        return []


def output_binary(root: Path, preset: str, config: dict) -> Path | None:
    """The built module binary, or None when it was not built (yet)."""
    name = require_module_table(config)["name"]
    system = py_platform.system()
    platform_tag = "win" if system == "Windows" else "linux"
    ext = ".dll" if system == "Windows" else ".so"
    arch = "x64"
    candidate = root / "build" / preset / "module" / f"{platform_tag}-{arch}" / (name + ext)
    return candidate if candidate.is_file() else None


# --- mta init -------------------------------------------------------------------

INIT_IGNORED_DIRS = {"build", ".git", "dist", "__pycache__", ".cache"}


def cmd_init(args: argparse.Namespace) -> int:
    name = args.name
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_.-]*", name):
        die(f"invalid module name: {name!r}")
    cwd = Path.cwd().resolve()

    if (cwd / "config" / "module.toml").is_file():
        die("refusing to run inside an existing module project (config/module.toml exists)")

    target = (cwd / name).resolve() if name else cwd
    if target.exists() and any(target.iterdir()):
        die(f"target directory is not empty: {target}")

    out(f"Creating module project {target} ...")
    # Copy the SDK checkout as the template. Guard against copying a directory
    # into its own subtree (the SDK checkout is the template AND may be the
    # CWD): exclude the target directory name at the top level.
    ignored = set(INIT_IGNORED_DIRS) | {name} if target.parent == cwd else set(INIT_IGNORED_DIRS)
    shutil.copytree(
        SDK_ROOT,
        target,
        dirs_exist_ok=True,
        ignore=shutil.ignore_patterns(
            *ignored,
            "*.pyc",
            "sdk_tests.exe",
            "sdk_docgen.exe",
            "*.dll",
            "*.so",
        ),
    )

    # Rewrite the module identity: the name is developer-owned (plan §5).
    toml_path = target / "config" / "module.toml"
    text = toml_path.read_text(encoding="utf-8")
    title = name.replace("-", " ").replace("_", " ").title()
    if not title.lower().endswith("module"):
        title = f"{title} Module"
    text = re.sub(r'(?m)^(name\s*=\s*")[^"]*("\s*)$', lambda m: f'name = "{name}"', text, count=1)
    text = re.sub(r'(?m)^(title\s*=\s*")[^"]*("\s*)$', lambda m: f'title = "{title}"', text, count=1)
    toml_path.write_text(text, encoding="utf-8")

    out("  config/module.toml    rewritten with the new identity")
    out("  source/sdk            SDK core copied")
    out("  source/functions      sample functions copied (edit or delete them)")
    out("")
    out("Next steps:")
    out(f"  cd {name}")
    out("  mta doctor")
    out("  mta build && mta test")
    return 0


# --- mta new function / new object ---------------------------------------------

FUNCTION_TEMPLATE = '''// {name}: generated by `mta new function` -- a minimal compile-ready
// function. The name below is registered EXACTLY as requested (plan s.26):
// the SDK never prefixes or renames developer functions.
#include <mta/sdk.hpp>

MTA_FUNCTION("{name}",
    [](std::string name)
    {{
        return "Hello, " + name;
    }});
'''

OBJECT_TEMPLATE = '''// {name}: generated by `mta new object` -- a native object skeleton with a
// stable, compiler-independent type identity (plan s.16/s.27).
#include <mta/sdk.hpp>

namespace
{{
struct {type}
{{
    double value = 0;
}};

// Stable type identity: the metatable name becomes "mta.<module>.{name}".
MTA_OBJECT("{name}", {type})

// The methods (self is the first parameter). Registry calls this once per VM.
void register_{ident}_methods(lua_State *L)
{{
    MTA_METHOD({type}, "get", []({type} &self) {{ return self.value; }});
    MTA_METHOD({type}, "add", []({type} &self, double v) {{
        self.value += v;
        return self.value;
    }});
}}

// Once per process: bind the method registrar to the type.
const bool {ident}_methods_registered = [] {{
    mta::userdata::Registry<{type}>::set_methods(&register_{ident}_methods);
    return true;
}}();
}} // namespace

MTA_LUA_FUNCTION("{name}_create", "Creates a {name} object.")
{{
    auto [value] = mta::lua::args<double>(L);
    mta::userdata::Registry<{type}>::create(L, {type}{{value}});
    return 1;
}}
'''


def cmd_new(args: argparse.Namespace) -> int:
    root = Path.cwd().resolve()
    load_config(root)  # new requires a module project
    kind = args.kind
    name = args.name
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_.-]*", name):
        die(f"invalid name: {name!r}")
    if "/" in name or "\\" in name or ".." in name:
        die("the function name must not contain path separators")

    if kind == "function":
        file_name = name.replace(".", "_")
        path = root / "source" / "functions" / f"{file_name}.cpp"
        template = FUNCTION_TEMPLATE.format(name=name)
    else:
        type_name = "".join(part.capitalize() for part in re.split(r"[._-]", name))
        file_name = name.replace(".", "_")
        path = root / "source" / "functions" / f"{file_name}.cpp"
        # C++ identifiers must be valid: dotted names become underscores there,
        # while the REGISTERED names (create function, MTA_OBJECT id) stay
        # verbatim (plan §26/§27).
        template = OBJECT_TEMPLATE.format(name=name, type=type_name, ident=file_name)

    if path.exists():
        die(f"refusing to overwrite an existing file: {path}")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(template, encoding="utf-8")
    out(f"Created {path}")
    out("The next build picks it up automatically (source discovery).")
    return 0


# --- mta build -------------------------------------------------------------------


def cmd_build(args: argparse.Namespace) -> int:
    root = Path.cwd().resolve()
    load_config(root)
    preset = args.preset or default_preset(root)
    out(f"Configuring ({preset}) ...")
    result = run_tool(["cmake", "--preset", preset], root, capture=False)
    if result.returncode != 0:
        die(f"cmake configure failed (preset {preset})", result.returncode or 1)
    out(f"Building ({preset}) ...")
    result = run_tool(["cmake", "--build", "--preset", preset], root, capture=False)
    if result.returncode != 0:
        die("cmake build failed", result.returncode or 1)

    config = load_config(root)
    binary = output_binary(root, preset, config)
    if binary is None:
        out("Build finished, but the module binary was not found at the expected path.")
        return 1
    out(f"Module ready: {binary}")
    return 0


# --- mta test ---------------------------------------------------------------------


def cmd_test(args: argparse.Namespace) -> int:
    root = Path.cwd().resolve()
    load_config(root)
    preset = args.preset or default_preset(root)
    what = args.suite

    if what == "integration":
        server_script = root / "other" / "server" / "mta_server.py"
        if not server_script.is_file():
            out("integration: NOT AVAILABLE -- the server harness is not installed")
            out("  (see other/server/; `mta server install` once it exists)")
            return 1
        return run_tool([sys.executable, str(server_script), "test"], root, capture=False).returncode

    if what == "unit":
        regex = "module_config"
    elif what == "lua":
        regex = "sdk_tests"
    else:
        regex = None

    cmd = ["ctest", "--preset", preset, "--output-on-failure"]
    if regex:
        cmd += ["-R", regex]
    result = run_tool(cmd, root, capture=False)
    return result.returncode


# --- mta docs ----------------------------------------------------------------------


def cmd_docs(args: argparse.Namespace) -> int:
    root = Path.cwd().resolve()
    load_config(root)
    preset = args.preset or default_preset(root)

    build = run_tool(["cmake", "--build", "--preset", preset, "--target", "sdk_docgen"], root)
    if build.returncode != 0:
        sys.stdout.write(build.stdout or "")
        sys.stderr.write(build.stderr or "")
        die("building sdk_docgen failed")

    system = py_platform.system()
    platform_tag = "win" if system == "Windows" else "linux"
    exe_ext = ".exe" if system == "Windows" else ""
    # CMAKE_RUNTIME_OUTPUT_DIRECTORY is build/<preset>/module/<tag>-<arch>.
    docgen = root / "build" / preset / "module" / f"{platform_tag}-x64" / f"sdk_docgen{exe_ext}"
    if not docgen.is_file():
        die(f"sdk_docgen binary not found at {docgen}")

    result = run_tool([str(docgen)], root)
    if result.returncode != 0:
        sys.stdout.write(result.stdout or "")
        die("sdk_docgen failed")
    if args.output:
        Path(args.output).write_text(result.stdout, encoding="utf-8")
        out(f"Documentation written to {args.output}")
    else:
        sys.stdout.write(result.stdout)
    return 0


# --- mta doctor ---------------------------------------------------------------------


def check(name: str, status: str, detail: str = "") -> tuple[str, str, str]:
    return name, status, detail


def cmd_doctor(args: argparse.Namespace) -> int:
    root = Path.cwd().resolve()
    findings: list[tuple[str, str, str]] = []

    # Project configuration -----------------------------------------------------
    config_path = root / "config" / "module.toml"
    if not config_path.is_file():
        findings.append(check("Project", FAIL, "config/module.toml not found"))
        config = None
    else:
        try:
            config = load_config(root)
            module_tbl = require_module_table(config)
            version = str(module_tbl.get("version", ""))
            version_ok = re.fullmatch(r"\d+\.\d+\.\d+", version) is not None
            findings.append(check(
                "Project",
                OK if version_ok else FAIL,
                f"name={module_tbl['name']} title={module_tbl.get('title', '')} version={version}"
                + ("" if version_ok else " (version is not X.Y.Z)"),
            ))
        except SystemExit:
            findings.append(check("Project", FAIL, "config/module.toml is invalid"))
            config = None

    # SDK headers + Lua ABI ------------------------------------------------------
    lua_src = SDK_ROOT / "other" / "third_party" / "lua" / "src"
    sdk_lua = SDK_ROOT / "other" / "third_party" / "mta-sdk" / "lua"
    headers_ok = lua_src.is_dir() and sdk_lua.is_dir()
    findings.append(check(
        "SDK headers",
        OK if headers_ok else FAIL,
        f"lua: {lua_src} mta-sdk: {SDK_ROOT / 'other/third_party/mta-sdk'}",
    ))

    mismatches = []
    compared = 0
    if headers_ok:
        for header in sorted(sdk_lua.glob("*.h")):
            twin = lua_src / header.name
            if not twin.is_file():
                mismatches.append(f"{header.name}: missing in lua/src")
                continue
            compared += 1
            if header.read_bytes() != twin.read_bytes():
                mismatches.append(f"{header.name}: differs from lua/src")
    findings.append(check(
        "Lua ABI",
        FAIL if mismatches else OK,
        f"{compared} header(s) byte-identical" if not mismatches else "; ".join(mismatches),
    ))

    # Build tools -----------------------------------------------------------------
    cmake_loc, cmake_ver = probe_version("cmake", ["--version"], root)
    ninja_loc, ninja_ver = probe_version("ninja", ["--version"], root)
    findings.append(check("CMake", OK if cmake_loc else FAIL, cmake_ver or "not found"))
    findings.append(check("Ninja", OK if ninja_loc else WARN, ninja_ver or "not found (MSBuild may be used)"))

    compiler_loc = which("g++", root) or which("cl", root)
    compiler_ver = ""
    if which("g++", root):
        _, compiler_ver = probe_version("g++", ["--version"], root)
    elif which("cl", root):
        _, compiler_ver = probe_version("cl", [], root)
    findings.append(check("Compiler", OK if compiler_loc else FAIL, compiler_ver or "not found"))

    cxx = (config or {}).get("build", {}).get("cxx_standard")
    findings.append(check("C++ standard", OK if cxx else WARN, f"C++{cxx}" if cxx else "not set in module.toml"))

    # Build system ------------------------------------------------------------------
    presets = preset_names(root)
    wanted = default_preset(root)
    findings.append(check(
        "Presets",
        OK if wanted in presets else WARN,
        f"{', '.join(presets) if presets else 'CMakePresets.json not found'}",
    ))

    sources = list((root / "source").rglob("*.cpp")) if (root / "source").is_dir() else []
    findings.append(check("Source discovery", OK if sources else FAIL, f"{len(sources)} source file(s)"))

    binary = output_binary(root, wanted, config) if config else None
    findings.append(check(
        "Build output",
        OK if binary else WARN,
        str(binary.relative_to(root)) if binary else f"{wanted} was not built yet (mta build)",
    ))

    # Git state ----------------------------------------------------------------------
    git = which("git", root)
    if git:
        branch = run_tool([git, "rev-parse", "--abbrev-ref", "HEAD"], root).stdout.strip()
        dirty = run_tool([git, "status", "--porcelain"], root).stdout.strip().splitlines()
        findings.append(check("Git", OK, f"branch {branch}, {len(dirty)} changed file(s)"))
    else:
        findings.append(check("Git", WARN, "git not found"))

    # Server test environment ------------------------------------------------------------
    install_json = root / "other" / "server" / "install.json"
    if install_json.is_file():
        try:
            import json

            info = json.loads(install_json.read_text(encoding="utf-8"))
            findings.append(check(
                "MTA server",
                OK,
                f"build {info.get('build', '?')} {info.get('platform', '?')}/{info.get('architecture', '?')}",
            ))
        except Exception:  # noqa: BLE001
            findings.append(check("MTA server", WARN, "install.json is not valid JSON"))
    else:
        findings.append(check("MTA server", WARN, "not installed (integration tests NOT RUN)"))

    # Report -----------------------------------------------------------------------------
    out("MTA Module SDK Doctor")
    out("-" * 60)
    for name, status, detail in findings:
        marker = {"OK": "[ok]  ", WARN: "[warn]", FAIL: "[FAIL]"}[status]
        out(f"{marker} {name:<18} {detail}")
    out("-" * 60)

    has_fail = any(status == FAIL for _, status, _ in findings)
    out("Status: " + ("NOT READY" if has_fail else "READY"))
    return 1 if has_fail else 0


# --- mta package ----------------------------------------------------------------------


def cmd_package(args: argparse.Namespace) -> int:
    root = Path.cwd().resolve()
    config = load_config(root)
    module_tbl = require_module_table(config)
    preset = args.preset or default_preset(root)

    binary = output_binary(root, preset, config)
    if binary is None:
        out("The module is not built yet; building first ...")
        if cmd_build(argparse.Namespace(preset=preset)) != 0:
            die("build failed during package")
        binary = output_binary(root, preset, config)
        if binary is None:
            die("module binary still missing after the build")

    version = str(module_tbl.get("version", "0.0.0"))
    system = py_platform.system()
    platform_tag = "win" if system == "Windows" else "linux"
    dist = root / "dist"
    dist.mkdir(exist_ok=True)
    target = dist / f"{module_tbl['name']}-{version}-{platform_tag}-x64{binary.suffix}"
    shutil.copy2(binary, target)

    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    out(f"Packaged: {target}")
    out(f"  sha256: {digest}")
    out(f"  source: {binary.relative_to(root)}")
    return 0


# --- mta server -------------------------------------------------------------------------


def cmd_server(args: argparse.Namespace) -> int:
    root = Path.cwd().resolve()
    server_script = root / "other" / "server" / "mta_server.py"
    if not server_script.is_file():
        die(
            "the server harness is not part of this checkout yet "
            "(other/server/mta_server.py not found); install lands in PHASE 11"
        )
    extra = args.server_args or []
    return run_tool([sys.executable, str(server_script), args.subcommand, *extra], root, capture=False).returncode


# --- argument parsing ----------------------------------------------------------------------


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="mta", description="MTA Module SDK project CLI")
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("init", help="create a new module project")
    p.add_argument("name", help="project directory name")
    p.set_defaults(func=cmd_init)

    p = sub.add_parser("new", help="generate a function or object skeleton")
    p.add_argument("kind", choices=["function", "object"])
    p.add_argument("name", help="registered name, used verbatim (e.g. crypto.sha256)")
    p.set_defaults(func=cmd_new)

    p = sub.add_parser("build", help="configure and build the module")
    p.add_argument("--preset", default=None, help="CMake configure preset (default: platform)")
    p.set_defaults(func=cmd_build)

    p = sub.add_parser("test", help="run unit/lua/integration tests")
    p.add_argument("suite", nargs="?", default="all", choices=["all", "unit", "lua", "integration"])
    p.add_argument("--preset", default=None)
    p.set_defaults(func=cmd_test)

    p = sub.add_parser("docs", help="generate function documentation from the registry")
    p.add_argument("--output", default=None, help="write markdown to this file instead of stdout")
    p.add_argument("--preset", default=None)
    p.set_defaults(func=cmd_docs)

    p = sub.add_parser("doctor", help="check the development environment")
    p.set_defaults(func=cmd_doctor)

    p = sub.add_parser("package", help="copy the built module into dist/")
    p.add_argument("--preset", default=None)
    p.set_defaults(func=cmd_package)

    p = sub.add_parser("server", help="manage the MTA server test environment")
    p.add_argument("subcommand", choices=["install", "update", "version", "start", "stop"])
    p.add_argument("server_args", nargs="*")
    p.set_defaults(func=cmd_server)

    return parser


def main(argv: list[str] | None = None) -> int:
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8", errors="replace")
        except Exception:  # noqa: BLE001
            pass
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())