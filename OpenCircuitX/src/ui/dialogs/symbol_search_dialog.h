#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "core/hdl/hdl_parser.h"
#include <vector>
#include <functional>

//--
// SymbolSearchDialog - Ctrl+Shift+O quick-jump to any symbol
//--
class SymbolSearchDialog : public wxDialog
{
public:
    // items   : all symbols from the current file (via OutlinePanel::Parse)
    // jumpCb  : called with the 1-based line number when user picks a symbol
    SymbolSearchDialog(wxWindow* parent,
                       const std::vector<OutlineItem>& items,
                       std::function<void(int)> jumpCb);

private:
    void OnSearch(wxCommandEvent& event);
    void OnListActivated(wxListEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void Populate(const wxString& filter);

    wxTextCtrl*  m_search;
    wxListCtrl*  m_list;
    std::function<void(int)> m_jumpCb;

    std::vector<OutlineItem> m_all;     // full unfiltered list
    std::vector<int>         m_indices; // index into m_all for each list row
};
