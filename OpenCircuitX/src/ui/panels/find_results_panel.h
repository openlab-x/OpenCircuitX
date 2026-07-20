#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <functional>
#include <vector>

//--
// FindResultsPanel - tabbed results panel for Find in Files
//--
class FindResultsPanel : public wxPanel
{
public:
    struct Result
    {
        wxString filePath;   // absolute path
        int      line;       // 1-based
        wxString text;       // line content (trimmed)
    };

    explicit FindResultsPanel(wxWindow* parent);

    // Replace displayed results; header is shown above the list (e.g. search term + count).
    void SetResults(const wxString& header,
                    const std::vector<Result>& results);

    void Clear();
    void ReapplyTheme();

    // Called when the user double-clicks a result row.
    // Receives the absolute file path and 1-based line number.
    void SetJumpCallback(std::function<void(const wxString&, int)> cb);

private:
    wxStaticText*                        m_header;
    wxListCtrl*                          m_list;
    std::vector<Result>                  m_results;
    std::function<void(const wxString&, int)> m_jumpCb;

    void OnActivated(wxListEvent& event);
};
