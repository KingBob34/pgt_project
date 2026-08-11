find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# WIN32 keeps the console window from appearing behind the game
add_executable(loom_player WIN32
        src/main.cpp
        src/player_window.cpp
)

target_link_libraries(loom_player PRIVATE
        Qt6::Widgets
        loom_serialization
        loom_runtime
        loom_nodes
)
