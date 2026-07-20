#include "watch_panel.h"
#include "ui/shell/app_theme.h"

enum
{
    ID_WP_Add    = wxID_HIGHEST + 500,
    ID_WP_Remove,
    ID_WP_Clear
};

// Column indices
enum { COL_NAME = 0, COL_VALUE, COL_WIDTH };

WatchPanel::WatchPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    //** Toolbar row **//
    m_bar = new wxPanel(this, wxID_ANY);
    wxPanel* bar = m_bar;
    bar->SetBackgroundColour(OCXTheme::BgPanel());

    wxButton* btnAdd    = new wxButton(bar, ID_WP_Add,    "Add...",  wxDefaultPosition, wxSize(64, 24));
    wxButton* btnRemove = new wxButton(bar, ID_WP_Remove, "Remove",  wxDefaultPosition, wxSize(64, 24));
    wxButton* btnClear  = new wxButton(bar, ID_WP_Clear,  "Clear",   wxDefaultPosition, wxSize(56, 24));

    for (wxButton* b : { btnAdd, btnRemove, btnClear })
    {
        b->SetBackgroundColour(OCXTheme::BgSash());
        b->SetForegroundColour(OCXTheme::FgText());
    }

    wxBoxSizer* barSizer = new wxBoxSizer(wxHORIZONTAL);
    barSizer->AddSpacer(4);
    barSizer->Add(btnAdd,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    barSizer->Add(btnRemove, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    barSizer->Add(btnClear,  0, wxALIGN_CENTER_VERTICAL);
    bar->SetSizer(barSizer);

    //** List **//
    m_list = new wxListCtrl(this, wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_HRULES | wxLC_VRULES |
                            wxBORDER_NONE);
    m_list->SetBackgroundColour(OCXTheme::BgOutput());
    m_list->SetForegroundColour(OCXTheme::FgText());
    m_list->SetTextColour(OCXTheme::FgText());

    m_list->InsertColumn(COL_NAME,  "Signal", wxLIST_FORMAT_LEFT, 160);
    m_list->InsertColumn(COL_VALUE, "Value",  wxLIST_FORMAT_LEFT, 120);
    m_list->InsertColumn(COL_WIDTH, "Width",  wxLIST_FORMAT_LEFT,  60);

    //** Root sizer **//
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    root->Add(bar,    0, wxEXPAND | wxBOTTOM, 2);
    root->Add(m_list, 1, wxEXPAND);
    SetSizer(root);

    // Events
    Bind(wxEVT_BUTTON, &WatchPanel::OnAdd,    this, ID_WP_Add);
    Bind(wxEVT_BUTTON, &WatchPanel::OnRemove, this, ID_WP_Remove);
    Bind(wxEVT_BUTTON, &WatchPanel::OnClear,  this, ID_WP_Clear);
}

void WatchPanel::ReapplyTheme()
{
    SetBackgroundColour(OCXTheme::BgPanel());
    m_bar->SetBackgroundColour(OCXTheme::BgPanel());
    for (wxWindow* child : m_bar->GetChildren())
    {
        child->SetBackgroundColour(OCXTheme::BgSash());
        child->SetForegroundColour(OCXTheme::FgText());
        child->Refresh();
    }
    m_list->SetBackgroundColour(OCXTheme::BgOutput());
    m_list->SetForegroundColour(OCXTheme::FgText());
    m_list->SetTextColour(OCXTheme::FgText());
    Refresh();
}

void WatchPanel::AddSignal(const wxString& name)
{
    // Avoid duplicates
    for (long i = 0; i < m_list->GetItemCount(); ++i)
    {
        if (m_list->GetItemText(i, COL_NAME) == name)
            return;
    }

    long idx = m_list->InsertItem((long)m_list->GetItemCount(), name);
    m_list->SetItem(idx, COL_VALUE, "--");
    m_list->SetItem(idx, COL_WIDTH, "?");
}

void WatchPanel::UpdateValues(long long cursorTime,
                               const std::vector<WfTrack>* tracks)
{
    for (long i = 0; i < m_list->GetItemCount(); ++i)
    {
        wxString sigName = m_list->GetItemText(i, COL_NAME);
        wxString val     = "--";
        wxString width   = "?";

        if (tracks)
        {
            for (const WfTrack& t : *tracks)
            {
                if (t.signal.name == sigName)
                {
                    val   = t.ValueAt(cursorTime);
                    width = wxString::Format("%d", t.signal.width);
                    break;
                }
            }
        }

        m_list->SetItem(i, COL_VALUE, val);
        m_list->SetItem(i, COL_WIDTH, width);
    }
}

void WatchPanel::OnAdd(wxCommandEvent&)
{
    wxTextEntryDialog dlg(this,
        "Enter signal name to watch\n(must match the name in the VCD file):",
        "Add Signal Watch");
    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString name = dlg.GetValue();
    name.Trim(true);
    name.Trim(false);
    if (!name.IsEmpty())
        AddSignal(name);
}

void WatchPanel::OnRemove(wxCommandEvent&)
{
    long sel = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel != wxNOT_FOUND)
        m_list->DeleteItem(sel);
}

void WatchPanel::OnClear(wxCommandEvent&)
{
    m_list->DeleteAllItems();
}
