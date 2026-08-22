find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

include(FetchContent)

# Pinned, not master: the view relies on how QtNodes handles mouse drags
set(QT_NODES_DEVELOPER_DEFAULTS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        QtNodes
        GIT_REPOSITORY https://github.com/paceholder/nodeeditor.git
        GIT_TAG 7bbcd3e08c8330e9a5b47dbb730955849b8f547c
)
FetchContent_MakeAvailable(QtNodes)

add_executable(LoomEngine WIN32
        src/main.cpp
        src/editor_window.cpp
        src/inline_edit.cpp
        src/move_command.cpp
        src/node_adaptor.cpp
        src/graph_model.cpp
        src/graph_scene.cpp
        src/graph_view.cpp
        src/rubber_band_style.cpp
        src/node_geometry.cpp
        src/graph_document.cpp
        src/pin_editor.cpp
        src/details_panel.cpp
        src/value_tree.cpp
        src/playtest_panel.cpp
        src/node_painter.cpp
        src/connection_painter.cpp
        src/node_palette.cpp
        src/prose_editor.cpp
)

# The tool that copies Qt's own libraries next to an exported game. Found
# beside qmake, which Qt6 always points at.
get_target_property(LOOM_QMAKE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(LOOM_QT_BIN "${LOOM_QMAKE}" DIRECTORY)

# Where the open and save dialogs start. Stands in until the packaged engine
# has a home of its own to point at.
target_compile_definitions(LoomEngine PRIVATE
        LOOM_STORIES_DIR="${CMAKE_SOURCE_DIR}/stories"
        LOOM_WINDEPLOYQT="${LOOM_QT_BIN}/windeployqt.exe"
)

target_link_libraries(LoomEngine PRIVATE
        Qt6::Widgets
        QtNodes
        loom_qt_common
        loom_serialization
        loom_runtime
        loom_nodes
)
