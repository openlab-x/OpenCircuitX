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

// VHDL file collection (design files first, testbenches last - GHDL order)
//--
static bool IsTestbenchName(const wxString& filename)
{
    wxString stem = wxFileName(filename).GetName().Lower();
    return stem.StartsWith("tb_") || stem.EndsWith("_tb") ||
           stem.EndsWith("_testbench") || stem == "testbench";
}

wxArrayString MainWindow::CollectProjectVHDLFiles() const
{
    wxArrayString sources, testbenches;
    if (currentProjectDirectory.IsEmpty())
        return sources;

    //--
    // Priority 1: explicit analyzeOrder from project file
    //   (user arranged files in the exact order GHDL needs them)
    //--
    if (!currentProject.analyzeOrder.IsEmpty())
    {
        wxStringTokenizer tok(currentProject.analyzeOrder, ",");
        while (tok.HasMoreTokens())
        {
            wxString f = tok.GetNextToken();
            f.Trim(true); f.Trim(false);
            if (f.IsEmpty()) continue;
            if (wxFileName(f).IsRelative())
                f = currentProjectDirectory + "/" + f;
            if (wxFileExists(f))
                sources.Add(f);
        }
        if (!sources.IsEmpty())
            return sources;
    }

    //--
    // Priority 2: sourceFiles list from project file
    //   (preserves the order the user added files, design first/tb last)
    //--
    if (!currentProject.sourceFiles.IsEmpty())
    {
        for (size_t i = 0; i < currentProject.sourceFiles.GetCount(); ++i)
        {
            wxString f = currentProject.sourceFiles[i];
            // Normalize: resolve mixed separators and double backslashes
            // that wxFileConfig may leave after reading the INI file.
            {
                wxFileName fn(f);
                fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE,
                             currentProjectDirectory);
                if (!fn.GetFullPath().IsEmpty())
                    f = fn.GetFullPath();
            }
            if (wxFileName(f).IsRelative())
                f = currentProjectDirectory + wxFILE_SEP_PATH + f;
            wxString ext = wxFileName(f).GetExt().Lower();
            if ((ext == "vhd" || ext == "vhdl") && wxFileExists(f))
            {
                // Same path can appear twice with different separators, and
                // analyzing a VHDL file twice re-declares its design units.
                bool seen = false;
                for (size_t j = 0; j < sources.GetCount() && !seen; ++j)
                    seen = sources[j].IsSameAs(f, false);
                for (size_t j = 0; j < testbenches.GetCount() && !seen; ++j)
                    seen = testbenches[j].IsSameAs(f, false);
                if (seen) continue;

                if (IsTestbenchName(f))
                    testbenches.Add(f);
                else
                    sources.Add(f);
            }
        }
        // Only trust the project list if it includes at least one design file.
        // If the list only has testbench files the design units are missing - 
        // fall through to the directory scan so they can be found.
        if (!sources.IsEmpty())
        {
            for (size_t i = 0; i < testbenches.GetCount(); ++i)
                sources.Add(testbenches[i]);
            return sources;
        }
    }

    //--
    // Priority 3: filesystem scan (no project open or project has no files listed)
    //--
    wxDir dir(currentProjectDirectory);
    if (!dir.IsOpened())
        return sources;

    wxString filename;
    bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
    while (cont)
    {
        wxString ext = wxFileName(filename).GetExt().Lower();
        if (ext == "vhd" || ext == "vhdl")
        {
            wxString full = currentProjectDirectory + "/" + filename;
            if (IsTestbenchName(filename))
                testbenches.Add(full);
            else
                sources.Add(full);
        }
        cont = dir.GetNext(&filename);
    }

    for (size_t i = 0; i < testbenches.GetCount(); ++i)
        sources.Add(testbenches[i]);

    return sources;
}

wxArrayString MainWindow::CollectProjectVerilogFiles() const
{
    wxArrayString files;
    if (currentProjectDirectory.IsEmpty())
        return files;

    // A project file can list the same path twice with different separators
    // (e.g. "dir\name.v" and "dir/name.v"), so compare normalized paths
    // rather than raw strings - handing iverilog the same file twice fails
    // with a duplicate module declaration.
    auto addUnique = [this, &files](const wxString& path)
    {
        wxFileName fn(path);
        fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE,
                     currentProjectDirectory);
        wxString norm = fn.GetFullPath();
        if (norm.IsEmpty()) norm = path;

        for (size_t i = 0; i < files.GetCount(); ++i)
            if (files[i].IsSameAs(norm, false))   // case-insensitive: Windows
                return;
        files.Add(norm);
    };

    // sourceFiles list first (preserves user order)
    if (!currentProject.sourceFiles.IsEmpty())
    {
        for (size_t i = 0; i < currentProject.sourceFiles.GetCount(); ++i)
        {
            wxString f = currentProject.sourceFiles[i];
            if (wxFileName(f).IsRelative())
                f = currentProjectDirectory + wxFILE_SEP_PATH + f;
            wxString ext = wxFileName(f).GetExt().Lower();
            if ((ext == "v" || ext == "sv") && wxFileExists(f))
                addUnique(f);
        }
        if (!files.IsEmpty())
            return files;
    }

    // Fallback: scan project directory
    wxDir dir(currentProjectDirectory);
    if (!dir.IsOpened())
        return files;

    wxString filename;
    bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
    while (cont)
    {
        wxString ext = wxFileName(filename).GetExt().Lower();
        if (ext == "v" || ext == "sv")
            addUnique(currentProjectDirectory + wxFILE_SEP_PATH + filename);
        cont = dir.GetNext(&filename);
    }

    return files;
}

//--
// Project explorer loading
//--
void MainWindow::LoadProjectFiles(const wxString& projectDir)
{
    wxTreeItemId rootId = projectExplorer->GetRootItem();
    projectExplorer->DeleteChildren(rootId);

    // Show project name as root label
    wxString rootLabel = currentProject.projectName.IsEmpty()
                       ? "Project Explorer"
                       : currentProject.projectName;
    projectExplorer->SetItemText(rootId, rootLabel);

    wxDir dir(projectDir);
    if (!dir.IsOpened())
    {
        outputPanel->LogMessage("Error: Could not open project directory: " + projectDir);
        return;
    }

    // Category nodes (created up front, deleted later if empty)
    wxTreeItemId idSrc  = projectExplorer->AppendItem(rootId, "Sources",          TREEIMG_FOLDER, TREEIMG_FOLDER);
    wxTreeItemId idTB   = projectExplorer->AppendItem(rootId, "Testbenches",      TREEIMG_FOLDER, TREEIMG_FOLDER);
    wxTreeItemId idSim  = projectExplorer->AppendItem(rootId, "Simulation",       TREEIMG_FOLDER, TREEIMG_FOLDER);
    wxTreeItemId idProj = projectExplorer->AppendItem(rootId, "Project Files",    TREEIMG_FOLDER, TREEIMG_FOLDER);
    wxTreeItemId idOth  = projectExplorer->AppendItem(rootId, "Other",            TREEIMG_FOLDER, TREEIMG_FOLDER);

    // Iterate directory entries (files only - subdirs get their own node in Other)
    wxString filename;
    bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
    while (cont)
    {
        wxString fullPath = projectDir + "/" + filename;

        if (wxDirExists(fullPath))
        {
            // Subdirectory - show as a non-clickable folder in Other
            projectExplorer->AppendItem(idOth, filename, TREEIMG_FOLDER, TREEIMG_FOLDER);
            cont = dir.GetNext(&filename);
            continue;
        }

        wxString ext  = wxFileName(filename).GetExt().Lower();
        bool isHDL    = (ext == "vhd" || ext == "vhdl" || ext == "v" || ext == "sv" || ext == "ocxcode");
        bool isProj   = (ext == "ocxproj" || ext == "ocxschem" || ext == "ocxconst" ||
                         ext == "ocxlib"  || ext == "ocxwork"  || ext == "ocxnet"   ||
                         ext == "ocxtempl");
        bool isSim    = (ext == "vcd" || ext == "vvp" || ext == "ocxwave" || ext == "ocxsim");
        bool isTB     = isHDL && IsTestbenchName(filename);

        int img = TREEIMG_GENERIC;
        if (isHDL)  img = TREEIMG_SOURCE;
        if (isProj) img = TREEIMG_PROJECT;

        wxTreeItemId parent;
        if      (isHDL && isTB) parent = idTB;
        else if (isHDL)         parent = idSrc;
        else if (isSim)         parent = idSim;
        else if (isProj)        parent = idProj;
        else                    parent = idOth;

        projectExplorer->AppendItem(parent, filename, img, img,
                                    new OCXFileItemData(fullPath));
        cont = dir.GetNext(&filename);
    }

    // Remove empty categories, expand non-empty ones
    auto tidy = [&](wxTreeItemId id)
    {
        if (!projectExplorer->ItemHasChildren(id))
            projectExplorer->Delete(id);
        else
            projectExplorer->Expand(id);
    };
    tidy(idSrc);
    tidy(idTB);
    tidy(idSim);
    tidy(idProj);
    tidy(idOth);

    projectExplorer->Expand(rootId);
    outputPanel->LogMessage("Loaded project files from: " + projectDir);
}

//--
void MainWindow::OnExit(wxCommandEvent& event)
{
    SaveProjectState();

    //** Session restore: save open file paths **//
    wxConfig cfg("OpenCircuitX");
    wxArrayString openPaths = logicEditor->GetOpenFilePaths();
    cfg.Write("SessionFileCount", (long)openPaths.GetCount());
    for (int i = 0; i < (int)openPaths.GetCount(); ++i)
        cfg.Write(wxString::Format("SessionFile%d", i), openPaths[i]);

    Close(true);
}

//--
// Help
//--
void MainWindow::OnAbout(wxCommandEvent& event)
{
    AboutDialog dlg(this);
    dlg.ShowModal();
}

void MainWindow::OnCheckForUpdates(wxCommandEvent& event)
{
    UpdateCheckerDialog dlg(this);
    dlg.ShowModal();
}

void MainWindow::OnUpdateAvailable(const wxString& newVersion)
{
    if (m_checkUpdatesItem)
        m_checkUpdatesItem->SetItemLabel(
            wxString::Format("&Check for Updates... (v%s available)", newVersion));

    // Menu bar badge, top right, always visible regardless of whether a
    // project is open or the Welcome screen is showing.
    if (m_ocxMenuBar)
        m_ocxMenuBar->SetUpdateAvailable(newVersion);
}

void MainWindow::OnFind(wxCommandEvent& event)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->ShowFindBar();
}

void MainWindow::OnReplace(wxCommandEvent& event)
{
    workspaceNotebook->SetSelection(0);
    logicEditor->ShowReplaceBar();
}

void MainWindow::OnGotoLine(wxCommandEvent& event)
{
    workspaceNotebook->SetSelection(0);
    long line = wxGetNumberFromUser(
        "Enter line number:", "Line:", "Go to Line",
        1, 1, 999999, this);
    if (line >= 1)
        logicEditor->GotoLine((int)line);
}

//--
// Find in Files
//--
void MainWindow::OnFindInFiles(wxCommandEvent& /*event*/)
{
    if (currentProjectDirectory.IsEmpty())
    {
        wxMessageBox("No project open. Open a project first.",
                     "Find in Files", wxOK | wxICON_WARNING);
        return;
    }

    wxString term = wxGetTextFromUser(
        "Search for:", "Find in Files", "", this);
    if (term.IsEmpty()) return;

    wxArrayString files = currentProject.sourceFiles;
    if (files.IsEmpty())
    {
        wxDir::GetAllFiles(currentProjectDirectory, &files, "*.vhd",  wxDIR_FILES);
        wxDir::GetAllFiles(currentProjectDirectory, &files, "*.vhdl", wxDIR_FILES);
        wxDir::GetAllFiles(currentProjectDirectory, &files, "*.v",    wxDIR_FILES);
        wxDir::GetAllFiles(currentProjectDirectory, &files, "*.sv",   wxDIR_FILES);
    }

    std::vector<FindResultsPanel::Result> results;

    for (const wxString& filePath : files)
    {
        wxTextFile tf;
        if (!tf.Open(filePath)) continue;

        int lineNum = 0;
        for (wxString ln = tf.GetFirstLine(); !tf.Eof(); ln = tf.GetNextLine())
        {
            ++lineNum;
            if (ln.Lower().Contains(term.Lower()))
            {
                FindResultsPanel::Result r;
                r.filePath = filePath;
                r.line     = lineNum;
                r.text     = ln.Trim(false).Trim(true);
                results.push_back(r);
            }
        }
        tf.Close();
    }

    wxString header = wxString::Format(
        "Find in Files: \"%s\" - %d result(s) in %d file(s)",
        term, (int)results.size(), (int)files.GetCount());

    outputPanel->GetFindResultsPanel()->SetResults(header, results);
    outputPanel->ShowFindResultsTab();

    OCXStatus(wxString::Format("Find in Files: %d result(s) for \"%s\"",
                                   (int)results.size(), term));
}

//--