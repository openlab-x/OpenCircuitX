#include "workspace_tabs.h"
#include <wx/panel.h>

WorkspaceTabs::WorkspaceTabs(wxWindow* parent)
    : wxNotebook(parent, wxID_ANY)
{
    // Initially, the workspace will be empty.
}

void WorkspaceTabs::AddTab(const wxString& tabName)
{
    wxPanel* newTab = new wxPanel(this);
    AddPage(newTab, tabName, true);
}
