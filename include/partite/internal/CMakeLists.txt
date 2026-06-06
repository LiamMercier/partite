add_library(partite_internal INTERFACE)
add_library(partite::internal ALIAS partite_internal)

target_include_directories(partite_internal
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

# Might not be necessary, but perhaps it will be in the future.
target_link_libraries(partite_internal INTERFACE partite)
