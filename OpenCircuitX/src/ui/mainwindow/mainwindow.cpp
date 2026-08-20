#include "mainwindow.h"
#ifdef __WXMSW__
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif
#include "ui/dialogs/new_project_dialog.h"
#include "ui/dialogs/open_project_dialog.h"
#include "ui/editor/logic_editor_panel.h"
#include "ui/panels/output_panel.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/dialogs/about_dialog.h"
#include "ui/dialogs/update_checker.h"
#include "ui/canvas/circuit_canvas.h"
#include "ui/panels/component_palette.h"
#include "ui/panels/canvas_action_panel.h"
#include "ui/waveform/waveform_panel.h"
#include "ui/panels/watch_panel.h"
#include "ui/panels/welcome_panel.h"
#include "ui/rtl_view/rtl_view_panel.h"
#include "ui/panels/outline_panel.h"
#include "ui/dialogs/symbol_search_dialog.h"
#include "ui/dialogs/quick_open_dialog.h"
#include "ui/panels/find_results_panel.h"
#include "core/simulation/circuit_simulator.h"
#include "core/toolchain/fpga_toolchain.h"
#include "core/plugin/plugin_manager.h"
#include "core/update_check.h"
#include "core/version.h"
#include "utils/project_file.h"
#include "ui/shell/app_theme.h"
#include "mdap/wxMaterialDesignArtProvider.hpp"
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/textfile.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/artprov.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/imaglist.h>
#include <wx/tokenzr.h>
#include <wx/numdlg.h>
#include <wx/bookctrl.h>
#include <wx/dcbuffer.h>
#include <wx/renderer.h>
#include <wx/file.h>
#include <vector>

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_MENU(wxID_NEW,              MainWindow::OnNewProject)
    EVT_MENU(ID_OpenProject,        MainWindow::OnOpenProject)
    EVT_MENU(wxID_SAVE,             MainWindow::OnSaveFile)
    EVT_MENU(wxID_SAVEAS,           MainWindow::OnSaveProject)
    EVT_MENU(ID_SaveSimulation,     MainWindow::OnSaveSimulation)
    EVT_MENU(wxID_EXIT,             MainWindow::OnExit)
    EVT_MENU(ID_Settings,           MainWindow::OnSettings)
    EVT_MENU(ID_CompileHDL,         MainWindow::OnCompileHDL)
    EVT_MENU(ID_RunSimulation,      MainWindow::OnRunSimulation)
    EVT_MENU(ID_DebugSimulation,    MainWindow::OnDebugSimulation)
    EVT_MENU(ID_NewFileInProject,   MainWindow::OnNewFileInProject)
    EVT_MENU(ID_NewFolderInProject, MainWindow::OnNewFolderInProject)
    EVT_MENU(ID_RenameFile,         MainWindow::OnRenameFile)
    EVT_MENU(ID_DeleteFile,         MainWindow::OnDeleteFile)
    EVT_MENU(ID_About,            MainWindow::OnAbout)
    EVT_MENU(ID_CheckForUpdates,  MainWindow::OnCheckForUpdates)
    EVT_MENU(ID_Find,                MainWindow::OnFind)
    EVT_MENU(ID_Replace,             MainWindow::OnReplace)
    EVT_MENU(ID_GotoLine,            MainWindow::OnGotoLine)
    EVT_MENU(ID_GenTestbench,        MainWindow::OnGenTestbench)
    EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, MainWindow::OnRecentFile)
    EVT_MENU(ID_ToggleLineNumbers,   MainWindow::OnToggleLineNumbers)
    EVT_MENU(ID_ToggleCodeFolding,   MainWindow::OnToggleCodeFolding)
    EVT_MENU(ID_LintVerilator,       MainWindow::OnLintVerilator)
    EVT_MENU(ID_RunVerilator,        MainWindow::OnRunVerilator)
    EVT_MENU(ID_ThemeDark,           MainWindow::OnThemeDark)
    EVT_MENU(ID_ThemeMidnight,       MainWindow::OnThemeMidnight)
    EVT_MENU(ID_ThemeLight,          MainWindow::OnThemeLight)
    EVT_MENU(ID_FPGASynthesize,      MainWindow::OnFPGASynthesize)
    EVT_MENU(ID_FPGAPlaceRoute,      MainWindow::OnFPGAPlaceRoute)
    EVT_MENU(ID_FPGAProgram,         MainWindow::OnFPGAProgram)
    EVT_TREE_SEL_CHANGED(wxID_ANY,       MainWindow::OnProjectFileSelected)
    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY,  MainWindow::OnProjectExplorerRightClick)
    EVT_MENU_RANGE(ID_SnippetFirst, ID_SnippetLast, MainWindow::OnInsertSnippet)
    EVT_MENU(ID_FindInFiles,     MainWindow::OnFindInFiles)
    EVT_MENU(ID_EditorZoomIn,    MainWindow::OnEditorZoomIn)
    EVT_MENU(ID_EditorZoomOut,   MainWindow::OnEditorZoomOut)
    EVT_MENU(ID_EditorZoomReset, MainWindow::OnEditorZoomReset)
    EVT_MENU(ID_ToggleComment,   MainWindow::OnToggleComment)
    EVT_MENU(ID_ToggleWordWrap,         MainWindow::OnToggleWordWrap)
    EVT_MENU(ID_ToggleShowWhitespace,   MainWindow::OnToggleShowWhitespace)
    EVT_MENU(ID_ReopenTab,              MainWindow::OnReopenTab)
    EVT_MENU(ID_ToggleBookmark,  MainWindow::OnToggleBookmark)
    EVT_MENU(ID_NextBookmark,    MainWindow::OnNextBookmark)
    EVT_MENU(ID_PrevBookmark,    MainWindow::OnPrevBookmark)
    EVT_TIMER(ID_AutoSaveTimer,  MainWindow::OnAutoSave)
    EVT_MENU(ID_GoToDefinition,       MainWindow::OnGoToDefinition)
    EVT_MENU(ID_SymbolSearch,         MainWindow::OnSymbolSearch)
    EVT_MENU(ID_ReplaceInFiles,       MainWindow::OnReplaceInFiles)
    EVT_MENU(ID_QuickOpen,            MainWindow::OnQuickOpen)
    EVT_MENU(ID_DuplicateLine,        MainWindow::OnDuplicateLine)
    EVT_MENU(ID_SelectAllOccurrences, MainWindow::OnSelectAllOccurrences)
    EVT_MENU(ID_JumpToMatchingBrace,  MainWindow::OnJumpToMatchingBrace)
    EVT_MENU(ID_SortLines,            MainWindow::OnSortLines)
    EVT_MENU(ID_UndoCanvas,           MainWindow::OnUndoCanvas)
    EVT_MENU(ID_RedoCanvas,           MainWindow::OnRedoCanvas)
    EVT_MENU(ID_KeyboardShortcuts,    MainWindow::OnKeyboardShortcuts)
    EVT_MENU(ID_CanvasCopy,           MainWindow::OnCanvasCopy)
    EVT_MENU(ID_CanvasPaste,          MainWindow::OnCanvasPaste)
    EVT_MENU(ID_CanvasSelectAll,      MainWindow::OnCanvasSelectAll)
    EVT_MENU(ID_CanvasDeleteSel,      MainWindow::OnCanvasDeleteSel)
    EVT_MENU_RANGE(ID_CanvasAlignLeft, ID_CanvasAlignCenterV, MainWindow::OnCanvasAlign)
    EVT_MENU(ID_CanvasExportPNG,      MainWindow::OnCanvasExportPNG)
    EVT_MENU(ID_CanvasExportVerilog,  MainWindow::OnCanvasExportVerilog)
wxEND_EVENT_TABLE()


//--
// ThemeStatusBar - owner-drawn status bar so text color follows the active theme.
// Native wxStatusBar on Windows ignores SetForegroundColour; this class paints
// its own fields using OCXTheme colours, while keeping the full wxStatusBar API
// (all SetStatusText calls in MainWindow continue to work unchanged).
//--
class ThemeStatusBar : public wxStatusBar
{
public:
    explicit ThemeStatusBar(wxWindow* parent)
        : wxStatusBar(parent, wxID_ANY, wxSTB_DEFAULT_STYLE)
    {
        SetBackgroundColour(OCXTheme::BgPanel());
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &ThemeStatusBar::OnPaint, this);
    }

private:
    void OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        wxSize sz = GetClientSize();

        dc.SetBackground(wxBrush(OCXTheme::BgPanel()));
        dc.Clear();

        // Top border
        dc.SetPen(wxPen(OCXTheme::BgSash(), 1));
        dc.DrawLine(0, 0, sz.x, 0);

        dc.SetFont(GetFont());
        dc.SetTextForeground(OCXTheme::FgText());

        const int pad    = 6;
        const int rightW = 160;   // must match SetStatusWidths
        int th = dc.GetTextExtent("A").GetHeight();
        int ty = (sz.y - th) / 2;

        // Left field
        wxString left = GetStatusText(0);
        if (!left.IsEmpty())
            dc.DrawText(left, pad, ty);

        // Field separator
        dc.SetPen(wxPen(OCXTheme::BgSash(), 1));
        dc.DrawLine(sz.x - rightW, 2, sz.x - rightW, sz.y - 2);

        // Right field (Ln / Col indicator)
        wxString right = GetStatusText(1);
        if (!right.IsEmpty())
            dc.DrawText(right, sz.x - rightW + pad, ty);

        // Sizing grip - diagonal tick marks in the bottom-right corner
        for (int i = 1; i <= 3; ++i)
        {
            int d = i * 4;
            dc.SetPen(wxPen(OCXTheme::FgDim(), 1));
            dc.DrawLine(sz.x - d,     sz.y - 2,
                        sz.x - 2,     sz.y - d);
        }
    }
};

MainWindow::MainWindow(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1280, 780))
{
    //** Restore saved theme before building UI **//
    {
        wxConfig cfg("OpenCircuitX");
        long savedTheme = 0;
        cfg.Read("Theme", &savedTheme, 0L);
        if (savedTheme == 1) OCXTheme::Set(OCXThemeId::Midnight);
        else if (savedTheme == 2) OCXTheme::Set(OCXThemeId::Light);
        else                      OCXTheme::Set(OCXThemeId::Dark);
    }

    //** App icon **//
    // Windows: embedded via OpenCircuitX.rc, loaded by resource name - no
    // file on disk needed. Linux/macOS have no such resource mechanism, so
    // res/app.png has to be loaded as a plain file instead, resolved next
    // to the actual running executable (CMake copies it there at build
    // time - see CMakeLists.txt) rather than assumed relative to cwd. PNG,
    // not ICO: gdk-pixbuf (what GNOME's dock/app search use to render a
    // .desktop file's Icon=) doesn't reliably decode this project's .ico,
    // even though wx's own in-app ICO handler can load it fine - two
    // separate decoders, confirmed by testing both on a real GNOME desktop.
#ifdef __WXMSW__
    SetIcon(wxIcon("IDI_MAIN", wxBITMAP_TYPE_ICO_RESOURCE));
#else
    {
        wxFileName exeDir(wxStandardPaths::Get().GetExecutablePath());
        wxString iconPath = exeDir.GetPath() + wxFILE_SEP_PATH + "app.png";
        wxIcon icon;
        if (wxFileExists(iconPath) && icon.LoadFile(iconPath, wxBITMAP_TYPE_PNG) && icon.IsOk())
            SetIcon(icon);
    }
#endif

    //** Menu bar **//
    wxMenuBar* menuBar = new wxMenuBar();

    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_NEW,          "&New Project\tCtrl+N",          "Create a new project");
    fileMenu->Append(ID_OpenProject,    "&Open Project\tCtrl+O",          "Open an existing project");
    m_recentMenu = new wxMenu();
    fileMenu->AppendSubMenu(m_recentMenu, "Recent &Projects", "Recently opened projects");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_QuickOpen,      "&Quick Open...\tCtrl+P",           "Fuzzy-search and open any project file");
    fileMenu->Append(ID_ReopenTab,      "Reopen &Closed Tab\tCtrl+Shift+T","Reopen the last closed editor tab");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_SAVE,         "&Save File\tCtrl+S",             "Save the current editor file");
    fileMenu->Append(wxID_SAVEAS,       "Save &Project\tCtrl+Shift+P",    "Save project file");
    fileMenu->Append(ID_SaveSimulation, "Save S&imulation\tCtrl+Shift+S", "Save simulation file");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT,         "E&xit\tAlt+F4",                  "Exit the application");

    //** File history (Recent Projects) **//
    m_fileHistory = new wxFileHistory(9);
    m_fileHistory->UseMenu(m_recentMenu);
    {
        wxConfig cfg("OpenCircuitX");
        m_fileHistory->Load(cfg);
    }

    //** Edit menu (snippets) **//
    wxMenu* snippetVHDL    = new wxMenu();
    wxMenu* snippetVerilog = new wxMenu();
    snippetVHDL->Append(ID_SnippetFirst + 0, "Process (clk + reset)",     "Synchronous clocked process");
    snippetVHDL->Append(ID_SnippetFirst + 1, "Moore State Machine",        "FSM with state type + next-state logic");
    snippetVHDL->Append(ID_SnippetFirst + 2, "Component Instantiation",   "Component port-map skeleton");
    snippetVHDL->Append(ID_SnippetFirst + 3, "Generate...For Loop",       "Replicated structure");
    snippetVHDL->Append(ID_SnippetFirst + 4, "Function Body",              "Pure function skeleton");
    snippetVHDL->Append(ID_SnippetFirst + 5, "Package Declaration",        "Package + body skeleton");
    snippetVerilog->Append(ID_SnippetFirst + 6, "Always @(posedge clk)",   "Synchronous always block");
    snippetVerilog->Append(ID_SnippetFirst + 7, "Case State Machine",       "FSM with parameter states");
    snippetVerilog->Append(ID_SnippetFirst + 8, "Task",                     "Task skeleton");
    snippetVerilog->Append(ID_SnippetFirst + 9, "Generate...For Loop",      "Replicated structure");

    wxMenu* editMenu = new wxMenu();
    editMenu->Append(ID_UndoCanvas,  "Undo Canvas\tCtrl+Z", "Undo last canvas operation");
    editMenu->Append(ID_RedoCanvas,  "Redo Canvas\tCtrl+Y", "Redo last undone canvas operation");
    editMenu->AppendSeparator();

    wxMenu* canvasMenu = new wxMenu();
    canvasMenu->Append(ID_CanvasCopy,      "Copy Gates\tCtrl+C",      "Copy selected gates to clipboard");
    canvasMenu->Append(ID_CanvasPaste,     "Paste Gates\tCtrl+V",     "Paste gates from clipboard");
    canvasMenu->Append(ID_CanvasSelectAll, "Select All\tCtrl+A",      "Select all gates");
    canvasMenu->Append(ID_CanvasDeleteSel, "Delete Selected\tDel",    "Delete selected gates");
    canvasMenu->AppendSeparator();
    canvasMenu->Append(ID_CanvasAlignLeft,    "Align Left",               "Align selected gates to the leftmost");
    canvasMenu->Append(ID_CanvasAlignRight,   "Align Right",              "Align selected gates to the rightmost");
    canvasMenu->Append(ID_CanvasAlignTop,     "Align Top",                "Align selected gates to the topmost");
    canvasMenu->Append(ID_CanvasAlignBottom,  "Align Bottom",             "Align selected gates to the bottommost");
    canvasMenu->Append(ID_CanvasAlignCenterH, "Distribute Horizontally",  "Distribute gate centres along horizontal axis");
    canvasMenu->Append(ID_CanvasAlignCenterV, "Distribute Vertically",    "Distribute gate centres along vertical axis");
    editMenu->AppendSubMenu(canvasMenu, "&Canvas", "Circuit canvas editing operations");
    editMenu->AppendSeparator();
    editMenu->AppendSubMenu(snippetVHDL,    "Insert VHDL Snippet",    "Common VHDL code patterns");
    editMenu->AppendSubMenu(snippetVerilog, "Insert Verilog Snippet", "Common Verilog code patterns");
    editMenu->AppendSeparator();
    editMenu->Append(ID_ToggleComment,        "Toggle &Comment\tCtrl+/",             "Comment or uncomment selected lines");
    editMenu->Append(ID_DuplicateLine,        "&Duplicate Line\tCtrl+D",             "Duplicate the current line or selection");
    editMenu->Append(ID_SelectAllOccurrences, "Select &All Occurrences\tCtrl+Shift+L","Highlight all occurrences of word under cursor");
    editMenu->Append(ID_JumpToMatchingBrace,  "Jump to &Matching Brace\tCtrl+]",     "Move caret to the matching bracket");
    editMenu->Append(ID_SortLines,            "&Sort Selected Lines",                 "Sort selected lines alphabetically");
    editMenu->AppendSeparator();
    editMenu->Append(ID_FindInFiles,          "Find in &Files...\tCtrl+Shift+F",     "Search all project source files");
    editMenu->Append(ID_ReplaceInFiles,       "Replace in F&iles...\tCtrl+Shift+H",  "Find and replace across all project files");

    m_toolsMenu = new wxMenu();
    wxMenu* toolsMenu = m_toolsMenu;
    toolsMenu->Append(ID_CompileHDL,      "&Compile HDL\tF5",      "Compile HDL code with GHDL");
    toolsMenu->Append(ID_RunSimulation,   "&Run Simulation\tF6",   "Run simulation with GHDL");
    toolsMenu->Append(ID_DebugSimulation, "&Debug Simulation\tF7", "Debug simulation");
    toolsMenu->AppendSeparator();
    toolsMenu->Append(ID_LintVerilator,   "&Lint with Verilator\tF8",        "Static analysis with Verilator");
    toolsMenu->Append(ID_RunVerilator,    "Run with &Verilator\tShift+F8",   "Build and run with Verilator");
    toolsMenu->AppendSeparator();

    wxMenu* fpgaMenu = new wxMenu();
    fpgaMenu->Append(ID_FPGASynthesize, "&Synthesize with Yosys\tF9",       "Synthesize HDL to netlist");
    fpgaMenu->Append(ID_FPGAPlaceRoute, "&Place && Route (nextpnr)\tShift+F9","Place & route for iCE40");
    fpgaMenu->Append(ID_FPGAProgram,    "&Program Board\tCtrl+F9",           "Flash bitstream via openFPGALoader");
    toolsMenu->AppendSubMenu(fpgaMenu, "&FPGA Toolchain", "Yosys / nextpnr / openFPGALoader");
    toolsMenu->AppendSeparator();
    toolsMenu->Append(ID_CanvasExportPNG,     "Export Canvas as &PNG...",     "Save the circuit canvas as a PNG image");
    toolsMenu->Append(ID_CanvasExportVerilog, "Export Canvas to &Verilog...", "Generate structural Verilog from the circuit canvas");
    toolsMenu->AppendSeparator();
    toolsMenu->Append(ID_GenTestbench,    "Generate &Testbench\tCtrl+T",      "Auto-generate a VHDL testbench for the current entity");
    toolsMenu->AppendSeparator();
    toolsMenu->Append(ID_Settings,        "&Settings...",                     "Application settings");

    wxMenu* themeMenu = new wxMenu();
    themeMenu->AppendRadioItem(ID_ThemeDark,     "&Dark",     "VS Code Dark palette");
    themeMenu->AppendRadioItem(ID_ThemeMidnight, "&Midnight", "GitHub Dark palette");
    themeMenu->AppendRadioItem(ID_ThemeLight,    "&Light",    "Clean white palette");

    wxMenu* viewMenu = new wxMenu();
    viewMenu->Append(ID_Find,          "&Find in Editor\tCtrl+F",       "Show inline find bar");
    viewMenu->Append(ID_Replace,       "Find && &Replace\tCtrl+H",      "Show find & replace bar");
    viewMenu->Append(ID_GotoLine,      "&Go to Line...\tCtrl+G",        "Jump to a specific line");
    viewMenu->Append(ID_GoToDefinition,"Go to &Definition\tF12",        "Jump to the definition of the word under the cursor");
    viewMenu->Append(ID_SymbolSearch,  "&Symbol Search\tCtrl+Shift+O",  "Quick jump to any symbol in the current file");
    viewMenu->AppendSeparator();
    wxMenu* zoomMenu = new wxMenu();
    zoomMenu->Append(ID_EditorZoomIn,    "Zoom &In\tCtrl+=",    "Increase editor font size");
    zoomMenu->Append(ID_EditorZoomOut,   "Zoom &Out\tCtrl+-",   "Decrease editor font size");
    zoomMenu->Append(ID_EditorZoomReset, "&Reset Zoom\tCtrl+0", "Reset editor font size");
    viewMenu->AppendSubMenu(zoomMenu, "Editor &Zoom", "Scale editor text size");
    viewMenu->AppendSeparator();
    wxMenu* bookmarkMenu = new wxMenu();
    bookmarkMenu->Append(ID_ToggleBookmark, "Toggle &Bookmark\tCtrl+B",       "Set or clear a bookmark on the current line");
    bookmarkMenu->Append(ID_NextBookmark,   "&Next Bookmark\tF2",             "Jump to the next bookmark");
    bookmarkMenu->Append(ID_PrevBookmark,   "&Previous Bookmark\tShift+F2",   "Jump to the previous bookmark");
    viewMenu->AppendSubMenu(bookmarkMenu, "&Bookmarks", "Line bookmark navigation");
    viewMenu->AppendSeparator();
    viewMenu->AppendCheckItem(ID_ToggleLineNumbers, "Show &Line Numbers", "Toggle line number gutter");
    viewMenu->AppendCheckItem(ID_ToggleCodeFolding, "Code &Folding",      "Toggle code folding margin");
    viewMenu->AppendCheckItem(ID_ToggleWordWrap,         "&Word Wrap",          "Wrap long lines in the editor");
    viewMenu->AppendCheckItem(ID_ToggleShowWhitespace,   "Show &Whitespace",    "Show spaces and tabs as visible characters");
    viewMenu->Check(ID_ToggleLineNumbers, true);
    viewMenu->AppendSeparator();
    viewMenu->AppendSubMenu(themeMenu, "&Theme", "Switch colour theme");

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(ID_KeyboardShortcuts, "&Keyboard Shortcuts...\tCtrl+Shift+?", "Show all keyboard shortcuts");
    helpMenu->AppendSeparator();
    m_checkUpdatesItem = helpMenu->Append(ID_CheckForUpdates, "&Check for Updates...", "Check for a newer version");
    helpMenu->AppendSeparator();
    helpMenu->Append(ID_About, "&About OpenCircuitX...", "About this application");

    // Do NOT call SetMenuBar - OCXMenuBar replaces it visually.
    // The wxMenu* objects are still used via PopupMenu, so events still reach MainWindow.

    // Tick the active theme radio item
    if (OCXTheme::Get() == OCXThemeId::Midnight)
        themeMenu->Check(ID_ThemeMidnight, true);
    else if (OCXTheme::Get() == OCXThemeId::Light)
        themeMenu->Check(ID_ThemeLight, true);
    else
        themeMenu->Check(ID_ThemeDark, true);

    //** Keyboard accelerator table (replaces wxMenuBar's built-in shortcuts) **//
    {
        wxAcceleratorEntry accel[] = {
            // File
            { wxACCEL_CTRL,               'N',          wxID_NEW              },
            { wxACCEL_CTRL,               'O',          ID_OpenProject        },
            { wxACCEL_CTRL,               'S',          wxID_SAVE             },
            { wxACCEL_CTRL,               'P',          ID_QuickOpen          },
            { wxACCEL_CTRL | wxACCEL_SHIFT, 'T',        ID_ReopenTab          },
            // Edit
            { wxACCEL_CTRL,               'Z',          ID_UndoCanvas         },
            { wxACCEL_CTRL,               'Y',          ID_RedoCanvas         },
            { wxACCEL_CTRL,               'D',          ID_DuplicateLine      },
            { wxACCEL_CTRL | wxACCEL_SHIFT, 'L',        ID_SelectAllOccurrences },
            { wxACCEL_CTRL | wxACCEL_SHIFT, 'F',        ID_FindInFiles        },
            { wxACCEL_CTRL | wxACCEL_SHIFT, 'H',        ID_ReplaceInFiles     },
            { wxACCEL_CTRL,               '/',          ID_ToggleComment      },
            // View / Navigation
            { wxACCEL_CTRL,               'F',          ID_Find               },
            { wxACCEL_CTRL,               'H',          ID_Replace            },
            { wxACCEL_CTRL,               'G',          ID_GotoLine           },
            { wxACCEL_NORMAL,             WXK_F12,      ID_GoToDefinition     },
            { wxACCEL_CTRL | wxACCEL_SHIFT, 'O',        ID_SymbolSearch       },
            // Bookmarks
            { wxACCEL_CTRL,               'B',          ID_ToggleBookmark     },
            { wxACCEL_NORMAL,             WXK_F2,       ID_NextBookmark       },
            { wxACCEL_SHIFT,              WXK_F2,       ID_PrevBookmark       },
            // Zoom
            { wxACCEL_CTRL,               '=',          ID_EditorZoomIn       },
            { wxACCEL_CTRL,               '-',          ID_EditorZoomOut      },
            { wxACCEL_CTRL,               '0',          ID_EditorZoomReset    },
            // Tools
            { wxACCEL_NORMAL,             WXK_F5,       ID_CompileHDL         },
            { wxACCEL_NORMAL,             WXK_F6,       ID_RunSimulation      },
            { wxACCEL_NORMAL,             WXK_F7,       ID_DebugSimulation    },
            { wxACCEL_NORMAL,             WXK_F8,       ID_LintVerilator      },
            { wxACCEL_SHIFT,              WXK_F8,       ID_RunVerilator       },
            { wxACCEL_NORMAL,             WXK_F9,       ID_FPGASynthesize     },
            { wxACCEL_SHIFT,              WXK_F9,       ID_FPGAPlaceRoute     },
            { wxACCEL_CTRL,               WXK_F9,       ID_FPGAProgram        },
            { wxACCEL_CTRL,               'T',          ID_GenTestbench       },
        };
        SetAcceleratorTable(wxAcceleratorTable(WXSIZEOF(accel), accel));
    }

    //** Dark frame **//
    SetBackgroundColour(OCXTheme::BgApp());

    //** Status bar (owner-drawn - 2 fields: messages | line/col) **//
    {
        ThemeStatusBar* sb = new ThemeStatusBar(this);
        SetStatusBar(sb);
        sb->SetFieldsCount(2);
        int w[] = { -4, 160 };
        sb->SetStatusWidths(2, w);
    }
    OCXStatus("Ready");

    //** Circuit simulator **//
    circuitSimulator = new CircuitSimulator();

    wxConfig config("OpenCircuitX");
    wxString ghdlPath;
    config.Read("GHDLPath", &ghdlPath, "ghdl");
    circuitSimulator->SetGHDLPath(ghdlPath);

    wxString stopTime;
    config.Read("StopTime", &stopTime, "");
    circuitSimulator->SetRunTime(stopTime);

    wxString icarusPath;
    config.Read("IcarusPath", &icarusPath, "iverilog");
    circuitSimulator->SetIcarusPath(icarusPath);

    wxString verilatorPath;
    config.Read("VerilatorPath", &verilatorPath, "verilator");
    circuitSimulator->SetVerilatorPath(verilatorPath);

    //** FPGA toolchain **//
    fpgaToolchain = new FPGAToolchain();
    {
        wxString yosysPath, nextpnrPath, nextpnrECP5Path,
                 icepackPath, ecppackPath, loaderPath, boardId, ghdlPlugin;
        config.Read("YosysPath",          &yosysPath,       "yosys");
        config.Read("NextpnrPath",        &nextpnrPath,     "nextpnr-ice40");
        config.Read("NextpnrECP5Path",    &nextpnrECP5Path, "nextpnr-ecp5");
        config.Read("IcepackPath",        &icepackPath,     "icepack");
        config.Read("EcppackPath",        &ecppackPath,     "ecppack");
        config.Read("OpenFPGALoaderPath", &loaderPath,      "openFPGALoader");
        config.Read("FPGABoard",          &boardId,         "ibreaker");
        config.Read("GHDLYosysPlugin",    &ghdlPlugin,      "");
        fpgaToolchain->SetYosysPath(yosysPath);
        fpgaToolchain->SetNextpnrPath(nextpnrPath);
        fpgaToolchain->SetNextpnrECP5Path(nextpnrECP5Path);
        fpgaToolchain->SetIcepackPath(icepackPath);
        fpgaToolchain->SetEcppackPath(ecppackPath);
        fpgaToolchain->SetOpenFPGALoaderPath(loaderPath);
        fpgaToolchain->SetGHDLPluginPath(ghdlPlugin);
        m_fpgaBoardId = boardId;
        OCXStatus("Ready  |  Board: " + m_fpgaBoardId);
    }

    //** Windows 11 dark title bar **//
#ifdef __WXMSW__
    {
        HWND hwnd = (HWND)GetHWND();
        BOOL dark = TRUE;
        // DWMWA_USE_IMMERSIVE_DARK_MODE = 20 (Win11), 19 (Win10 older builds)
        DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
        DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
    }
#endif

    //** Root panel (holds OCXMenuBar + OCXToolBar + splitter content) **//
    wxPanel* rootPanel = new wxPanel(this, wxID_ANY);
    rootPanel->SetBackgroundColour(OCXTheme::BgApp());

    // OCX Menu Bar
    m_ocxMenuBar = new OCXMenuBar(rootPanel);
    m_ocxMenuBar->Append(fileMenu,  "&File");
    m_ocxMenuBar->Append(editMenu,  "&Edit");
    m_ocxMenuBar->Append(toolsMenu, "&Tools");
    m_ocxMenuBar->Append(viewMenu,  "&View");
    m_ocxMenuBar->Append(helpMenu,  "&Help");
    m_ocxMenuBar->SetVersionInfo(OCX_VERSION_STRING, [this]() {
        wxCommandEvent e;
        OnCheckForUpdates(e);
    });

    // OCX Toolbar
    m_ocxToolBar = new OCXToolBar(rootPanel);
    {
        wxSize bmpSize(OCXToolBar::BMP_SZ, OCXToolBar::BMP_SZ);
        wxColour clrNeutral = OCXTheme::FgText();
        auto MI = [&](const wxArtID& id, const wxColour& clr) {
            return wxMaterialDesignArtProvider::GetBitmap(id, wxART_CLIENT_MATERIAL_FILLED, bmpSize, clr);
        };
        m_ocxToolBar->AddTool(wxID_NEW,           MI(wxART_NOTE_ADD,    wxColour(  0,150,220)), "New");
        m_ocxToolBar->AddTool(ID_OpenProject,     MI(wxART_FOLDER_OPEN, wxColour(220,175, 50)), "Open");
        m_ocxToolBar->AddTool(wxID_SAVE,          MI(wxART_SAVE,        wxColour(  0,122,204)), "Save");
        m_ocxToolBar->AddSeparator();
        m_ocxToolBar->AddTool(ID_CompileHDL,      MI(wxART_BUILD,       wxColour(200,150,  0)), "Compile");
        m_ocxToolBar->AddTool(ID_RunSimulation,   MI(wxART_PLAY_ARROW,  wxColour( 60,180, 75)), "Run");
        m_ocxToolBar->AddTool(ID_DebugSimulation, MI(wxART_BUG_REPORT,  wxColour(230,115,  0)), "Debug");
        m_ocxToolBar->AddSeparator();
        m_ocxToolBar->AddTool(ID_UndoCanvas,      MI(wxART_UNDO,        clrNeutral),            "Undo");
        m_ocxToolBar->AddTool(ID_RedoCanvas,      MI(wxART_REDO,        clrNeutral),            "Redo");
        m_ocxToolBar->AddSeparator();
        m_ocxToolBar->AddTool(ID_Find,            MI(wxART_SEARCH,      clrNeutral),            "Find");
        m_ocxToolBar->AddTool(ID_Settings,        MI(wxART_SETTINGS,    clrNeutral),            "Settings");
    }

    //** Layout **//
    wxSplitterWindow* mainSplitter = new wxSplitterWindow(
        rootPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxSP_LIVE_UPDATE | wxBORDER_NONE);
#pragma warning(suppress: 4996)
    mainSplitter->SetSashSize(4);

    rightSplitter = new wxSplitterWindow(
        mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxSP_LIVE_UPDATE | wxBORDER_NONE);
#pragma warning(suppress: 4996)
    rightSplitter->SetSashSize(4);

    mainSplitter->SetBackgroundColour(OCXTheme::BgSash());
    rightSplitter->SetBackgroundColour(OCXTheme::BgSash());

    //** Left pane: project explorer (top) + outline panel (bottom) **//
    wxSplitterWindow* leftSplitter = new wxSplitterWindow(
        mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxSP_LIVE_UPDATE | wxBORDER_NONE);
#pragma warning(suppress: 4996)
    leftSplitter->SetSashSize(4);
    leftSplitter->SetBackgroundColour(OCXTheme::BgSash());

    projectExplorer = new wxTreeCtrl(
        leftSplitter, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxTR_DEFAULT_STYLE | wxBORDER_NONE);

    projectExplorer->SetBackgroundColour(OCXTheme::BgPanel());
    projectExplorer->SetForegroundColour(OCXTheme::FgText());

    wxSize iconSize(16, 16);
    wxImageList* imgList = new wxImageList(16, 16, true, 4);
    imgList->Add(wxMaterialDesignArtProvider::GetBitmap(wxART_FOLDER,          wxART_CLIENT_MATERIAL_FILLED, iconSize, wxColour(220,175, 50)));
    imgList->Add(wxMaterialDesignArtProvider::GetBitmap(wxART_CODE,            wxART_CLIENT_MATERIAL_FILLED, iconSize, wxColour( 86,156,214)));
    imgList->Add(wxMaterialDesignArtProvider::GetBitmap(wxART_DEVELOPER_BOARD, wxART_CLIENT_MATERIAL_FILLED, iconSize, wxColour( 78,201,176)));
    imgList->Add(wxMaterialDesignArtProvider::GetBitmap(wxART_ARTICLE,         wxART_CLIENT_MATERIAL_FILLED, iconSize, OCXTheme::FgText()));
    projectExplorer->AssignImageList(imgList);

    wxTreeItemId rootId = projectExplorer->AddRoot("Project Explorer",
                                                    TREEIMG_FOLDER, TREEIMG_FOLDER);
    projectExplorer->Expand(rootId);

    // Outline panel (symbol tree - below project explorer)
    outlinePanel = new OutlinePanel(leftSplitter);
    outlinePanel->SetBackgroundColour(OCXTheme::BgPanel());

    leftSplitter->SplitHorizontally(projectExplorer, outlinePanel, 300);
    leftSplitter->SetMinimumPaneSize(60);

    // Workspace notebook - tabs: HDL Editor | Waveform | RTL View | Circuit Canvas
    workspaceNotebook = new wxNotebook(rightSplitter, wxID_ANY);
    workspaceNotebook->SetBackgroundColour(OCXTheme::BgPanel());

    logicEditor = new LogicEditorPanel(workspaceNotebook);
    workspaceNotebook->AddPage(logicEditor, "HDL Editor");

    // Status bar field 1: line / col / file-type indicator
    logicEditor->SetCaretCallback([this](int line, int col, const wxString& ext)
    {
        int total = logicEditor->GetLineCount();
        wxString info = wxString::Format("Ln %d/%d, Col %d", line, total, col);
        if (!ext.IsEmpty())
            info += "  |  " + ext.Upper();
        OCXStatus(info, 1);
    });

    // Outline panel - re-parse whenever the active editor changes or is modified.
    // Also feed parsed symbol names into auto-complete.
    logicEditor->SetOutlineCallback([this](const wxString& code, const wxString& ext)
    {
        outlinePanel->UpdateOutline(code, ext);

        // Extract identifier names for auto-complete
        auto items = HdlParser::Parse(code, ext);
        wxArrayString names;
        names.Alloc(items.size());
        for (const auto& item : items)
            names.Add(item.name);
        logicEditor->SetAutoCompleteSymbols(names);
    });

    // Live VHDL syntax check on every explicit save
    logicEditor->SetSaveCallback([this](const wxString& filePath, const wxString& ext)
    {
        // Refresh RTL view (no external tools needed). Runs for every language,
        // not just VHDL - a Verilog file needs to reach ParseAndShow so the panel
        // can say why it can't render it instead of sitting blank.
        rtlViewPanel->ParseAndShow(logicEditor->GetCode(),
                                   logicEditor->GetCurrentFilePath());

        if (ext != "vhd" && ext != "vhdl")
            return;

        if (!circuitSimulator->IsGHDLAvailable())
            return;
        // No project open means no work library - skip to avoid spurious errors.
        if (currentProjectDirectory.IsEmpty())
            return;

        wxArrayString errors;
        bool ok = circuitSimulator->SyntaxCheckVHDL(filePath, errors);

        if (!ok)
        {
            outputPanel->ClearErrors();
            for (size_t i = 0; i < errors.GetCount(); ++i)
                outputPanel->LogError(errors[i]);
            outputPanel->ShowErrorTab();

            auto parsed = outputPanel->GetParsedErrors();
            std::vector<LogicEditorPanel::ErrorMark> marks;
            marks.reserve(parsed.size());
            for (const auto& pe : parsed)
            {
                LogicEditorPanel::ErrorMark m;
                m.filePath = pe.file;
                m.line     = pe.line;
                marks.push_back(m);
            }
            logicEditor->SetErrorDecorations(marks);
        }
        else
        {
            logicEditor->ClearErrorDecorations();
        }
    });

    // Outline panel jump → editor line
    outlinePanel->SetJumpCallback([this](int line)
    {
        workspaceNotebook->SetSelection(0);
        logicEditor->GotoLine(line);
    });

    wxPanel* canvasTab = new wxPanel(workspaceNotebook);
    canvasTab->SetBackgroundColour(OCXTheme::BgPanel());

    circuitCanvas = new CircuitCanvas(canvasTab);
    circuitCanvas->SetStatusCallback([this](const wxString& s) { OCXStatus(s); });
    circuitCanvas->SetSimVCDCallback([this](const wxString& vcdPath) {
        // Auto-load the recorded VCD in the Waveform tab
        waveformPanel->LoadVCD(vcdPath);
        workspaceNotebook->SetSelection(1); // switch to Waveform tab
        outputPanel->LogMessage("Canvas simulation recorded: " + vcdPath);
        // Reset the record button label
        canvasActionPanel->UpdateAnimButtons();
    });

    // Left: component placement palette (gates, combinational, sequential, ports)
    componentPalette = new ComponentPalette(canvasTab, circuitCanvas, logicEditor,
        [this]() { workspaceNotebook->SetSelection(0); });

    // Right: action panel (sim, animate, zoom, align, edit, canvas ops, export)
    canvasActionPanel = new CanvasActionPanel(canvasTab, circuitCanvas, logicEditor,
        [this]() { workspaceNotebook->SetSelection(0); });

    // Three-column layout: [Palette 160px] | [Canvas flex] | [Actions 160px]
    wxBoxSizer* cvSizer = new wxBoxSizer(wxHORIZONTAL);
    cvSizer->Add(componentPalette,  0, wxEXPAND);
    cvSizer->Add(circuitCanvas,     1, wxEXPAND);
    cvSizer->Add(canvasActionPanel, 0, wxEXPAND);
    canvasTab->SetSizer(cvSizer);

    waveformPanel = new WaveformPanel(workspaceNotebook);
    workspaceNotebook->AddPage(waveformPanel, "Waveform");

    rtlViewPanel = new RTLViewPanel(workspaceNotebook);
    workspaceNotebook->AddPage(rtlViewPanel, "RTL View");

    workspaceNotebook->AddPage(canvasTab, "Circuit Canvas");

    outputPanel = new OutputPanel(rightSplitter);

    //** Watch panel (debugger signal watch list) **//
    watchPanel = new WatchPanel(outputPanel);
    outputPanel->AddTab(watchPanel, "Watch");

    //** Error line-jump callback **//
    outputPanel->SetErrorJumpCallback(
        [this](const wxString& file, int line, int col)
        {
            wxString resolved = file;
            if (!wxFileExists(resolved) && !currentProjectDirectory.IsEmpty())
                resolved = currentProjectDirectory + "/" + file;
            if (wxFileExists(resolved))
            {
                workspaceNotebook->SetSelection(0); // switch to HDL Editor
                logicEditor->JumpToFileLine(resolved, line, col);
            }
        });

    //** Find Results panel jump callback **//
    outputPanel->GetFindResultsPanel()->SetJumpCallback(
        [this](const wxString& filePath, int line)
        {
            workspaceNotebook->SetSelection(0);
            logicEditor->JumpToFileLine(filePath, line);
        });

    //** Waveform cursor callback → watch panel update + cursor time tracking **//
    waveformPanel->SetCursorCallback(
        [this](long long cursorTime)
        {
            m_waveformCursorTime = cursorTime;
            watchPanel->UpdateValues(cursorTime, waveformPanel->GetTracks());
        });

    // >> hover over a signal name in the HDL editor shows its value at the current waveform cursor position.
    logicEditor->SetSignalValueCallback([this](const wxString& name) -> wxString
    {
        const std::vector<WfTrack>* tracks = waveformPanel->GetTracks();
        if (!tracks) return wxString();
        for (const WfTrack& tr : *tracks)
        {
            if (tr.signal.name.IsSameAs(name, false))
            {
                wxString val = tr.ValueAt(m_waveformCursorTime);
                if (val.IsEmpty()) return wxString();
                return name + " = " + val;
            }
        }
        return wxString();
    });

    //** Waveform "Add to Watch" callback (right-click or double-click signal) **//
    waveformPanel->SetWatchCallback(
        [this](const wxString& name)
        {
            watchPanel->AddSignal(name);
        });

    // Refresh RTL view whenever the user switches to that tab
    workspaceNotebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED,
        [this](wxBookCtrlEvent& e)
        {
            // Compare the page itself, not a hard-coded index - the index was
            // stale after Waveform was inserted ahead of RTL View, so the
            // refresh fired on the Circuit Canvas tab instead.
            int sel = e.GetSelection();
            if (sel >= 0 && sel < (int)workspaceNotebook->GetPageCount()
                && workspaceNotebook->GetPage(sel) == rtlViewPanel)
            {
                wxString code = logicEditor->GetCode();
                if (!code.IsEmpty())
                    rtlViewPanel->ParseAndShow(code,
                                               logicEditor->GetCurrentFilePath());
                else
                    rtlViewPanel->Clear();
            }
            e.Skip();
        });

    mainSplitter->SplitVertically(leftSplitter, rightSplitter, 250);
    rightSplitter->SplitHorizontally(workspaceNotebook, outputPanel, -200);

    mainSplitter->SetMinimumPaneSize(150);
    rightSplitter->SetMinimumPaneSize(80);

    //** Welcome panel (shown when no project is open) **//
    m_welcomePanel = new WelcomePanel(
        rightSplitter,
        [this]() { wxCommandEvent e; OnNewProject(e); },
        [this]() { wxCommandEvent e; OnOpenProject(e); },
        [this](const wxString& path) { OpenProjectFile(path); });
    m_welcomePanel->Hide();

    // Silent background version check - badges the Help menu and banners
    // the Welcome screen if a newer release is found. No-ops if offline.
    // weakThis guards against the window closing before the fetch returns.
    wxWeakRef<MainWindow> weakThis(this);
    OCXCheckForUpdatesAsync([weakThis](const wxString& newVersion) {
        if (weakThis) weakThis->OnUpdateAvailable(newVersion);
    });

    // Root panel layout: OCXMenuBar → OCXToolBar → splitter content
    //** Run Config Bar (between toolbar and main content) **//
    m_runConfigBar = new wxPanel(rootPanel, wxID_ANY,
                                 wxDefaultPosition, wxSize(-1, 32));
    m_runConfigBar->SetBackgroundColour(OCXTheme::BgPanel());
    {
        auto mkLbl = [&](const wxString& text) -> wxStaticText* {
            wxStaticText* l = new wxStaticText(m_runConfigBar, wxID_ANY, text);
            l->SetForegroundColour(OCXTheme::FgDim());
            return l;
        };

        m_entityField = new wxTextCtrl(m_runConfigBar, wxID_ANY, "",
                                       wxDefaultPosition, wxSize(180, 22),
                                       wxBORDER_SIMPLE | wxTE_PROCESS_ENTER);
        m_entityField->SetBackgroundColour(OCXTheme::BgEditor());
        m_entityField->SetForegroundColour(OCXTheme::FgText());
        m_entityField->SetHint("top entity / tb name");
        m_entityField->SetToolTip("Top-level entity to simulate (F6) or synthesize (F9). "
                                  "Usually your testbench, e.g. tb_counter.");

        m_stopTimeField = new wxTextCtrl(m_runConfigBar, wxID_ANY, "",
                                         wxDefaultPosition, wxSize(80, 22),
                                         wxBORDER_SIMPLE | wxTE_PROCESS_ENTER);
        m_stopTimeField->SetBackgroundColour(OCXTheme::BgEditor());
        m_stopTimeField->SetForegroundColour(OCXTheme::FgText());
        m_stopTimeField->SetHint("e.g. 500ns");
        m_stopTimeField->SetToolTip("Simulation stop time passed to GHDL --stop-time=");

        const wxString stds[] = { "08", "93", "19" };
        m_vhdlStdChoice = new wxChoice(m_runConfigBar, wxID_ANY,
                                       wxDefaultPosition, wxSize(52, 24),
                                       3, stds);
        m_vhdlStdChoice->SetSelection(0);  // default: 08
        m_vhdlStdChoice->SetBackgroundColour(OCXTheme::BgEditor());
        m_vhdlStdChoice->SetForegroundColour(OCXTheme::FgText());
        m_vhdlStdChoice->SetToolTip("VHDL standard for GHDL (--std=)");

        wxBoxSizer* rcSz = new wxBoxSizer(wxHORIZONTAL);
        rcSz->AddSpacer(10);
        rcSz->Add(mkLbl("Entity:"),    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        rcSz->Add(m_entityField,       0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
        rcSz->Add(mkLbl("Stop Time:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        rcSz->Add(m_stopTimeField,     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
        rcSz->Add(mkLbl("Std:"),       0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
        rcSz->Add(m_vhdlStdChoice,     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
        rcSz->Add(mkLbl("F5 = Compile   F6 = Run   F7 = Debug   F9 = FPGA Synth"),
                  0, wxALIGN_CENTER_VERTICAL);
        m_runConfigBar->SetSizer(rcSz);

        // Persist entity + stop time whenever user finishes editing
        m_entityField->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
            currentProject.simTopEntity = m_entityField->GetValue().Trim(true).Trim(false);
            currentProject.runTime      = m_stopTimeField->GetValue().Trim(true).Trim(false);
            circuitSimulator->SetRunTime(currentProject.runTime);
            SaveProjectState();
        });
        m_stopTimeField->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
            currentProject.simTopEntity = m_entityField->GetValue().Trim(true).Trim(false);
            currentProject.runTime      = m_stopTimeField->GetValue().Trim(true).Trim(false);
            circuitSimulator->SetRunTime(currentProject.runTime);
            SaveProjectState();
        });
        // Also persist on focus loss
        m_entityField->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
            currentProject.simTopEntity = m_entityField->GetValue().Trim(true).Trim(false);
            SaveProjectState(); e.Skip();
        });
        m_stopTimeField->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
            currentProject.runTime = m_stopTimeField->GetValue().Trim(true).Trim(false);
            circuitSimulator->SetRunTime(currentProject.runTime);
            SaveProjectState(); e.Skip();
        });
    }

    wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(m_ocxMenuBar,    0, wxEXPAND);
    rootSizer->Add(m_ocxToolBar,    0, wxEXPAND);
    rootSizer->Add(m_runConfigBar,  0, wxEXPAND);
    rootSizer->Add(mainSplitter,    1, wxEXPAND);
    rootPanel->SetSizer(rootSizer);

    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(rootPanel, 1, wxEXPAND);
    SetSizer(frameSizer);

    Centre();

    //** Plugins **//
    pluginManager = new PluginManager(outputPanel);
    wxString exeDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    pluginManager->LoadAll(exeDir + "/plugins");

    // Wire any menu items registered by plugins into the Tools menu.
    const auto& pluginItems = pluginManager->GetMenuItems();
    if (!pluginItems.empty())
    {
        m_toolsMenu->AppendSeparator();
        // Dynamic IDs start well above any static ID used above.
        int dynId = wxID_HIGHEST + 500;
        for (const PluginMenuItem& item : pluginItems)
        {
            m_toolsMenu->Append(dynId, item.label);
            // Capture by value so the lambda owns the data.
            Bind(wxEVT_MENU, [item](wxCommandEvent&) {
                if (item.callback)
                    item.callback(item.userData);
            }, dynId);
            ++dynId;
        }
    }

    //** Auto-save timer (interval from config, default 1 minute) **//
    {
        wxConfig cfg("OpenCircuitX");
        int mins = 1;
        cfg.Read("AutoSaveInterval", &mins, 1);
        if (mins < 1) mins = 1;
        m_autoSaveTimer.SetOwner(this, ID_AutoSaveTimer);
        m_autoSaveTimer.Start(mins * 60000);
    }

    //** Session restore: reopen files from the previous session **//
    {
        wxConfig cfg("OpenCircuitX");
        long count = 0;
        cfg.Read("SessionFileCount", &count, 0L);
        for (long i = 0; i < count; ++i)
        {
            wxString path;
            cfg.Read(wxString::Format("SessionFile%d", (int)i), &path, "");
            if (!path.IsEmpty() && wxFileExists(path))
                logicEditor->LoadFile(path);
        }
    }

    // Show welcome screen if no project was opened at startup.
    if (currentProjectDirectory.IsEmpty())
        ShowWelcomePanel();
}

//--
