#pragma once

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/weakref.h>
#include <wx/timer.h>
#include <wx/treectrl.h>
#include <wx/splitter.h>
#include <wx/toolbar.h>
#include <wx/notebook.h>
#include <wx/docview.h>
#include "utils/project_file.h"
#include "ui/shell/ocx_toolbar.h"
#include "ui/shell/ocx_menubar.h"

class LogicEditorPanel;
class OutputPanel;
class CircuitSimulator;
class FPGAToolchain;
class PluginManager;
class CircuitCanvas;
class ComponentPalette;
class CanvasActionPanel;
class WaveformPanel;
class WatchPanel;
class OutlinePanel;
class WelcomePanel;
class RTLViewPanel;

//--
// Event IDs shared by the event table and implementation files
//--
enum
{
    ID_OpenProject       = wxID_HIGHEST + 1,
    ID_CompileHDL,
    ID_RunSimulation,
    ID_DebugSimulation,
    ID_SaveSimulation,
    ID_NewFileInProject,
    ID_NewFolderInProject,
    ID_Settings,
    ID_About,
    ID_CheckForUpdates,
    ID_Find,
    ID_ToggleLineNumbers,
    ID_ToggleCodeFolding,
    ID_LintVerilator,
    ID_RunVerilator,
    ID_ThemeDark,
    ID_ThemeMidnight,
    ID_ThemeLight,
    ID_FPGASynthesize,
    ID_FPGAPlaceRoute,
    ID_FPGAProgram,
    ID_Replace,
    ID_GotoLine,
    ID_GenTestbench,
    ID_RenameFile,
    ID_DeleteFile,
    ID_SnippetFirst,
    ID_SnippetLast = ID_SnippetFirst + 9,
    ID_FindInFiles,
    ID_EditorZoomIn,
    ID_EditorZoomOut,
    ID_EditorZoomReset,
    ID_ToggleComment,
    ID_ToggleWordWrap,
    ID_ToggleShowWhitespace,
    ID_ReopenTab,
    ID_ToggleBookmark,
    ID_NextBookmark,
    ID_PrevBookmark,
    ID_AutoSaveTimer,
    ID_GoToDefinition,
    ID_SymbolSearch,
    ID_ReplaceInFiles,
    ID_QuickOpen,
    ID_DuplicateLine,
    ID_SelectAllOccurrences,
    ID_JumpToMatchingBrace,
    ID_SortLines,
    ID_UndoCanvas,
    ID_RedoCanvas,
    ID_KeyboardShortcuts,
    ID_CanvasCopy,
    ID_CanvasPaste,
    ID_CanvasSelectAll,
    ID_CanvasDeleteSel,
    ID_CanvasAlignLeft,
    ID_CanvasAlignRight,
    ID_CanvasAlignTop,
    ID_CanvasAlignBottom,
    ID_CanvasAlignCenterH,
    ID_CanvasAlignCenterV,
    ID_CanvasExportPNG,
    ID_CanvasExportVerilog
};

enum TreeImg
{
    TREEIMG_FOLDER  = 0,
    TREEIMG_SOURCE  = 1,
    TREEIMG_PROJECT = 2,
    TREEIMG_GENERIC = 3
};

// Tree item data storing the full path for project-tree leaf nodes
class OCXFileItemData : public wxTreeItemData
{
public:
    wxString filePath;
    explicit OCXFileItemData(const wxString& path) : filePath(path) {}
};

class MainWindow : public wxFrame
{
public:
    MainWindow(const wxString& title);

    // Open a .ocxproj file directly - used by command-line arg handling.
    void OpenProjectFile(const wxString& projectFilePath);

private:
    void OnNewProject(wxCommandEvent& event);
    void OnOpenProject(wxCommandEvent& event);
    void OnSaveFile(wxCommandEvent& event);
    void OnSaveProject(wxCommandEvent& event);
    void OnSaveSimulation(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void OnCompileHDL(wxCommandEvent& event);
    void OnRunSimulation(wxCommandEvent& event);
    void OnDebugSimulation(wxCommandEvent& event);
    void OnProjectFileSelected(wxTreeEvent& event);
    void OnProjectExplorerRightClick(wxTreeEvent& event);
    void OnNewFileInProject(wxCommandEvent& event);
    void OnNewFolderInProject(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnCheckForUpdates(wxCommandEvent& event);
    void OnFind(wxCommandEvent& event);
    void OnReplace(wxCommandEvent& event);
    void OnGotoLine(wxCommandEvent& event);
    void OnRecentFile(wxCommandEvent& event);
    void OnGenTestbench(wxCommandEvent& event);
    void OnRenameFile(wxCommandEvent& event);
    void OnDeleteFile(wxCommandEvent& event);
    void OnInsertSnippet(wxCommandEvent& event);
    void OnFindInFiles(wxCommandEvent& event);
    void OnEditorZoomIn(wxCommandEvent& event);
    void OnEditorZoomOut(wxCommandEvent& event);
    void OnEditorZoomReset(wxCommandEvent& event);
    void OnToggleComment(wxCommandEvent& event);
    void OnToggleWordWrap(wxCommandEvent& event);
    void OnToggleShowWhitespace(wxCommandEvent& event);
    void OnReopenTab(wxCommandEvent& event);
    void OnToggleBookmark(wxCommandEvent& event);
    void OnNextBookmark(wxCommandEvent& event);
    void OnPrevBookmark(wxCommandEvent& event);
    void OnAutoSave(wxTimerEvent& event);
    void OnToggleLineNumbers(wxCommandEvent& event);
    void OnToggleCodeFolding(wxCommandEvent& event);
    void OnLintVerilator(wxCommandEvent& event);
    void OnRunVerilator(wxCommandEvent& event);
    void OnFPGASynthesize(wxCommandEvent& event);
    void OnFPGAPlaceRoute(wxCommandEvent& event);
    void OnFPGAProgram(wxCommandEvent& event);
    void OnThemeDark(wxCommandEvent& event);
    void OnThemeMidnight(wxCommandEvent& event);
    void OnThemeLight(wxCommandEvent& event);
    void OnGoToDefinition(wxCommandEvent& event);
    void OnSymbolSearch(wxCommandEvent& event);
    void OnReplaceInFiles(wxCommandEvent& event);
    void OnQuickOpen(wxCommandEvent& event);
    void OnDuplicateLine(wxCommandEvent& event);
    void OnSelectAllOccurrences(wxCommandEvent& event);
    void OnJumpToMatchingBrace(wxCommandEvent& event);
    void OnSortLines(wxCommandEvent& event);
    void OnUndoCanvas(wxCommandEvent& event);
    void OnRedoCanvas(wxCommandEvent& event);
    void OnKeyboardShortcuts(wxCommandEvent& event);
    void OnCanvasCopy(wxCommandEvent& event);
    void OnCanvasPaste(wxCommandEvent& event);
    void OnCanvasSelectAll(wxCommandEvent& event);
    void OnCanvasDeleteSel(wxCommandEvent& event);
    void OnCanvasAlign(wxCommandEvent& event);
    void OnCanvasExportPNG(wxCommandEvent& event);
    void OnCanvasExportVerilog(wxCommandEvent& event);

    // Re-apply the active theme colours to all top-level panels immediately.
    void ApplyThemeColors();

    // Wrapper: updates the status bar field and forces a full repaint so the
    // owner-drawn ThemeStatusBar redraws both fields correctly.
    void OCXStatus(const wxString& text, int field = 0);

    void LoadProjectFiles(const wxString& projectDir);
    void SaveProjectState();

    // Returns all .vhd/.vhdl files in the project directory, design files
    // first and testbench files last (GHDL dependency order).
    wxArrayString CollectProjectVHDLFiles() const;

    // Returns all .v/.sv files in the project directory.
    wxArrayString CollectProjectVerilogFiles() const;

    wxSplitterWindow* splitter;
    wxSplitterWindow* rightSplitter  = nullptr;
    wxTreeCtrl*       projectExplorer;
    wxNotebook*       workspaceNotebook;
    WelcomePanel*     m_welcomePanel = nullptr;
    LogicEditorPanel* logicEditor;
    CircuitCanvas*    circuitCanvas;
    ComponentPalette*  componentPalette;
    CanvasActionPanel* canvasActionPanel;
    WaveformPanel*    waveformPanel;
    RTLViewPanel*     rtlViewPanel   = nullptr;
    WatchPanel*       watchPanel;
    OutlinePanel*     outlinePanel;
    OutputPanel*      outputPanel;
    CircuitSimulator* circuitSimulator;
    FPGAToolchain*    fpgaToolchain;
    PluginManager*    pluginManager;

    wxString    currentProjectDirectory;
    wxString    currentProjectFilePath;
    ProjectFile currentProject;

    wxTimer  m_autoSaveTimer;

    OCXToolBar* m_ocxToolBar   = nullptr;
    OCXMenuBar* m_ocxMenuBar   = nullptr;

    wxMenu*  m_toolsMenu       = nullptr;   // kept for plugin menu item injection
    wxMenu*  m_recentMenu      = nullptr;   // Recent Projects sub-menu
    wxFileHistory* m_fileHistory = nullptr; // MRU list (up to 9 entries)
    wxMenuItem* m_checkUpdatesItem = nullptr; // relabeled once a newer version is found

    // Stores the tree item that was right-clicked; used by OnRenameFile /
    // OnDeleteFile because GetSelection() may return the wrong item by the
    // time the popup-menu command fires.
    wxTreeItemId m_contextMenuItem;

    bool      m_showLineNumbers     = true;
    bool      m_codeFolding         = false;
    wxString  m_fpgaBoardId         = "ibreaker";
    long long m_waveformCursorTime  = 0;

    // Run Config Bar
    wxPanel*   m_runConfigBar    = nullptr;
    wxTextCtrl* m_entityField    = nullptr;
    wxTextCtrl* m_stopTimeField  = nullptr;
    wxChoice*   m_vhdlStdChoice  = nullptr;

    void UpdateRunConfigBar();   // populate fields from currentProject
    void ShowWelcomePanel();
    void HideWelcomePanel();
    void OnUpdateAvailable(const wxString& newVersion); // startup check found a newer release

    wxDECLARE_EVENT_TABLE();
};
