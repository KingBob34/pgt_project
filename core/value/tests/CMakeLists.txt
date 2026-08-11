add_executable(loom_value_tests
        inspect_test.cpp
        path_test.cpp
        parse_test.cpp
)

# Links only the layer under test, so a test cannot reach a layer above it either
target_link_libraries(loom_value_tests PRIVATE loom_value Catch2::Catch2WithMain)

catch_discover_tests(loom_value_tests)
