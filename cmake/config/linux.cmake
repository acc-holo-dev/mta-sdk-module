set_target_properties(sdk_base PROPERTIES
    SKIP_BUILD_RPATH YES
    BUILD_WITH_INSTALL_RPATH NO
)

target_link_options(sdk_base PRIVATE
    -Wl,--no-undefined
)
