add_executable(unit-tests
)

target_link_libraries(unit-tests PRIVATE
    GTest::gtest_main
    libproject::graphs
)

include(GoogleTest)

gtest_discover_tests(unit-tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
