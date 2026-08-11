add_executable(loom_nodes_tests
        builtin_test.cpp
        node_behaviour_test.cpp
)

# Links only the layer under test
target_link_libraries(loom_nodes_tests PRIVATE loom_nodes Catch2::Catch2WithMain)

catch_discover_tests(loom_nodes_tests)
