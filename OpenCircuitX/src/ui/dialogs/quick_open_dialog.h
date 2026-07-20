#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <functional>
#include <vector>

//--
// QuickOpenDialog - Ctrl+P fuzzy file picker (VSCode style)
//--
class QuickOpenDialog : public wxDialog
{
public:
    // files   : all candidate file paths (absolute)
    // openCb  : called with the chosen file path
    QuickOpenDialog(wxWindow* parent,
                    const wxArrayString& files,
                    std::function<void(const wxString&)> openCb);

private:
    void Populate(const wxString& filter);
    void Open(long row);

    void OnSearch(wxCommandEvent& event);
    void OnListActivated(wxListEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    wxTextCtrl*  m_search;
    wxListCtrl*  m_list;
    std::function<void(const wxString&)> m_openCb;

    wxArrayString m_all;       // full unfiltered paths
    wxArrayString m_filtered;  // paths shown in the list
};
