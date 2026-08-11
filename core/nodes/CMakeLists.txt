add_library(loom_nodes STATIC
        src/nodes_internal.cpp
        src/builtin.cpp

        src/scene_start_node.cpp
        src/branch_node.cpp
        src/go_to_scene_node.cpp

        src/show_text_node.cpp
        src/show_choices_node.cpp

        src/get_variable_node.cpp
        src/set_variable_node.cpp

        src/string_value_node.cpp
        src/integer_value_node.cpp
        src/float_value_node.cpp
        src/bool_value_node.cpp
        src/color_value_node.cpp

        src/random_integer_node.cpp
        src/equal_node.cpp
        src/less_node.cpp
        src/less_or_equal_node.cpp
        src/greater_node.cpp
        src/greater_or_equal_node.cpp
        src/floor_node.cpp
        src/ceil_node.cpp
        src/round_node.cpp

        src/comment_node.cpp
        src/print_node.cpp
)

target_include_directories(loom_nodes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(loom_nodes PUBLIC loom_graph)

if (LOOM_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
