#pragma once
#include <wx/notebook.h>

class WorkspaceTabs : public wxNotebook
{
public:
    WorkspaceTabs(wxWindow* parent);

    void AddTab(const wxString& tabName); // Adds a new tab to the workspace.
};
