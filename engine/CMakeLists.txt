cmake_minimum_required(VERSION 3.20)

project(logic_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(nlohmann_json)

add_executable(logic_demo
        src/value.h
        src/condition.h
        src/condition.cpp
        src/scene.h
        src/scene.cpp
        src/game_state.h
        src/game_state.cpp

        src/nodes/node.h
        src/nodes/start_node.h
        src/nodes/narrative_node.h
        src/nodes/choice_node.h
        src/nodes/condition_node.h
        src/nodes/end_node.h
        src/nodes/node_registry.h
        src/nodes/node_registry.cpp
        src/node_result.h

        src/loader.h
        src/loader.cpp
        src/runner.h
        src/runner.cpp
        src/frontends/terminal.h
        src/frontends/terminal.cpp
        src/main.cpp
)

target_link_libraries(
        logic_demo
        PRIVATE nlohmann_json::nlohmann_json
)

target_include_directories(
        logic_demo
        PRIVATE src
)