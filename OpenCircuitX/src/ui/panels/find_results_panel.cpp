#include "find_results_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/sizer.h>
#include <wx/filename.h>

FindResultsPanel::FindResultsPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(OCXTheme::BgOutput());

    m_header = new wxStaticText(this, wxID_ANY, "No search performed.");
    m_header->SetForegroundColour(OCXTheme::FgDim());

    wxFont hf = m_header->GetFont();
    hf.SetPointSize(hf.GetPointSize() - 1);
    m_header->SetFont(hf);

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);

    m_list->SetBackgroundColour(OCXTheme::BgOutput());
    m_list->SetTextColour(OCXTheme::FgText());

    m_list->InsertColumn(0, "File",  wxLIST_FORMAT_LEFT, 180);
    m_list->InsertColumn(1, "Line",  wxLIST_FORMAT_RIGHT, 52);
    m_list->InsertColumn(2, "Text",  wxLIST_FORMAT_LEFT, 560);

    wxBoxSizer* sz = new wxBoxSizer(wxVERTICAL);
    sz->Add(m_header, 0, wxEXPAND | wxALL, 4);
    sz->Add(m_list,   1, wxEXPAND);
    SetSizer(sz);

    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &FindResultsPanel::OnActivated, this);
}

void FindResultsPanel::ReapplyTheme()
{
    SetBackgroundColour(OCXTheme::BgOutput());
    m_header->SetForegroundColour(OCXTheme::FgDim());
    m_list->SetBackgroundColour(OCXTheme::BgOutput());
    m_list->SetTextColour(OCXTheme::FgText());
    Refresh();
}

void FindResultsPanel::SetResults(const wxString& header,
                                  const std::vector<Result>& results)
{
    m_results = results;
    m_header->SetLabel(header);

    m_list->DeleteAllItems();
    for (int i = 0; i < (int)results.size(); ++i)
    {
        const Result& r = results[i];
        long row = m_list->InsertItem(i, wxFileName(r.filePath).GetFullName());
        m_list->SetItem(row, 1, wxString::Format("%d", r.line));
        m_list->SetItem(row, 2, r.text);
    }
}

void FindResultsPanel::Clear()
{
    m_results.clear();
    m_list->DeleteAllItems();
    m_header->SetLabel("No search performed.");
}

void FindResultsPanel::SetJumpCallback(std::function<void(const wxString&, int)> cb)
{
    m_jumpCb = cb;
}

void FindResultsPanel::OnActivated(wxListEvent& event)
{
    long row = event.GetIndex();
    if (row < 0 || row >= (long)m_results.size()) return;
    if (m_jumpCb)
        m_jumpCb(m_results[(size_t)row].filePath, m_results[(size_t)row].line);
}
