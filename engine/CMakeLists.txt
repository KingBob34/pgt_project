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
        src/story.h
        src/terminal_io.h
        src/terminal_io.cpp
        src/engine.h
        src/engine.cpp
        src/main.cpp
        src/loader.h
        src/loader.cpp
)

target_link_libraries(logic_demo PRIVATE nlohmann_json::nlohmann_json)
