#include "rtl_view_schematic_p.h"
#include <wx/filename.h>

namespace {
enum {
    ID_ZOOM_IN  = wxID_HIGHEST + 400,
    ID_ZOOM_OUT,
    ID_ZOOM_FIT,
    ID_ZOOM_RST,
    ID_EXPORT,
};
}

//** RTLViewPanel : construction **//
RTLViewPanel::RTLViewPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxBORDER_NONE)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    //** splitter **//
    m_splitter = new wxSplitterWindow(this, wxID_ANY,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxSP_THIN_SASH | wxSP_LIVE_UPDATE |
                                      wxBORDER_NONE);
    m_splitter->SetBackgroundColour(OCXTheme::BgPanel());
    m_splitter->SetMinimumPaneSize(80);

    //** left: tree **//
    m_tree = new wxTreeCtrl(m_splitter, wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT |
                            wxTR_FULL_ROW_HIGHLIGHT | wxBORDER_NONE |
                            wxTR_HIDE_ROOT);
    m_tree->SetBackgroundColour(OCXTheme::BgEditor());
    m_tree->SetForegroundColour(OCXTheme::FgText());
    m_tree->Bind(wxEVT_TREE_SEL_CHANGED,      &RTLViewPanel::OnTreeSelect, this);
    m_tree->Bind(wxEVT_TREE_BEGIN_LABEL_EDIT, [](wxTreeEvent& e){ e.Veto(); });
    m_tree->Bind(wxEVT_CONTEXT_MENU,          [](wxContextMenuEvent&){});

    //** right: toolbar + schematic **//
    wxPanel* rightPane = new wxPanel(m_splitter, wxID_ANY);
    rightPane->SetBackgroundColour(OCXTheme::BgPanel());

    wxPanel* toolbar = new wxPanel(rightPane, wxID_ANY, wxDefaultPosition,
                                   wxSize(-1, 28));
    toolbar->SetBackgroundColour(wxColour(30, 32, 38));

    auto mkBtn = [&](wxWindow* par, const wxString& label, int id) {
        auto* b = new wxButton(par, id, label, wxDefaultPosition,
                               wxSize(-1, 24), wxBORDER_NONE);
        b->SetBackgroundColour(wxColour(50, 52, 60));
        b->SetForegroundColour(OCXTheme::FgText());
        return b;
    };

    wxButton* btnOut = mkBtn(toolbar, "  -  ", ID_ZOOM_OUT);
    wxButton* btnIn  = mkBtn(toolbar, "  +  ", ID_ZOOM_IN);
    wxButton* btnFit = mkBtn(toolbar, " Fit ", ID_ZOOM_FIT);
    wxButton* btnExp = mkBtn(toolbar, " Export PNG ", ID_EXPORT);

    m_zoomLabel = new wxStaticText(toolbar, wxID_ANY, "100%",
                                   wxDefaultPosition, wxSize(48, -1),
                                   wxALIGN_CENTRE_HORIZONTAL);
    m_zoomLabel->SetForegroundColour(OCXTheme::FgText());

    m_infoLabel = new wxStaticText(toolbar, wxID_ANY, "Click a gate to inspect",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxST_ELLIPSIZE_END);
    m_infoLabel->SetForegroundColour(OCXTheme::FgDim());

    wxBoxSizer* tbSizer = new wxBoxSizer(wxHORIZONTAL);
    tbSizer->AddSpacer(6);
    tbSizer->Add(btnOut,      0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 2);
    tbSizer->Add(m_zoomLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 4);
    tbSizer->Add(btnIn,       0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 2);
    tbSizer->AddSpacer(6);
    tbSizer->Add(btnFit,      0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 2);
    tbSizer->AddSpacer(12);
    tbSizer->Add(m_infoLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 4);
    tbSizer->Add(btnExp,      0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 2);
    tbSizer->AddSpacer(6);
    toolbar->SetSizer(tbSizer);

    m_schem = new Schematic(rightPane, this);

    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    rightSizer->Add(toolbar,  0, wxEXPAND);
    rightSizer->Add(m_schem, 1, wxEXPAND);
    rightPane->SetSizer(rightSizer);

    m_splitter->SplitVertically(m_tree, rightPane, 200);

    wxBoxSizer* outer = new wxBoxSizer(wxHORIZONTAL);
    outer->Add(m_splitter, 1, wxEXPAND);
    SetSizer(outer);

    //** toolbar event bindings **//
    btnOut->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ m_schem->ZoomOut();     UpdateInfoLabel(); });
    btnIn ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ m_schem->ZoomIn();      UpdateInfoLabel(); });
    btnFit->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ m_schem->FitToWindow(); UpdateInfoLabel(); });
    btnExp->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ m_schem->ExportPNG(); });
}

//** RTLViewPanel : public API **//
void RTLViewPanel::ParseAndShow(const wxString& vhdlCode, const wxString& filePath)
{
    m_entity     = VHDLEntityParser::ParseText(vhdlCode);
    m_netlist    = VHDLGateExtractor::Extract(vhdlCode, m_entity);
    m_hasContent = m_entity.valid;
    m_hasGates   = m_hasContent && !m_netlist.gates.empty();

    // RTL View parses VHDL source only. Without this, a Verilog file just
    // renders an empty panel and the user has no idea why.
    if (!m_hasContent)
    {
        wxString ext = wxFileName(filePath).GetExt().Lower();
        bool looksVerilog = (ext == "v" || ext == "sv" || ext == "svh")
                            || (filePath.IsEmpty()
                                && vhdlCode.Lower().Contains("module ")
                                && !vhdlCode.Lower().Contains("entity "));

        if (looksVerilog)
            m_schem->SetEmptyMessage(
                "RTL View currently parses VHDL only, not Verilog or SystemVerilog.\n"
                "Open a .vhd file to see a schematic here.");
        else
            m_schem->SetEmptyMessage(wxEmptyString);
    }
    else
    {
        m_schem->SetEmptyMessage(wxEmptyString);
    }

    m_schem->SetData(&m_entity, &m_netlist, m_hasContent, m_hasGates);
    BuildTree();
    UpdateInfoLabel();
}

void RTLViewPanel::Clear()
{
    m_entity     = VHDLEntity{};
    m_netlist    = RTLNetlist{};
    m_hasContent = false;
    m_hasGates   = false;
    m_schem->Clear();
    m_tree->DeleteAllItems();
    UpdateInfoLabel();
}

void RTLViewPanel::ReapplyTheme()
{
    SetBackgroundColour(OCXTheme::BgPanel());
    m_tree->SetBackgroundColour(OCXTheme::BgEditor());
    m_tree->SetForegroundColour(OCXTheme::FgText());
    m_schem->ReapplyTheme();
    Refresh();
}

//** RTLViewPanel : tree **//
void RTLViewPanel::BuildTree()
{
    m_tree->DeleteAllItems();
    if (!m_hasContent) return;

    m_treeRoot = m_tree->AddRoot(m_entity.name);

    m_treeInputs = m_tree->AppendItem(m_treeRoot, "Inputs");
    for (int i = 0; i < (int)m_netlist.inputPorts.GetCount(); ++i)
        m_tree->AppendItem(m_treeInputs, m_netlist.inputPorts[i],
                           -1, -1, new RTLTreeData(RTLTreeData::InputPort, i));
    m_tree->Expand(m_treeInputs);

    m_treeOutputs = m_tree->AppendItem(m_treeRoot, "Outputs");
    for (int i = 0; i < (int)m_netlist.outputPorts.GetCount(); ++i)
        m_tree->AppendItem(m_treeOutputs, m_netlist.outputPorts[i],
                           -1, -1, new RTLTreeData(RTLTreeData::OutputPort, i));
    m_tree->Expand(m_treeOutputs);

    m_treeSignals = m_tree->AppendItem(m_treeRoot, "Signals");
    int sigCount = 0;
    for (int i = 0; i < (int)m_netlist.gates.size(); ++i)
    {
        const wxString& sig = m_netlist.gates[i].outputSignal;
        if (m_netlist.outputPorts.Index(sig) == wxNOT_FOUND)
        {
            m_tree->AppendItem(m_treeSignals, sig, -1, -1,
                               new RTLTreeData(RTLTreeData::Signal, i));
            ++sigCount;
        }
    }
    if (sigCount > 0) m_tree->Expand(m_treeSignals);

    m_treeGates = m_tree->AppendItem(m_treeRoot, "Gates");
    for (int i = 0; i < (int)m_netlist.gates.size(); ++i)
    {
        const RTLGateInst& g = m_netlist.gates[i];
        m_tree->AppendItem(m_treeGates,
                           RTLGateLabel(g.gateType) + "  " + g.outputSignal,
                           -1, -1, new RTLTreeData(RTLTreeData::Gate, i));
    }
    if (m_hasGates) m_tree->Expand(m_treeGates);

    m_tree->ExpandAll();
}

void RTLViewPanel::OnTreeSelect(wxTreeEvent& event)
{
    if (m_syncingTree) { event.Skip(); return; }

    wxTreeItemId id = event.GetItem();
    if (!id.IsOk()) { event.Skip(); return; }

    auto* data = static_cast<RTLTreeData*>(m_tree->GetItemData(id));
    int sel = -1;
    if (data && (data->kind == RTLTreeData::Gate ||
                 data->kind == RTLTreeData::Signal))
        sel = data->idx;

    m_schem->SelectGate(sel);
    UpdateInfoLabel();
}

void RTLViewPanel::SyncTreeToGate(int gateIdx)
{
    if (m_syncingTree || !m_treeGates.IsOk()) return;
    m_syncingTree = true;

    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(m_treeGates, cookie);
    while (child.IsOk())
    {
        auto* data = static_cast<RTLTreeData*>(m_tree->GetItemData(child));
        if (data && data->kind == RTLTreeData::Gate && data->idx == gateIdx)
        {
            m_tree->SelectItem(child);
            break;
        }
        child = m_tree->GetNextChild(m_treeGates, cookie);
    }
    if (gateIdx < 0)
        m_tree->UnselectAll();

    m_syncingTree = false;
}

void RTLViewPanel::UpdateInfoLabel()
{
    if (m_zoomLabel) m_zoomLabel->SetLabel(m_schem->GetZoomText());
    if (m_infoLabel) m_infoLabel->SetLabel(m_schem->GetInfoText());
}
