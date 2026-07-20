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
#include "ui/panels/outline_panel.h"
#include "ui/dialogs/symbol_search_dialog.h"
#include "ui/dialogs/quick_open_dialog.h"
#include "ui/panels/find_results_panel.h"
#include "core/simulation/circuit_simulator.h"
#include "core/toolchain/fpga_toolchain.h"
#include "core/plugin/plugin_manager.h"
#include "utils/project_file.h"
#include "ui/shell/app_theme.h"
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
#include <wx/tokenzr.h>
#include <wx/numdlg.h>
#include <wx/dcbuffer.h>
#include <wx/renderer.h>
#include <wx/file.h>
#include <vector>

//--
// Editor ops - zoom, bookmarks, autosave, recent, line numbers, Verilator, undo/redo
//--

void MainWindow::OnEditorZoomIn(wxCommandEvent& /*event*/)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->EditorZoomIn();
}

void MainWindow::OnEditorZoomOut(wxCommandEvent& /*event*/)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->EditorZoomOut();
}

void MainWindow::OnEditorZoomReset(wxCommandEvent& /*event*/)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->EditorZoomReset();
}

void MainWindow::OnToggleComment(wxCommandEvent& /*event*/)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->ToggleLineComment();
}

void MainWindow::OnToggleWordWrap(wxCommandEvent& event)
{
    bool enable = event.IsChecked();
    logicEditor->SetWordWrap(enable);
}

void MainWindow::OnToggleShowWhitespace(wxCommandEvent& event)
{
    logicEditor->SetShowWhitespace(event.IsChecked());
}

void MainWindow::OnReopenTab(wxCommandEvent& /*event*/)
{
    workspaceNotebook->SetSelection(0);
    if (!logicEditor->ReopenLastClosedTab())
        OCXStatus("No closed tabs to reopen.");
}

void MainWindow::OnToggleBookmark(wxCommandEvent& /*event*/)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->ToggleBookmark();
}

void MainWindow::OnNextBookmark(wxCommandEvent& /*event*/)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->GotoNextBookmark();
}

void MainWindow::OnPrevBookmark(wxCommandEvent& /*event*/)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->GotoPrevBookmark();
}

void MainWindow::OnAutoSave(wxTimerEvent& /*event*/)
{
    if (currentProjectDirectory.IsEmpty()) return;
    int n = logicEditor->SaveAllModified();
    if (n > 0)
        OCXStatus(wxString::Format("Auto-saved %d file(s)", n));
}

void MainWindow::OnRecentFile(wxCommandEvent& event)
{
    if (!m_fileHistory)
        return;
    wxString path = m_fileHistory->GetHistoryFile(
                        event.GetId() - wxID_FILE1);
    if (!path.IsEmpty() && wxFileExists(path))
        OpenProjectFile(path);
    else
        wxMessageBox("File not found:\n" + path, "Recent Projects",
                     wxOK | wxICON_WARNING);
}

void MainWindow::OnToggleLineNumbers(wxCommandEvent& event)
{
    m_showLineNumbers = event.IsChecked();
    logicEditor->SetLineNumbers(m_showLineNumbers);
}

void MainWindow::OnToggleCodeFolding(wxCommandEvent& event)
{
    m_codeFolding = event.IsChecked();
    logicEditor->SetCodeFolding(m_codeFolding);
}

//--
// Verilator
//--
void MainWindow::OnLintVerilator(wxCommandEvent&)
{
    wxString filePath = logicEditor->GetCurrentFilePath();
    if (filePath.IsEmpty())
    {
        outputPanel->LogMessage("No file is open. Open a Verilog file first.");
        return;
    }

    if (!circuitSimulator->IsVerilatorAvailable())
    {
        outputPanel->LogError("Verilator not found. Go to Tools > Settings to set the Verilator path.");
        outputPanel->ShowErrorTab();
        return;
    }

    logicEditor->SaveCurrentFile();
    circuitSimulator->SetWorkDir(currentProjectDirectory);

    outputPanel->ClearOutput();
    outputPanel->ClearErrors();
    outputPanel->ShowOutputTab();
    outputPanel->LogMessage("Linting: " + filePath);

    wxArrayString output, errors;
    bool ok = circuitSimulator->LintVerilator(filePath, output, errors);

    for (size_t i = 0; i < output.GetCount(); ++i) outputPanel->LogMessage(output[i]);
    for (size_t i = 0; i < errors.GetCount(); ++i) outputPanel->LogError(errors[i]);

    if (!ok) { outputPanel->ShowErrorTab(); OCXStatus("Verilator lint: issues found."); }
    else        OCXStatus("Verilator lint passed.");
}

void MainWindow::OnRunVerilator(wxCommandEvent&)
{
    wxString filePath = logicEditor->GetCurrentFilePath();
    if (filePath.IsEmpty())
    {
        outputPanel->LogMessage("No file is open. Open a Verilog file first.");
        return;
    }

    if (!circuitSimulator->IsVerilatorAvailable())
    {
        outputPanel->LogError("Verilator not found. Go to Tools > Settings to set the Verilator path.");
        outputPanel->ShowErrorTab();
        return;
    }

    logicEditor->SaveCurrentFile();
    circuitSimulator->SetWorkDir(currentProjectDirectory);

    outputPanel->ClearOutput();
    outputPanel->ClearErrors();
    outputPanel->ShowOutputTab();
    outputPanel->LogMessage("Running Verilator simulation: " + filePath);

    wxArrayString output, errors;
    bool ok = circuitSimulator->RunVerilator(filePath, output, errors);

    for (size_t i = 0; i < output.GetCount(); ++i) outputPanel->LogMessage(output[i]);
    for (size_t i = 0; i < errors.GetCount(); ++i) outputPanel->LogError(errors[i]);

    if (!ok) { outputPanel->ShowErrorTab(); OCXStatus("Verilator simulation failed."); }
    else        OCXStatus("Verilator simulation complete.");
}

//--
// Undo / Redo
//--
void MainWindow::OnUndoCanvas(wxCommandEvent&)
{
    if (workspaceNotebook->GetSelection() == 1)
        circuitCanvas->UndoCanvas();
    else
        logicEditor->Undo();
}

void MainWindow::OnRedoCanvas(wxCommandEvent&)
{
    if (workspaceNotebook->GetSelection() == 1)
        circuitCanvas->RedoCanvas();
    else
        logicEditor->Redo();
}

//--
// Keyboard shortcuts dialog
//--
void MainWindow::OnKeyboardShortcuts(wxCommandEvent&)
{
    wxDialog dlg(this, wxID_ANY, "Keyboard Shortcuts",
                 wxDefaultPosition, wxSize(620, 560),
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    wxListCtrl* list = new wxListCtrl(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    list->InsertColumn(0, "Shortcut",    wxLIST_FORMAT_LEFT, 180);
    list->InsertColumn(1, "Action",      wxLIST_FORMAT_LEFT, 380);

    struct Row { const char* key; const char* desc; };
    static const Row rows[] = {
        // File
        { "Ctrl+N",          "New Project" },
        { "Ctrl+O",          "Open Project" },
        { "Ctrl+P",          "Quick Open (fuzzy file picker)" },
        { "Ctrl+S",          "Save Current File" },
        { "Ctrl+Shift+T",    "Reopen Closed Tab" },
        { "Alt+F4",          "Exit" },
        // Edit
        { "Ctrl+Z",          "Undo (editor / canvas)" },
        { "Ctrl+Y",          "Redo (editor / canvas)" },
        { "Ctrl+/",          "Toggle Line Comment" },
        { "Ctrl+D",          "Duplicate Line / Selection" },
        { "Ctrl+Shift+L",    "Select All Occurrences" },
        { "Ctrl+]",          "Jump to Matching Brace" },
        { "Alt+Up",          "Move Line Up" },
        { "Alt+Down",        "Move Line Down" },
        { "Ctrl+Shift+F",    "Find in Files" },
        { "Ctrl+Shift+H",    "Replace in Files" },
        // View
        { "Ctrl+F",          "Find in Editor" },
        { "Ctrl+H",          "Find & Replace in Editor" },
        { "Ctrl+G",          "Go to Line" },
        { "F12",             "Go to Definition" },
        { "Ctrl+Shift+O",    "Symbol Search" },
        { "Ctrl+=",          "Editor Zoom In" },
        { "Ctrl+-",          "Editor Zoom Out" },
        { "Ctrl+0",          "Editor Zoom Reset" },
        { "Ctrl+B",          "Toggle Bookmark" },
        { "F2",              "Next Bookmark" },
        { "Shift+F2",        "Previous Bookmark" },
        // Tools
        { "F5",              "Compile HDL" },
        { "F6",              "Run Simulation" },
        { "F7",              "Debug Simulation" },
        { "F8",              "Lint with Verilator" },
        { "Shift+F8",        "Run with Verilator" },
        { "F9",              "FPGA Synthesize (Yosys)" },
        { "Shift+F9",        "FPGA Place & Route (nextpnr)" },
        { "Ctrl+F9",         "FPGA Program Board" },
        { "Ctrl+T",          "Generate Testbench" },
        // Canvas
        { "Ctrl+Shift+F",    "Canvas Zoom to Fit" },
        { "Del",             "Delete Selected Gate(s)" },
        { "Ctrl+A",          "Select All Gates" },
        { "Ctrl+C",          "Copy Selected Gates" },
        { "Ctrl+V",          "Paste Gates" },
        // Help
        { "Ctrl+Shift+?",    "Keyboard Shortcuts (this dialog)" },
    };

    for (int i = 0; i < (int)(sizeof(rows)/sizeof(rows[0])); ++i)
    {
        long row = list->InsertItem(i, wxString::FromAscii(rows[i].key));
        list->SetItem(row, 1, wxString::FromAscii(rows[i].desc));
    }

    wxBoxSizer* sz = new wxBoxSizer(wxVERTICAL);
    sz->Add(list, 1, wxEXPAND | wxALL, 8);
    wxButton* btn = new wxButton(&dlg, wxID_OK, "Close");
    sz->Add(btn, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 8);
    dlg.SetSizer(sz);

    dlg.ShowModal();
}

//--
// Editor power features - Quick Open, Duplicate Line, Occurrences, Brace, Sort
//--
void MainWindow::OnQuickOpen(wxCommandEvent&)
{
    wxArrayString files;

    // Collect files from the project directory (one level for now + recursive via wxDir)
    if (!currentProjectDirectory.IsEmpty())
    {
        wxDir::GetAllFiles(currentProjectDirectory, &files);
    }

    // Also include any editor tabs not associated with the project directory
    for (const wxString& p : logicEditor->GetOpenFilePaths())
        if (!p.IsEmpty() && files.Index(p) == wxNOT_FOUND)
            files.Add(p);

    // Filter to source-like files only
    wxArrayString sourceFiles;
    for (const wxString& f : files)
    {
        wxString ext = wxFileName(f).GetExt().Lower();
        if (ext == "vhd"  || ext == "vhdl" || ext == "v"   || ext == "sv"  ||
            ext == "pcf"  || ext == "lpf"  || ext == "xdc" || ext == "tcl" ||
            ext == "txt"  || ext == "md")
            sourceFiles.Add(f);
    }

    if (sourceFiles.IsEmpty())
    {
        wxMessageBox("No project files found. Open or create a project first.",
                     "Quick Open", wxOK | wxICON_INFORMATION, this);
        return;
    }

    QuickOpenDialog dlg(this, sourceFiles, [this](const wxString& path) {
        workspaceNotebook->SetSelection(0);
        logicEditor->LoadFile(path);
    });
    dlg.ShowModal();
}

void MainWindow::OnDuplicateLine(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->DuplicateLine();
}

void MainWindow::OnSelectAllOccurrences(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->SelectAllOccurrences();
}

void MainWindow::OnJumpToMatchingBrace(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->JumpToMatchingBrace();
}

void MainWindow::OnSortLines(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->SortSelectedLines();
}
