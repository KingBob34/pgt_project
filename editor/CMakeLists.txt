cmake_minimum_required(VERSION 3.20)
project(gui_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(gui_demo
        src/main.cpp
        src/mainwindow.h
        src/mainwindow.cpp
        src/canvasview.h
        src/canvasview.cpp
)

target_link_libraries(gui_demo PRIVATE Qt6::Widgets)
