#pragma once
#include "rtl_view_panel.h"
#include "ui/canvas/gate_renderer_ansi.h"
#include "ui/shell/app_theme.h"
#include <wx/scrolwin.h>
#include <algorithm>
#include <map>
#include <vector>

//** RTLTreeData **//
class RTLTreeData : public wxTreeItemData
{
public:
    enum Kind { None, InputPort, OutputPort, Gate, Signal };
    RTLTreeData(Kind k, int i) : kind(k), idx(i) {}
    Kind kind = None;
    int  idx  = -1;
};

//** Static helpers **//
static bool RTLGateDistinctive(GateType t)
{
    switch (t) {
    case GateType::AND: case GateType::OR:   case GateType::NOT:
    case GateType::NAND:case GateType::NOR:  case GateType::XOR:
    case GateType::XNOR: return true;
    default:             return false;
    }
}

static wxString RTLGateLabel(GateType t)
{
    switch (t) {
    case GateType::AND:  return "AND";
    case GateType::OR:   return "OR";
    case GateType::NOT:  return "NOT";
    case GateType::NAND: return "NAND";
    case GateType::NOR:  return "NOR";
    case GateType::XOR:  return "XOR";
    case GateType::XNOR: return "XNOR";
    default:             return "GATE";
    }
}

static wxColour ColourForDir(PortDir d)
{
    switch (d) {
    case PortDir::Out:    return wxColour(106, 210, 106);
    case PortDir::InOut:  return wxColour(255, 215,   0);
    case PortDir::Buffer: return wxColour(255, 160,  40);
    default:              return wxColour( 78, 201, 176);
    }
}

static int EntityBlockHeight(const VHDLEntity& e)
{
    const int HDR_H = 28, PORT_STP = 30, PAD_V = 20;
    int left = 0, right = 0;
    for (const auto& p : e.ports)
        (p.dir == PortDir::In || p.dir == PortDir::InOut ? left : right)++;
    return HDR_H + PAD_V * 2 + std::max(1, std::max(left, right)) * PORT_STP;
}

//** RTLViewPanel::Schematic - inner scrolled drawing window **//
class RTLViewPanel::Schematic : public wxScrolledWindow
{
public:
    Schematic(wxWindow* parent, RTLViewPanel* owner);

    void SetData(const VHDLEntity* ent, const RTLNetlist* net,
                 bool hasContent, bool hasGates);
    void Clear();

    void   SetZoom(double z);
    void   ZoomIn()       { SetZoom(m_zoom * 1.25); }
    void   ZoomOut()      { SetZoom(m_zoom / 1.25); }
    void   FitToWindow();
    void   ResetZoom()    { SetZoom(1.0); }
    double GetZoom() const { return m_zoom; }

    void SelectGate(int idx);
    int  GetSelectedGate() const { return m_selGate; }

    void ExportPNG();

    wxString GetZoomText() const;
    wxString GetInfoText() const;

    void ReapplyTheme();

private:
    void ComputeLayout();
    int  GateTopY()       const;
    void UpdateVirtualSize();

    void OnPaint(wxPaintEvent&);
    void OnMouseWheel(wxMouseEvent&);
    void OnLeftClick(wxMouseEvent&);
    void OnRightDown(wxMouseEvent&);
    void OnRightUp(wxMouseEvent&);
    void OnMouseMove(wxMouseEvent&);
    void OnContextMenu(wxContextMenuEvent&);

    void DrawEmptyState(wxDC& dc, const wxSize& clientSz);
    void DrawEntity(wxDC& dc, const wxSize& logSz);
    void DrawGateSchematic(wxDC& dc, wxGraphicsContext* gc,
                           const wxSize& logSz, int topY);

    RTLViewPanel*         m_owner      = nullptr;
    const VHDLEntity*     m_entity     = nullptr;
    const RTLNetlist*     m_netlist    = nullptr;
    bool                  m_hasContent = false;
    bool                  m_hasGates   = false;
    double                m_zoom       = 1.0;
    int                   m_selGate    = -1;

    std::vector<wxPoint>  m_gpos;
    int                   m_logW       = 1200;
    int                   m_logH       = 500;

    bool                  m_panning    = false;
    bool                  m_hasPanned  = false;
    wxPoint               m_panStart;
    int                   m_panAccumX  = 0;
    int                   m_panAccumY  = 0;
};
