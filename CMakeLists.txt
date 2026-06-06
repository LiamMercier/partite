cmake_minimum_required(VERSION 3.25)

project(partite
        LANGUAGES CXX
        VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# setup main library target
add_library(partite INTERFACE)

set(GRAPH_VERTEX_ID_TYPE uint32_t CACHE STRING "Vertex ID type at compile time")
set(GRAPH_EDGE_ID_TYPE uint32_t CACHE STRING "Edge ID type at compile time")

target_compile_definitions(partite INTERFACE
    GRAPH_VERTEX_ID_TYPE=${GRAPH_VERTEX_ID_TYPE}
    GRAPH_EDGE_ID_TYPE=${GRAPH_EDGE_ID_TYPE}
)

target_include_directories(partite
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_subdirectory(include/partite/graphs)

if(GRAPH_BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()
