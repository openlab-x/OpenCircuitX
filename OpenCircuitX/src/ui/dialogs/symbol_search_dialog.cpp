#include "symbol_search_dialog.h"
#include <wx/sizer.h>

//--
// SymbolSearchDialog
//--
SymbolSearchDialog::SymbolSearchDialog(wxWindow* parent,
                                       const std::vector<OutlineItem>& items,
                                       std::function<void(int)> jumpCb)
    : wxDialog(parent, wxID_ANY, "Symbol Search", wxDefaultPosition,
               wxSize(480, 420), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_jumpCb(jumpCb)
    , m_all(items)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_search = new wxTextCtrl(this, wxID_ANY, "",
                              wxDefaultPosition, wxDefaultSize,
                              wxTE_PROCESS_ENTER);
    sizer->Add(m_search, 0, wxEXPAND | wxALL, 6);

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_list->InsertColumn(0, "Symbol",   wxLIST_FORMAT_LEFT, 200);
    m_list->InsertColumn(1, "Kind",     wxLIST_FORMAT_LEFT, 100);
    m_list->InsertColumn(2, "Line",     wxLIST_FORMAT_RIGHT, 60);
    sizer->Add(m_list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Close"), 0, wxRIGHT, 6);
    sizer->Add(btnSizer, 0, wxEXPAND | wxBOTTOM, 6);

    SetSizer(sizer);

    Populate("");

    m_search->Bind(wxEVT_TEXT,         &SymbolSearchDialog::OnSearch,        this);
    m_search->Bind(wxEVT_TEXT_ENTER,   &SymbolSearchDialog::OnSearch,        this);
    m_search->Bind(wxEVT_KEY_DOWN,     &SymbolSearchDialog::OnKeyDown,       this);
    m_list  ->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                                       &SymbolSearchDialog::OnListActivated, this);
    m_list  ->Bind(wxEVT_KEY_DOWN,     &SymbolSearchDialog::OnKeyDown,       this);

    m_search->SetFocus();
}

//--
void SymbolSearchDialog::Populate(const wxString& filter)
{
    m_list->DeleteAllItems();
    m_indices.clear();

    wxString lo = filter.Lower();

    for (int i = 0; i < (int)m_all.size(); ++i)
    {
        const OutlineItem& it = m_all[i];
        if (!lo.IsEmpty() && !it.name.Lower().Contains(lo))
            continue;

        long row = m_list->InsertItem((long)m_indices.size(), it.name);
        m_list->SetItem(row, 1, it.kind);
        m_list->SetItem(row, 2, wxString::Format("%d", it.line));
        m_indices.push_back(i);
    }

    if (m_list->GetItemCount() > 0)
        m_list->SetItemState(0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                             wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
}

//--
void SymbolSearchDialog::OnSearch(wxCommandEvent&)
{
    Populate(m_search->GetValue());
}

//--
void SymbolSearchDialog::OnListActivated(wxListEvent& event)
{
    long row = event.GetIndex();
    if (row < 0 || row >= (long)m_indices.size()) return;

    int idx  = m_indices[(size_t)row];
    int line = m_all[(size_t)idx].line;
    if (m_jumpCb) m_jumpCb(line);
    EndModal(wxID_OK);
}

//--
void SymbolSearchDialog::OnKeyDown(wxKeyEvent& event)
{
    int key = event.GetKeyCode();

    if (key == WXK_RETURN || key == WXK_NUMPAD_ENTER)
    {
        long sel = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (sel >= 0 && sel < (long)m_indices.size())
        {
            int line = m_all[(size_t)m_indices[(size_t)sel]].line;
            if (m_jumpCb) m_jumpCb(line);
            EndModal(wxID_OK);
        }
        return;
    }

    // Arrow keys from search box move selection in the list
    if (event.GetEventObject() == m_search)
    {
        if (key == WXK_DOWN || key == WXK_UP)
        {
            long cur = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            long next = (key == WXK_DOWN) ? cur + 1 : cur - 1;
            if (next < 0) next = 0;
            if (next >= m_list->GetItemCount())
                next = m_list->GetItemCount() - 1;
            if (next >= 0)
            {
                if (cur >= 0)
                    m_list->SetItemState(cur, 0,
                                         wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                m_list->SetItemState(next,
                                     wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                                     wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                m_list->EnsureVisible(next);
            }
            return;
        }
    }

    if (key == WXK_ESCAPE)
    {
        EndModal(wxID_CANCEL);
        return;
    }

    event.Skip();
}
