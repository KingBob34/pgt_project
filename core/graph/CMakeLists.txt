add_library(loom_graph STATIC
        src/pin.cpp
        src/execution.cpp
        src/graph.cpp
        src/catalog.cpp
        src/diagnostics.cpp
        src/validate.cpp
)

target_include_directories(loom_graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(loom_graph PUBLIC loom_value)

if (LOOM_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
