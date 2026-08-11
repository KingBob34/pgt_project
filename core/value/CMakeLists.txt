add_library(loom_value STATIC
        src/inspect.cpp
        src/path.cpp
        src/parse.cpp
)

target_include_directories(loom_value PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(loom_value PUBLIC nlohmann_json::nlohmann_json)

if (LOOM_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
