add_executable(loom_graph_tests
        compatibility_test.cpp
        following_test.cpp
        paths_test.cpp
        validate_test.cpp
)

# Links only the layer under test
target_link_libraries(loom_graph_tests PRIVATE loom_graph Catch2::Catch2WithMain)

catch_discover_tests(loom_graph_tests)
