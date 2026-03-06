add_executable(unit-tests
)

include(GoogleTest)

gtest_discover_tests(unit-tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
