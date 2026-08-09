cmake_minimum_required(VERSION 3.20)
project(editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

# QtNodes build options, set before it is pulled in
set(USE_QT6 ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING     OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
        QtNodes
        GIT_REPOSITORY https://github.com/paceholder/nodeeditor.git
        GIT_TAG master
)

FetchContent_MakeAvailable(QtNodes)

add_executable(editor
        src/main.cpp
        src/editor_view.h
        src/editor_view.cpp
        src/mainwindow.h
        src/mainwindow.cpp

        src/nodes/editor_node.h
        src/nodes/editor_start_node.h
        src/nodes/editor_narrative_node.h
        src/nodes/editor_choice_node.h
        src/nodes/editor_condition_node.h
        src/nodes/editor_end_node.h
        src/nodes/editor_branch_node.h
        src/nodes/editor_compare_node.h
        src/nodes/editor_boolean_node.h
        src/nodes/editor_value_node.h
        src/nodes/editor_node_geometry.h
        src/nodes/editor_node_geometry.cpp
        src/editor_scene.h
        src/editor_scene.cpp
)

target_link_libraries(
        editor
        PRIVATE Qt6::Widgets
        PRIVATE QtNodes::QtNodes
)

target_include_directories(
        editor
        PRIVATE src
)
