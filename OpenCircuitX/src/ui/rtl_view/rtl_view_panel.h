#pragma once
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/dcgraph.h>
#include "core/hdl/vhdl_entity_parser.h"
#include "core/hdl/vhdl_gate_extractor.h"

class RTLViewPanel : public wxPanel
{
public:
    explicit RTLViewPanel(wxWindow* parent);

    // filePath is optional and only used to explain an empty result, e.g. to
    // tell the user RTL View doesn't parse Verilog rather than drawing nothing.
    void ParseAndShow(const wxString& vhdlCode,
                      const wxString& filePath = wxEmptyString);
    void Clear();
    void ReapplyTheme();

    // Called by inner Schematic on gate click to sync tree selection
    void SyncTreeToGate(int gateIdx);
    // Called by Schematic whenever zoom or selection changes
    void UpdateInfoLabel();

private:
    class Schematic;   // defined in .cpp

    void BuildTree();
    void OnTreeSelect(wxTreeEvent& event);

    wxSplitterWindow* m_splitter    = nullptr;
    wxTreeCtrl*       m_tree        = nullptr;
    wxStaticText*     m_zoomLabel   = nullptr;
    wxStaticText*     m_infoLabel   = nullptr;
    Schematic*        m_schem       = nullptr;
    bool              m_syncingTree = false;

    VHDLEntity  m_entity;
    RTLNetlist  m_netlist;
    bool        m_hasContent = false;
    bool        m_hasGates   = false;

    wxTreeItemId m_treeRoot;
    wxTreeItemId m_treeInputs;
    wxTreeItemId m_treeOutputs;
    wxTreeItemId m_treeGates;
    wxTreeItemId m_treeSignals;
};
