add_library(partite_graphs INTERFACE)
add_library(partite::graphs ALIAS partite_graphs)

target_include_directories(partite_graphs
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(partite_graphs INTERFACE partite)
