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

// Project explorer
//--
void MainWindow::OnProjectFileSelected(wxTreeEvent& event)
{
    if (event.GetEventObject() != projectExplorer)
        return;

    wxTreeItemId item = event.GetItem();
    if (!item.IsOk())
        return;

    // Category / folder nodes carry no data - clicking them just expands/collapses
    OCXFileItemData* data = dynamic_cast<OCXFileItemData*>(
                                projectExplorer->GetItemData(item));
    if (!data || data->filePath.IsEmpty())
        return;

    wxString filePath = data->filePath;
    if (wxFileExists(filePath))
    {
        workspaceNotebook->SetSelection(0); // HDL Editor tab
        logicEditor->LoadFile(filePath);
        currentProject.lastOpenedFile = wxFileName(filePath).GetFullName();
        outputPanel->LogMessage("Opened: " + filePath);
        OCXStatus("Editing: " + wxFileName(filePath).GetFullName());
    }
}

void MainWindow::OnProjectExplorerRightClick(wxTreeEvent& event)
{
    if (event.GetEventObject() != projectExplorer)
        return;
    if (currentProjectDirectory.IsEmpty())
        return;

    wxTreeItemId item = event.GetItem();
    projectExplorer->SelectItem(item);

    // Save the right-clicked item now - by the time the popup command fires
    // GetSelection() may no longer point to it (focus/selection gets reset).
    m_contextMenuItem = item;

    bool isFile = false;
    if (item.IsOk())
    {
        OCXFileItemData* data = dynamic_cast<OCXFileItemData*>(
            projectExplorer->GetItemData(item));
        isFile = (data != nullptr);
    }

    wxMenu menu;
    menu.Append(ID_NewFileInProject,   "New File");
    menu.Append(ID_NewFolderInProject, "New Folder");
    if (isFile)
    {
        menu.AppendSeparator();
        menu.Append(ID_RenameFile, "Rename...");
        menu.Append(ID_DeleteFile, "Delete");
    }
    PopupMenu(&menu);
}

void MainWindow::OnNewFileInProject(wxCommandEvent& event)
{
    if (currentProjectDirectory.IsEmpty())
    {
        wxMessageBox("No project open.", "New File", wxOK | wxICON_WARNING);
        return;
    }

    //** Template picker dialog **//
    wxDialog tplDlg(this, wxID_ANY, "New File",
                    wxDefaultPosition, wxSize(480, 340),
                    wxDEFAULT_DIALOG_STYLE);

    struct Tpl { wxString label; wxString ext; wxString description; };
    static const Tpl tpls[] = {
        { "VHDL Entity + Architecture",   "vhd",  "Entity declaration with empty port list and behavioral architecture" },
        { "VHDL Package",                  "vhd",  "Package declaration and body" },
        { "VHDL Testbench",                "vhd",  "Testbench skeleton with clk/reset process" },
        { "Verilog Module",                "v",    "Module skeleton with empty port list" },
        { "SystemVerilog Module",          "sv",   "SystemVerilog module with always_ff example" },
        { "iCE40 Constraints (PCF)",       "pcf",  "Pin constraint file for iCE40 FPGAs" },
        { "ECP5 Constraints (LPF)",        "lpf",  "Preference file for ECP5 FPGAs" },
        { "Blank File",                    "",     "Empty file (you choose the extension)" },
    };
    const int NTPLS = (int)(sizeof(tpls)/sizeof(tpls[0]));

    wxBoxSizer* tsz = new wxBoxSizer(wxVERTICAL);

    wxStaticText* lbl1 = new wxStaticText(&tplDlg, wxID_ANY, "Template:");
    tsz->Add(lbl1, 0, wxLEFT | wxTOP | wxRIGHT, 10);

    wxListBox* tplList = new wxListBox(&tplDlg, wxID_ANY,
                                       wxDefaultPosition, wxSize(-1, 140),
                                       0, nullptr, wxLB_SINGLE);
    for (int i = 0; i < NTPLS; ++i)
        tplList->Append(tpls[i].label);
    tplList->SetSelection(0);
    tsz->Add(tplList, 0, wxEXPAND | wxALL, 10);

    wxStaticText* descLbl = new wxStaticText(&tplDlg, wxID_ANY,
        tpls[0].description, wxDefaultPosition, wxSize(-1, 36), wxST_NO_AUTORESIZE);
    tsz->Add(descLbl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxBoxSizer* nameRow = new wxBoxSizer(wxHORIZONTAL);
    nameRow->Add(new wxStaticText(&tplDlg, wxID_ANY, "File name:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    wxTextCtrl* nameCtrl = new wxTextCtrl(&tplDlg, wxID_ANY, "design.vhd",
                                           wxDefaultPosition, wxSize(240, -1));
    nameRow->Add(nameCtrl, 1);
    tsz->Add(nameRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    wxBoxSizer* btnRow = new wxBoxSizer(wxHORIZONTAL);
    btnRow->AddStretchSpacer();
    btnRow->Add(new wxButton(&tplDlg, wxID_OK,     "Create"), 0, wxRIGHT, 6);
    btnRow->Add(new wxButton(&tplDlg, wxID_CANCEL, "Cancel"), 0);
    tsz->Add(btnRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    tplDlg.SetSizer(tsz);

    // Update description + default name when template selection changes
    tplList->Bind(wxEVT_LISTBOX, [&](wxCommandEvent&) {
        int sel = tplList->GetSelection();
        if (sel < 0 || sel >= NTPLS) return;
        descLbl->SetLabel(tpls[sel].description);
        if (!tpls[sel].ext.IsEmpty())
        {
            wxString cur = nameCtrl->GetValue();
            wxString stem = wxFileName(cur).GetName();
            if (stem.IsEmpty()) stem = "design";
            nameCtrl->SetValue(stem + "." + tpls[sel].ext);
        }
    });

    if (tplDlg.ShowModal() != wxID_OK) return;

    int tplIdx = tplList->GetSelection();
    if (tplIdx < 0) tplIdx = 0;

    wxString fileName = nameCtrl->GetValue().Trim(true).Trim(false);
    if (fileName.IsEmpty()) return;

    wxString filePath = currentProjectDirectory + "/" + fileName;
    if (wxFileExists(filePath))
    {
        wxMessageBox("A file with that name already exists.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxString stem = wxFileName(fileName).GetName();
    if (stem.IsEmpty()) stem = "design";
    wxString ext  = wxFileName(fileName).GetExt().Lower();

    wxString content;
    switch (tplIdx)
    {
    case 0: // VHDL Entity + Architecture
        content =
            "library IEEE;\n"
            "use IEEE.STD_LOGIC_1164.ALL;\n"
            "use IEEE.NUMERIC_STD.ALL;\n"
            "\n"
            "entity " + stem + " is\n"
            "    port (\n"
            "        clk  : in  std_logic;\n"
            "        rst  : in  std_logic\n"
            "    );\n"
            "end entity " + stem + ";\n"
            "\n"
            "architecture Behavioral of " + stem + " is\n"
            "begin\n"
            "\n"
            "end architecture Behavioral;\n";
        break;

    case 1: // VHDL Package
        content =
            "library IEEE;\n"
            "use IEEE.STD_LOGIC_1164.ALL;\n"
            "\n"
            "package " + stem + "_pkg is\n"
            "\n"
            "    -- Constants\n"
            "    -- Types\n"
            "    -- Function declarations\n"
            "\n"
            "end package " + stem + "_pkg;\n"
            "\n"
            "package body " + stem + "_pkg is\n"
            "\n"
            "end package body " + stem + "_pkg;\n";
        break;

    case 2: // VHDL Testbench
        content =
            "library IEEE;\n"
            "use IEEE.STD_LOGIC_1164.ALL;\n"
            "\n"
            "entity tb_" + stem + " is\n"
            "end tb_" + stem + ";\n"
            "\n"
            "architecture sim of tb_" + stem + " is\n"
            "\n"
            "    signal clk : std_logic := '0';\n"
            "    signal rst : std_logic := '1';\n"
            "\n"
            "    component " + stem + "\n"
            "        port (\n"
            "            clk : in std_logic;\n"
            "            rst : in std_logic\n"
            "        );\n"
            "    end component;\n"
            "\n"
            "begin\n"
            "\n"
            "    uut : " + stem + " port map (\n"
            "        clk => clk,\n"
            "        rst => rst\n"
            "    );\n"
            "\n"
            "    -- 10 ns clock\n"
            "    clk <= not clk after 5 ns;\n"
            "\n"
            "    stim : process\n"
            "    begin\n"
            "        rst <= '1';\n"
            "        wait for 20 ns;\n"
            "        rst <= '0';\n"
            "        wait for 100 ns;\n"
            "        wait;\n"
            "    end process;\n"
            "\n"
            "end architecture sim;\n";
        break;

    case 3: // Verilog Module
        content =
            "module " + stem + " (\n"
            "    input  wire clk,\n"
            "    input  wire rst\n"
            ");\n"
            "\n"
            "endmodule\n";
        break;

    case 4: // SystemVerilog Module
        content =
            "module " + stem + " (\n"
            "    input  logic clk,\n"
            "    input  logic rst\n"
            ");\n"
            "\n"
            "    always_ff @(posedge clk or posedge rst) begin\n"
            "        if (rst) begin\n"
            "            // reset logic\n"
            "        end else begin\n"
            "            // clocked logic\n"
            "        end\n"
            "    end\n"
            "\n"
            "endmodule\n";
        break;

    case 5: // PCF
        content =
            "# iCE40 Pin Constraints\n"
            "# set_io <signal_name> <pin>\n"
            "set_io clk  35\n"
            "set_io rst  38\n";
        break;

    case 6: // LPF
        content =
            "# ECP5 Lattice Preference File\n"
            "LOCATE COMP \"clk\" SITE \"P6\";\n"
            "IOBUF PORT \"clk\" IO_TYPE=LVCMOS33;\n"
            "FREQUENCY PORT \"clk\" 25 MHZ;\n";
        break;

    case 7: // Blank
    default:
        content = "";
        break;
    }

    wxFileOutputStream fileOut(filePath);
    if (!fileOut.IsOk())
    {
        wxMessageBox("Could not create file.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (!content.IsEmpty())
    {
        wxTextOutputStream textOut(fileOut);
        textOut << content;
    }

    currentProject.sourceFiles.Add(filePath);
    SaveProjectState();
    LoadProjectFiles(currentProjectDirectory);
    logicEditor->LoadFile(filePath);
    workspaceNotebook->SetSelection(0);
    outputPanel->LogMessage("Created: " + filePath);
}

void MainWindow::OnNewFolderInProject(wxCommandEvent& event)
{
    wxTextEntryDialog dlg(this, "Enter folder name:", "New Folder");

    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString folderName = dlg.GetValue();
    folderName.Trim(true);
    folderName.Trim(false);
    if (folderName.IsEmpty())
        return;

    wxString folderPath = currentProjectDirectory + "/" + folderName;
    if (wxDir::Exists(folderPath))
    {
        wxMessageBox("A folder with that name already exists.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxMkdir(folderPath);
    LoadProjectFiles(currentProjectDirectory);
    outputPanel->LogMessage("Created folder: " + folderPath);
}

void MainWindow::OnRenameFile(wxCommandEvent&)
{
    if (!m_contextMenuItem.IsOk()) return;
    OCXFileItemData* data = dynamic_cast<OCXFileItemData*>(
        projectExplorer->GetItemData(m_contextMenuItem));
    if (!data) return;

    wxString oldPath = data->filePath;
    wxString oldName = wxFileName(oldPath).GetFullName();

    wxTextEntryDialog dlg(this, "New file name:", "Rename File", oldName);
    if (dlg.ShowModal() != wxID_OK) return;

    wxString newName = dlg.GetValue().Trim(true).Trim(false);
    if (newName.IsEmpty() || newName == oldName) return;

    wxFileName newFn(oldPath);
    newFn.SetFullName(newName);
    wxString newPath = newFn.GetFullPath();
    if (wxFileExists(newPath))
    {
        wxMessageBox("A file with that name already exists.", "Rename",
                     wxOK | wxICON_ERROR);
        return;
    }

    if (!wxRenameFile(oldPath, newPath))
    {
        wxMessageBox("Could not rename file.", "Rename", wxOK | wxICON_ERROR);
        return;
    }

    // Update project source list (sourceFiles stores full paths)
    int idx = currentProject.sourceFiles.Index(oldPath);
    if (idx != wxNOT_FOUND)
    {
        currentProject.sourceFiles.RemoveAt(idx);
        currentProject.sourceFiles.Add(newPath);
        SaveProjectState();
    }

    LoadProjectFiles(currentProjectDirectory);
    outputPanel->LogMessage("Renamed: " + oldName + " -> " + newName);
}

void MainWindow::OnDeleteFile(wxCommandEvent&)
{
    if (!m_contextMenuItem.IsOk()) return;
    OCXFileItemData* data = dynamic_cast<OCXFileItemData*>(
        projectExplorer->GetItemData(m_contextMenuItem));
    if (!data) return;

    wxString path = data->filePath;
    wxString name = wxFileName(path).GetFullName();

    if (wxMessageBox("Delete \"" + name + "\"?\nThis cannot be undone.",
                     "Delete File", wxYES_NO | wxICON_WARNING) != wxYES)
        return;

    if (!wxRemoveFile(path))
    {
        wxMessageBox("Could not delete file.", "Delete", wxOK | wxICON_ERROR);
        return;
    }

    // Remove from project source list (sourceFiles stores full paths)
    currentProject.sourceFiles.Remove(path);
    SaveProjectState();

    LoadProjectFiles(currentProjectDirectory);
    outputPanel->LogMessage("Deleted: " + name);
}

//--