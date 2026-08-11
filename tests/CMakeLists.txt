# The one test target that crosses layers on purpose: a story file goes in,
# a played-through story comes out.
add_executable(loom_story_tests
        story_test.cpp
)

target_link_libraries(loom_story_tests PRIVATE
        loom_serialization
        loom_runtime
        loom_nodes
        Catch2::Catch2WithMain
)

catch_discover_tests(loom_story_tests)
