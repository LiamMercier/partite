add_library(libproject_graphs INTERFACE)
add_library(libproject::graphs ALIAS libproject_graphs)

target_include_directories(libproject_graphs
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(libproject_graphs INTERFACE libproject)
