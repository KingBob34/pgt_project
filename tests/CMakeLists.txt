# The one test target that crosses layers on purpose: a story file goes in,
# a played-through story comes out.
add_executable(loom_story_tests
        story_test.cpp
)

# One test plays the story that ships in stories/, so it needs to find it.
target_compile_definitions(loom_story_tests PRIVATE
        LOOM_STORIES_DIR="${CMAKE_SOURCE_DIR}/stories"
)

target_link_libraries(loom_story_tests PRIVATE
        loom_serialization
        loom_runtime
        loom_nodes
        Catch2::Catch2WithMain
)

catch_discover_tests(loom_story_tests)
