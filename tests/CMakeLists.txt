add_executable(unit-tests
    graph-constant-tests.cpp
    property-graph-tests.cpp
    property-graph-exhaustive-tests.cpp
    graph-representation-tests.cpp
    traversal-tests.cpp
)

target_link_libraries(unit-tests PRIVATE
    GTest::gtest_main
    libproject::graphs
)

include(GoogleTest)

gtest_discover_tests(unit-tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
