add_executable(unit-tests
    graph-constant-tests.cpp
    property-graph-tests.cpp
    graph-representation-tests.cpp
    traversal-tests.cpp
)

if (GRAPH_EXTENSIVE_TESTS)
    target_sources(unit-tests PRIVATE
        property-graph-exhaustive-tests.cpp
    )
endif()

target_link_libraries(unit-tests PRIVATE
    GTest::gtest_main
    libproject::graphs
)

include(GoogleTest)

gtest_discover_tests(unit-tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
