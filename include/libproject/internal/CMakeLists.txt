add_library(libproject_internal INTERFACE)
add_library(libproject::internal ALIAS libproject_internal)

target_include_directories(libproject_internal
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

# Might not be necessary, but perhaps it will be in the future.
target_link_libraries(libproject_internal INTERFACE libproject)
