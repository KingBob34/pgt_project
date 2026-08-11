add_library(loom_runtime STATIC
        src/interpreter.cpp
        src/save.cpp
)

target_include_directories(loom_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(loom_runtime PUBLIC loom_graph)

if (LOOM_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
