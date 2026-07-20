#include "project_explorer_panel.h"
#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/msgdlg.h>

enum
{
    ID_NEW_FILE = wxID_HIGHEST + 1,
    ID_NEW_FOLDER
};

wxBEGIN_EVENT_TABLE(ProjectExplorerPanel, wxPanel)
EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, ProjectExplorerPanel::OnItemRightClick)
EVT_MENU(ID_NEW_FILE, ProjectExplorerPanel::OnNewFile)
EVT_MENU(ID_NEW_FOLDER, ProjectExplorerPanel::OnNewFolder)
wxEND_EVENT_TABLE()

ProjectExplorerPanel::ProjectExplorerPanel(wxWindow* parent)
    : wxPanel(parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    projectExplorer = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(250, -1), wxTR_DEFAULT_STYLE);
    sizer->Add(projectExplorer, 1, wxEXPAND);

    SetSizer(sizer);

    contextMenu = new wxMenu;
    contextMenu->Append(ID_NEW_FILE, "New File");
    contextMenu->Append(ID_NEW_FOLDER, "New Folder");

    currentProjectDirectory = "";  // Set dynamically after project opens
}

// Handle right-click on an item
void ProjectExplorerPanel::OnItemRightClick(wxTreeEvent& event)
{
    PopupMenu(contextMenu);
}

// Create a new file
void ProjectExplorerPanel::OnNewFile(wxCommandEvent& event)
{
    if (currentProjectDirectory.IsEmpty())
    {
        wxMessageBox("No project is open.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxTextEntryDialog dlg(this, "Enter file name (e.g., new_file.vhdl, new_file.ocxcode):", "Create New File");
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString fileName = dlg.GetValue();
        wxString filePath = currentProjectDirectory + "/" + fileName;

        wxTextFile file(filePath);
        if (file.Create())
        {
            file.Write();
            file.Close();
            RefreshProjectExplorer();
        }
        else
        {
            wxMessageBox("File creation failed.", "Error", wxOK | wxICON_ERROR);
        }
    }
}

// Create a new folder
void ProjectExplorerPanel::OnNewFolder(wxCommandEvent& event)
{
    if (currentProjectDirectory.IsEmpty())
    {
        wxMessageBox("No project is open.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxTextEntryDialog dlg(this, "Enter folder name:", "Create New Folder");
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString folderName = dlg.GetValue();
        wxString folderPath = currentProjectDirectory + "/" + folderName;

        if (!wxDir::Exists(folderPath))
        {
            wxMkdir(folderPath);
            RefreshProjectExplorer();
        }
        else
        {
            wxMessageBox("Folder already exists.", "Error", wxOK | wxICON_ERROR);
        }
    }
}

// Refresh project explorer
void ProjectExplorerPanel::RefreshProjectExplorer()
{
    wxTreeItemId rootId = projectExplorer->GetRootItem();
    projectExplorer->DeleteChildren(rootId);

    wxDir dir(currentProjectDirectory);
    if (!dir.IsOpened()) return;

    wxString filename;
    bool cont = dir.GetFirst(&filename);
    while (cont)
    {
        projectExplorer->AppendItem(rootId, filename);
        cont = dir.GetNext(&filename);
    }

    projectExplorer->Expand(rootId);
}
