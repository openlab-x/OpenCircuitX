#include "outline_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/sizer.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>

//--
// OutlinePanel - constructor
//--
OutlinePanel::OutlinePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxBORDER_NONE);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_tree, 1, wxEXPAND);
    SetSizer(sizer);

    m_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &OutlinePanel::OnItemActivated, this);
}

void OutlinePanel::ReapplyTheme()
{
    SetBackgroundColour(OCXTheme::BgPanel());
    m_tree->SetBackgroundColour(OCXTheme::BgPanel());
    m_tree->SetForegroundColour(OCXTheme::FgText());
    Refresh();
}

//--
// UpdateOutline - rebuild the wxTreeCtrl
//--
void OutlinePanel::UpdateOutline(const wxString& code, const wxString& ext)
{
    m_tree->DeleteAllItems();
    wxTreeItemId root = m_tree->AddRoot("Root");

    auto items = Parse(code, ext);
    if (items.empty())
        return;

    // Map: kind prefix labels
    auto kindLabel = [](const wxString& kind) -> wxString
    {
        if (kind == "entity")    return "[E]";
        if (kind == "arch")      return "[A]";
        if (kind == "module")    return "[M]";
        if (kind == "port_in"  || kind == "input")  return "->";
        if (kind == "port_out" || kind == "output") return "<-";
        if (kind == "port_inout"|| kind == "inout") return "<>";
        if (kind == "port")      return "--";
        if (kind == "signal")    return "~";
        if (kind == "process" || kind == "always" || kind == "initial") return "#";
        if (kind == "wire")      return "w";
        if (kind == "reg")       return "r";
        if (kind == "assign")    return "=";
        return "?";
    };

    // Top-level kinds: entity, arch, module
    // Everything else becomes a child of the last top-level node
    wxTreeItemId currentParent = root;

    for (const OutlineItem& item : items)
    {
        bool isTop = (item.kind == "entity" || item.kind == "arch" || item.kind == "module");
        wxString label = wxString::Format("%s %s  (line %d)",
                                          kindLabel(item.kind), item.name, item.line);

        wxTreeItemId id;
        if (isTop)
        {
            id = m_tree->AppendItem(root, label, -1, -1,
                                    new OutlineItemData(item.line));
            currentParent = id;
        }
        else
        {
            id = m_tree->AppendItem(currentParent, label, -1, -1,
                                    new OutlineItemData(item.line));
        }
    }

    // Expand all top-level nodes
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(root, cookie);
    while (child.IsOk())
    {
        m_tree->Expand(child);
        child = m_tree->GetNextChild(root, cookie);
    }
}

//--
// OnItemActivated - fire jump callback
//--
void OutlinePanel::OnItemActivated(wxTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    if (!id.IsOk()) return;

    OutlineItemData* data = dynamic_cast<OutlineItemData*>(m_tree->GetItemData(id));
    if (data && m_jumpCb)
        m_jumpCb(data->GetLine());
}
