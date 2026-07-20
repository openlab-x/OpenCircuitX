#pragma once
#include <wx/wx.h>
#include <wx/treectrl.h>
#include <functional>
#include <vector>
#include "core/hdl/hdl_parser.h"

//--
// OutlineItemData - attached to each wxTreeCtrl node to store line number
//--
class OutlineItemData : public wxTreeItemData
{
public:
    explicit OutlineItemData(int line) : m_line(line) {}
    int GetLine() const { return m_line; }
private:
    int m_line;
};

//--
// OutlinePanel
//--
class OutlinePanel : public wxPanel
{
public:
    OutlinePanel(wxWindow* parent);

    void ReapplyTheme();

    // Re-parse code and rebuild the tree.
    // ext should be lowercase: "vhd", "vhdl", "v", "sv"
    void UpdateOutline(const wxString& code, const wxString& ext);

    // Fired when the user activates (double-click / Enter) a node.
    // Receives the 1-based line number of the symbol.
    void SetJumpCallback(std::function<void(int)> cb) { m_jumpCb = cb; }

    // Re-expose HdlParser::Parse as a static - callers that already hold a
    // panel pointer can call this without an extra include.
    static std::vector<OutlineItem> Parse(const wxString& code, const wxString& ext)
    { return HdlParser::Parse(code, ext); }

private:
    wxTreeCtrl*              m_tree;
    std::function<void(int)> m_jumpCb;

    void OnItemActivated(wxTreeEvent& event);
};
