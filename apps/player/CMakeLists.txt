find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# WIN32 keeps the console window from appearing behind the game
add_executable(LoomGame WIN32
        src/main.cpp
        src/option_button.cpp
        src/player_window.cpp
)

target_link_libraries(LoomGame PRIVATE
        Qt6::Widgets
        loom_qt_common
        loom_serialization
        loom_runtime
        loom_nodes
)
