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
        src/to_float_node.cpp
        src/to_string_node.cpp
        src/to_bool_node.cpp

        src/add_node.cpp
        src/subtract_node.cpp
        src/multiply_node.cpp
        src/and_node.cpp
        src/or_node.cpp
        src/not_node.cpp

        src/add_to_list_node.cpp
        src/remove_from_list_node.cpp
        src/contains_node.cpp
        src/list_count_node.cpp

        src/comment_node.cpp
        src/print_node.cpp
)

target_include_directories(loom_nodes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(loom_nodes PUBLIC loom_graph)

if (LOOM_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
