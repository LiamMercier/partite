cmake_minimum_required(VERSION 3.25)

project(libproject
        LANGUAGES CXX
        VERSION 1.0.0)

# setup main library target
add_library(libproject INTERFACE)

target_include_directories(libproject
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_subdirectory(include/libproject/graphs)

if(LIBPROJECT_BUILD_TESTS)
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
