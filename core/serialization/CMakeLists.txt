add_library(loom_serialization STATIC
        src/graph_writer.cpp
        src/graph_reader.cpp
)

target_include_directories(loom_serialization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(loom_serialization PUBLIC loom_graph)

if (LOOM_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
