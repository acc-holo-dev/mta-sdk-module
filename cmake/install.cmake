include_guard(GLOBAL)

# Install layout: the module binary plus docs and license, ready to drop
# into mods/deathmatch/modules/ of an MTA server.
install(TARGETS sdk_base
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION .
)

# CPack: produce a ZIP with the module binary, README and LICENSE.
# The package lands in build/<preset>/package/ regardless of where cpack runs.
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/package")
set(CPACK_PACKAGE_NAME "ml_base")
set(CPACK_PACKAGE_VENDOR "HoloDev")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MTA:SA Lua module SDK base (ml_base)")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_PACKAGE_FILE_NAME "ml_base-${PROJECT_VERSION}-${SDK_PLATFORM_TAG}-${SDK_ARCH_TAG}")
include(CPack)