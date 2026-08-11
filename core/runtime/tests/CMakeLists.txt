add_executable(loom_runtime_tests
        interpreter_test.cpp
        save_test.cpp
)

# Links only the layer under test
target_link_libraries(loom_runtime_tests PRIVATE loom_runtime Catch2::Catch2WithMain)

# TestHost and the stub node types live beside the tests, not in the library
target_include_directories(loom_runtime_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

catch_discover_tests(loom_runtime_tests)
