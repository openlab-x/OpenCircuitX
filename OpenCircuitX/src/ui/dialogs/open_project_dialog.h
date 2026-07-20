#pragma once
#include <wx/wx.h>
#include <wx/filedlg.h>

class OpenProjectDialog : public wxDialog
{
public:
    OpenProjectDialog(wxWindow* parent);

    // Function to get the selected project file path
    wxString GetProjectFilePath() const;

private:
    void OnBrowse(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);

    wxTextCtrl* projectFileCtrl;  // Text field to show selected file path
    wxDECLARE_EVENT_TABLE();
};
