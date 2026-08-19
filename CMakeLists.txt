cmake_minimum_required(VERSION 3.20)
project(OpenCircuitX VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# wxWidgets
# ---------------------------------------------------------------------------
# On Windows with a manual wxWidgets install, set WXWIN before running cmake:
#   cmake -DWXWIN=C:/wxWidgets ..
# On Linux/macOS with wx-config on PATH, find_package handles it automatically.

if(DEFINED WXWIN)
    set(wxWidgets_ROOT_DIR "${WXWIN}")
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base stc aui adv net)
include(${wxWidgets_USE_FILE})

# curl is only needed for the non-Windows update-check path
# (Windows uses WinINet instead, see src/core/update_check.cpp)
if(NOT WIN32)
    find_package(CURL REQUIRED)
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

set(OCX_SOURCES
    OpenCircuitX/main.cpp

    OpenCircuitX/src/core/circuit/gate.cpp
    OpenCircuitX/src/core/hdl/hdl_parser.cpp
    OpenCircuitX/src/core/hdl/vhdl_entity_parser.cpp
    OpenCircuitX/src/core/hdl/vhdl_gate_extractor.cpp
    OpenCircuitX/src/core/plugin/plugin_manager.cpp
    OpenCircuitX/src/core/simulation/circuit_simulator.cpp
    OpenCircuitX/src/core/simulation/vcd_parser.cpp
    OpenCircuitX/src/core/toolchain/fpga_toolchain.cpp
    OpenCircuitX/src/core/update_check.cpp

    OpenCircuitX/src/ui/canvas/canvas_draw.cpp
    OpenCircuitX/src/ui/canvas/canvas_edit.cpp
    OpenCircuitX/src/ui/canvas/canvas_input.cpp
    OpenCircuitX/src/ui/canvas/canvas_io.cpp
    OpenCircuitX/src/ui/canvas/canvas_sim.cpp
    OpenCircuitX/src/ui/canvas/circuit_canvas.cpp
    OpenCircuitX/src/ui/canvas/gate_renderer_ansi.cpp

    OpenCircuitX/src/ui/dialogs/about_dialog.cpp
    OpenCircuitX/src/ui/dialogs/new_project_dialog.cpp
    OpenCircuitX/src/ui/dialogs/open_project_dialog.cpp
    OpenCircuitX/src/ui/dialogs/quick_open_dialog.cpp
    OpenCircuitX/src/ui/dialogs/settings_dialog.cpp
    OpenCircuitX/src/ui/dialogs/symbol_search_dialog.cpp
    OpenCircuitX/src/ui/dialogs/update_checker.cpp

    OpenCircuitX/src/ui/editor/logic_editor_edit.cpp
    OpenCircuitX/src/ui/editor/logic_editor_panel.cpp
    OpenCircuitX/src/ui/editor/logic_editor_search.cpp
    OpenCircuitX/src/ui/editor/logic_editor_tabs.cpp

    OpenCircuitX/src/ui/mainwindow/mainwindow.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_build_ops.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_editor_ops.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_explorer_ops.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_file_ops.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_fpga_ops.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_hdl_ops.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_project_loading.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_snippets.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_testbench.cpp
    OpenCircuitX/src/ui/mainwindow/mainwindow_theme_ops.cpp

    OpenCircuitX/src/ui/panels/canvas_action_panel.cpp
    OpenCircuitX/src/ui/panels/component_palette.cpp
    OpenCircuitX/src/ui/panels/find_results_panel.cpp
    OpenCircuitX/src/ui/panels/outline_panel.cpp
    OpenCircuitX/src/ui/panels/output_panel.cpp
    OpenCircuitX/src/ui/panels/project_explorer_panel.cpp
    OpenCircuitX/src/ui/panels/watch_panel.cpp
    OpenCircuitX/src/ui/panels/welcome_panel.cpp
    OpenCircuitX/src/ui/panels/workspace_tabs.cpp

    OpenCircuitX/src/ui/rtl_view/rtl_view_draw.cpp
    OpenCircuitX/src/ui/rtl_view/rtl_view_events.cpp
    OpenCircuitX/src/ui/rtl_view/rtl_view_panel.cpp
    OpenCircuitX/src/ui/rtl_view/rtl_view_schematic.cpp

    OpenCircuitX/src/ui/shell/app_theme.cpp
    OpenCircuitX/src/ui/shell/ocx_menubar.cpp
    OpenCircuitX/src/ui/shell/ocx_toolbar.cpp

    OpenCircuitX/src/ui/waveform/waveform_panel.cpp
    OpenCircuitX/src/ui/waveform/waveform_wavepanel.cpp

    OpenCircuitX/src/utils/project_file.cpp

    # Vendored Material Design icon provider (repo-tracked, not the wxWidgets
    # build dependency in pack/)
    OpenCircuitX/include/mdap/wxMaterialDesignArtProvider.cpp
    OpenCircuitX/include/mdap/wxMaterialFilledArt.cpp
)

# ---------------------------------------------------------------------------
# Platform-specific additions
# ---------------------------------------------------------------------------

if(WIN32)
    # Embed the Windows resource file (icon + VERSIONINFO)
    list(APPEND OCX_SOURCES OpenCircuitX/OpenCircuitX.rc)
    add_executable(OpenCircuitX WIN32 ${OCX_SOURCES})
elseif(APPLE)
    # macOS app bundle
    set(MACOSX_BUNDLE_BUNDLE_NAME "OpenCircuitX")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    add_executable(OpenCircuitX MACOSX_BUNDLE ${OCX_SOURCES})
else()
    # Linux
    add_executable(OpenCircuitX ${OCX_SOURCES})
endif()

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------

target_include_directories(OpenCircuitX PRIVATE
    OpenCircuitX/src
    OpenCircuitX/include
    OpenCircuitX/include/mdap
)

# ---------------------------------------------------------------------------
# Link
# ---------------------------------------------------------------------------

target_link_libraries(OpenCircuitX PRIVATE ${wxWidgets_LIBRARIES})

if(NOT WIN32)
    target_link_libraries(OpenCircuitX PRIVATE CURL::libcurl)
endif()

# On Linux, wxWidgets may need dl for plugin loading (wxDynamicLibrary)
if(UNIX AND NOT APPLE)
    target_link_libraries(OpenCircuitX PRIVATE dl)
endif()

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------

if(MSVC)
    target_compile_options(OpenCircuitX PRIVATE /W3)
else()
    target_compile_options(OpenCircuitX PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------

install(TARGETS OpenCircuitX
        RUNTIME DESTINATION bin          # Windows / Linux exe
        BUNDLE  DESTINATION .            # macOS .app
)

# Install a plugins/ directory so users can drop plugin DLLs/SOs there
install(DIRECTORY DESTINATION bin/plugins)
