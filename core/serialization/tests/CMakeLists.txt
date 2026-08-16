add_executable(loom_serialization_tests
        round_trip_test.cpp
        variables_test.cpp
)

# Links only the layer under test
target_link_libraries(loom_serialization_tests PRIVATE loom_serialization Catch2::Catch2WithMain)

catch_discover_tests(loom_serialization_tests)
