find_package(Qt6 REQUIRED COMPONENTS Widgets)

# What the editor and the game both need to show a story, and nothing else.
add_library(loom_qt_common STATIC
        src/convert.cpp
        src/passage.cpp
)

target_include_directories(loom_qt_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(loom_qt_common PUBLIC Qt6::Widgets loom_graph)
