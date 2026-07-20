#pragma once
#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/menu.h>  // For right-click menu

class ProjectExplorerPanel : public wxPanel
{
public:
    ProjectExplorerPanel(wxWindow* parent);

private:
    wxTreeCtrl* projectExplorer;
    wxMenu* contextMenu;
    wxString currentProjectDirectory;

    void OnItemRightClick(wxTreeEvent& event);
    void OnNewFile(wxCommandEvent& event);
    void OnNewFolder(wxCommandEvent& event);
    void RefreshProjectExplorer();

    wxDECLARE_EVENT_TABLE();
};
