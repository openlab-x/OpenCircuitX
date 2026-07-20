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
#include <wx/imaglist.h>
#include <wx/tokenzr.h>
#include <wx/numdlg.h>
#include <wx/dcbuffer.h>
#include <wx/renderer.h>
#include <wx/file.h>
#include <vector>

// Phase 17 - HDL Code Intelligence
//--
void MainWindow::OnGoToDefinition(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(0);

    wxString word = logicEditor->GetWordAtCursor();
    if (word.IsEmpty())
    {
        OCXStatus("Go to Definition: no identifier under cursor.");
        return;
    }

    // Search all open project files for a definition of this symbol.
    // We use HdlParser::Parse() on each file and look for a name match.
    wxArrayString openFiles = logicEditor->GetOpenFilePaths();

    // Also include all project files even if not open yet.
    if (!currentProjectDirectory.IsEmpty())
    {
        wxDir dir(currentProjectDirectory);
        wxString fn;
        bool ok = dir.GetFirst(&fn, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
        while (ok)
        {
            wxString full = currentProjectDirectory + "/" + fn;
            wxString ext  = wxFileName(fn).GetExt().Lower();
            if (ext == "vhd" || ext == "vhdl" || ext == "v" || ext == "sv")
                if (openFiles.Index(full) == wxNOT_FOUND)
                    openFiles.Add(full);
            ok = dir.GetNext(&fn);
        }
    }

    for (const wxString& filePath : openFiles)
    {
        if (filePath.IsEmpty()) continue;
        wxString ext = wxFileName(filePath).GetExt().Lower();

        // Get source: from editor if open, otherwise from disk
        wxString src;
        wxArrayString loaded = logicEditor->GetOpenFilePaths();
        int tabIdx = loaded.Index(filePath);
        if (tabIdx != wxNOT_FOUND)
        {
            // file is open - save current first so content is fresh on disk
        }

        // Read from disk
        wxFile f(filePath);
        if (!f.IsOpened()) continue;
        size_t sz = (size_t)f.Length();
        wxString buf;
        buf.reserve(sz);
        char tmp[4096];
        size_t bytesRead;
        while ((bytesRead = f.Read(tmp, sizeof(tmp))) > 0)
            buf += wxString::FromAscii(tmp, bytesRead);

        auto items = HdlParser::Parse(buf, ext);
        for (const OutlineItem& it : items)
        {
            if (it.name.IsSameAs(word, false))
            {
                // Jump to this file + line
                workspaceNotebook->SetSelection(0);
                logicEditor->JumpToFileLine(filePath, it.line);
                OCXStatus(wxString::Format("Go to Definition: %s (%s) in %s, line %d",
                    word, it.kind, wxFileName(filePath).GetFullName(), it.line));
                return;
            }
        }
    }

    OCXStatus("Go to Definition: definition of '" + word + "' not found.");
}

void MainWindow::OnSymbolSearch(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(0);

    wxString code = logicEditor->GetCode();
    wxString ext  = logicEditor->GetCurrentFileExt();

    auto items = HdlParser::Parse(code, ext);
    if (items.empty())
    {
        wxMessageBox("No symbols found in the current file.\n"
                     "Open a .vhd, .vhdl, .v, or .sv file first.",
                     "Symbol Search", wxOK | wxICON_INFORMATION, this);
        return;
    }

    SymbolSearchDialog dlg(this, items, [this](int line) {
        workspaceNotebook->SetSelection(0);
        logicEditor->GotoLine(line);
    });
    dlg.ShowModal();
}

void MainWindow::OnReplaceInFiles(wxCommandEvent&)
{
    if (currentProjectDirectory.IsEmpty())
    {
        wxMessageBox("No project open. Open or create a project first.",
                     "Replace in Files", wxOK | wxICON_INFORMATION, this);
        return;
    }

    // Build a simple dialog: Find + Replace fields + Replace All button.
    wxDialog dlg(this, wxID_ANY, "Replace in Files",
                 wxDefaultPosition, wxSize(500, 220),
                 wxDEFAULT_DIALOG_STYLE);

    wxBoxSizer* sz = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(&dlg, wxID_ANY, "Find:"),
              0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxTextCtrl* findCtrl = new wxTextCtrl(&dlg, wxID_ANY, "",
                                           wxDefaultPosition, wxSize(320, -1));
    row1->Add(findCtrl, 1, wxEXPAND);
    sz->Add(row1, 0, wxEXPAND | wxALL, 10);

    wxBoxSizer* row2 = new wxBoxSizer(wxHORIZONTAL);
    row2->Add(new wxStaticText(&dlg, wxID_ANY, "Replace:"),
              0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxTextCtrl* replCtrl = new wxTextCtrl(&dlg, wxID_ANY, "",
                                           wxDefaultPosition, wxSize(320, -1));
    row2->Add(replCtrl, 1, wxEXPAND);
    sz->Add(row2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // File filter row
    wxBoxSizer* row3 = new wxBoxSizer(wxHORIZONTAL);
    row3->Add(new wxStaticText(&dlg, wxID_ANY, "File types:"),
              0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxTextCtrl* extCtrl = new wxTextCtrl(&dlg, wxID_ANY, "vhd,vhdl,v,sv",
                                          wxDefaultPosition, wxSize(200, -1));
    row3->Add(extCtrl, 0);
    sz->Add(row3, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxBoxSizer* btnRow = new wxBoxSizer(wxHORIZONTAL);
    btnRow->AddStretchSpacer();
    wxButton* btnReplace = new wxButton(&dlg, wxID_OK,     "Replace All");
    wxButton* btnCancel  = new wxButton(&dlg, wxID_CANCEL, "Cancel");
    btnRow->Add(btnReplace, 0, wxRIGHT, 6);
    btnRow->Add(btnCancel,  0);
    sz->Add(btnRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    dlg.SetSizer(sz);

    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString needle  = findCtrl->GetValue();
    wxString replace = replCtrl->GetValue();
    wxString extList = extCtrl->GetValue();

    if (needle.IsEmpty())
    {
        wxMessageBox("Find text cannot be empty.", "Replace in Files", wxOK | wxICON_WARNING, this);
        return;
    }

    // Build set of allowed extensions
    wxArrayString allowedExts = wxSplit(extList, ',');
    for (wxString& e : allowedExts)
        e = e.Trim(true).Trim(false).Lower();

    // Walk project directory recursively (one level for now)
    wxArrayString filesToProcess;
    wxDir dir(currentProjectDirectory);
    wxString fn;
    bool ok = dir.GetFirst(&fn, wxEmptyString, wxDIR_FILES);
    while (ok)
    {
        wxString ext = wxFileName(fn).GetExt().Lower();
        if (allowedExts.Index(ext) != wxNOT_FOUND)
            filesToProcess.Add(currentProjectDirectory + "/" + fn);
        ok = dir.GetNext(&fn);
    }

    int totalReplacements = 0;
    int filesModified     = 0;

    for (const wxString& filePath : filesToProcess)
    {
        // Read file
        wxFile rFile(filePath, wxFile::read);
        if (!rFile.IsOpened()) continue;
        wxString content;
        content.reserve((size_t)rFile.Length());
        char buf[4096]; size_t n;
        while ((n = rFile.Read(buf, sizeof(buf))) > 0)
            content += wxString::FromAscii(buf, n);
        rFile.Close();

        if (!content.Contains(needle)) continue;

        // Count and replace
        int count = 0;
        wxString result;
        result.reserve(content.Length());
        size_t pos = 0;
        size_t nLen = needle.Length();
        while (pos < content.Length())
        {
            size_t found = content.find(needle, pos);
            if (found == wxString::npos)
            {
                result += content.Mid(pos);
                break;
            }
            result += content.Mid(pos, found - pos);
            result += replace;
            pos = found + nLen;
            ++count;
        }

        // Write back
        wxFile wFile(filePath, wxFile::write);
        if (!wFile.IsOpened()) continue;
        wFile.Write(result.ToUTF8(), result.ToUTF8().length());
        wFile.Close();

        totalReplacements += count;
        ++filesModified;

        // If the file is open in the editor, reload it
        wxArrayString openPaths = logicEditor->GetOpenFilePaths();
        if (openPaths.Index(filePath) != wxNOT_FOUND)
            logicEditor->LoadFile(filePath); // reopens / refreshes
    }

    wxString msg = wxString::Format(
        "Replaced %d occurrence(s) in %d file(s).",
        totalReplacements, filesModified);
    wxMessageBox(msg, "Replace in Files", wxOK | wxICON_INFORMATION, this);
    OCXStatus(msg);
}

//--
// Canvas clipboard / selection operations
//--
void MainWindow::OnCanvasCopy(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(3);
    circuitCanvas->CopySelected();
}

void MainWindow::OnCanvasPaste(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(3);
    circuitCanvas->PasteClipboard();
}

void MainWindow::OnCanvasSelectAll(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(3);
    circuitCanvas->SelectAll();
}

void MainWindow::OnCanvasDeleteSel(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(3);
    circuitCanvas->DeleteSelected();
}

// Handles all 6 alignment IDs (ID_CanvasAlignLeft … ID_CanvasAlignCenterV)
void MainWindow::OnCanvasAlign(wxCommandEvent& event)
{
    workspaceNotebook->SetSelection(3);
    int id = event.GetId();
    if      (id == ID_CanvasAlignLeft)    circuitCanvas->AlignLeft();
    else if (id == ID_CanvasAlignRight)   circuitCanvas->AlignRight();
    else if (id == ID_CanvasAlignTop)     circuitCanvas->AlignTop();
    else if (id == ID_CanvasAlignBottom)  circuitCanvas->AlignBottom();
    else if (id == ID_CanvasAlignCenterH) circuitCanvas->AlignCenterH();
    else if (id == ID_CanvasAlignCenterV) circuitCanvas->AlignCenterV();
}

//--
// Canvas export
//--
void MainWindow::OnCanvasExportPNG(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(3);

    wxFileDialog dlg(this, "Export Canvas as PNG", currentProjectDirectory, "circuit.png",
                     "PNG files (*.png)|*.png",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString path = dlg.GetPath();
    if (!circuitCanvas->ExportToPNG(path))
    {
        wxMessageBox("Failed to export PNG.", "Export", wxOK | wxICON_ERROR, this);
        return;
    }
    outputPanel->LogMessage("Canvas exported to: " + path);
    OCXStatus("Exported: " + wxFileName(path).GetFullName());
}

void MainWindow::OnCanvasExportVerilog(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(3);

    wxString verilog = circuitCanvas->ExportToVerilog();
    if (verilog.IsEmpty())
    {
        wxMessageBox("Nothing to export - canvas is empty.", "Export to Verilog",
                     wxOK | wxICON_WARNING, this);
        return;
    }

    wxFileDialog dlg(this, "Export Canvas to Verilog", currentProjectDirectory, "circuit.v",
                     "Verilog files (*.v)|*.v",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString path = dlg.GetPath();
    wxFile f(path, wxFile::write);
    if (!f.IsOpened())
    {
        wxMessageBox("Could not open file for writing.", "Export to Verilog",
                     wxOK | wxICON_ERROR, this);
        return;
    }
    f.Write(verilog.ToUTF8(), verilog.ToUTF8().length());
    f.Close();

    outputPanel->LogMessage("Verilog exported to: " + path);
    OCXStatus("Exported: " + wxFileName(path).GetFullName());

    // Offer to open in the editor
    if (wxMessageBox("Open in editor?", "Export to Verilog",
                     wxYES_NO | wxICON_QUESTION, this) == wxYES)
        logicEditor->LoadFile(path);
}
