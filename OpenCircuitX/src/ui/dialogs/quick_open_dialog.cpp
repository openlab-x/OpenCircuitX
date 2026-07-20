#include "quick_open_dialog.h"
#include <wx/filename.h>
#include <wx/sizer.h>

QuickOpenDialog::QuickOpenDialog(wxWindow* parent,
                                 const wxArrayString& files,
                                 std::function<void(const wxString&)> openCb)
    : wxDialog(parent, wxID_ANY, "Quick Open",
               wxDefaultPosition, wxSize(520, 440),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_openCb(openCb)
    , m_all(files)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_search = new wxTextCtrl(this, wxID_ANY, "",
                              wxDefaultPosition, wxDefaultSize,
                              wxTE_PROCESS_ENTER);
    sizer->Add(m_search, 0, wxEXPAND | wxALL, 6);

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER | wxBORDER_SIMPLE);
    m_list->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 180);
    m_list->InsertColumn(1, "Path", wxLIST_FORMAT_LEFT, 300);
    sizer->Add(m_list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

    wxBoxSizer* btnRow = new wxBoxSizer(wxHORIZONTAL);
    btnRow->AddStretchSpacer();
    btnRow->Add(new wxButton(this, wxID_CANCEL, "Close"), 0, wxRIGHT, 6);
    sizer->Add(btnRow, 0, wxEXPAND | wxBOTTOM, 6);

    SetSizer(sizer);

    Populate("");

    m_search->Bind(wxEVT_TEXT,       &QuickOpenDialog::OnSearch,       this);
    m_search->Bind(wxEVT_KEY_DOWN,   &QuickOpenDialog::OnKeyDown,      this);
    m_list  ->Bind(wxEVT_LIST_ITEM_ACTIVATED, &QuickOpenDialog::OnListActivated, this);
    m_list  ->Bind(wxEVT_KEY_DOWN,   &QuickOpenDialog::OnKeyDown,      this);

    m_search->SetFocus();
}

void QuickOpenDialog::Populate(const wxString& filter)
{
    m_list->DeleteAllItems();
    m_filtered.Clear();

    wxString lo = filter.Lower();

    for (const wxString& path : m_all)
    {
        wxFileName fn(path);
        wxString name = fn.GetFullName();
        // Fuzzy: filter matches anywhere in name OR relative path
        if (!lo.IsEmpty() && !name.Lower().Contains(lo) && !path.Lower().Contains(lo))
            continue;

        long row = m_list->InsertItem((long)m_filtered.GetCount(), name);
        m_list->SetItem(row, 1, fn.GetPath());
        m_filtered.Add(path);
    }

    if (m_list->GetItemCount() > 0)
        m_list->SetItemState(0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                             wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
}

void QuickOpenDialog::Open(long row)
{
    if (row < 0 || row >= (long)m_filtered.GetCount()) return;
    if (m_openCb) m_openCb(m_filtered[(size_t)row]);
    EndModal(wxID_OK);
}

void QuickOpenDialog::OnSearch(wxCommandEvent&)
{
    Populate(m_search->GetValue());
}

void QuickOpenDialog::OnListActivated(wxListEvent& event)
{
    Open(event.GetIndex());
}

void QuickOpenDialog::OnKeyDown(wxKeyEvent& event)
{
    int key = event.GetKeyCode();

    if (key == WXK_RETURN || key == WXK_NUMPAD_ENTER)
    {
        long sel = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        Open(sel);
        return;
    }

    if (key == WXK_ESCAPE)
    {
        EndModal(wxID_CANCEL);
        return;
    }

    // Arrow keys from search box move the list selection
    if (event.GetEventObject() == m_search && (key == WXK_DOWN || key == WXK_UP))
    {
        long cur  = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        long next = (key == WXK_DOWN) ? cur + 1 : cur - 1;
        next = wxMax(0L, wxMin(next, m_list->GetItemCount() - 1));
        if (next >= 0)
        {
            if (cur >= 0)
                m_list->SetItemState(cur, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
            m_list->SetItemState(next,
                                 wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                                 wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
            m_list->EnsureVisible(next);
        }
        return;
    }

    event.Skip();
}
