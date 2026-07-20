#include "rtl_view_schematic_p.h"
#include <wx/dcgraph.h>
#include <wx/filedlg.h>

//** Schematic : construction **//
RTLViewPanel::Schematic::Schematic(wxWindow* parent, RTLViewPanel* owner)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxHSCROLL | wxVSCROLL | wxBORDER_NONE)
    , m_owner(owner)
{
    SetBackgroundColour(OCXTheme::BgPanel());
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetScrollRate(10, 10);

    Bind(wxEVT_PAINT,        &Schematic::OnPaint,       this);
    Bind(wxEVT_MOUSEWHEEL,   &Schematic::OnMouseWheel,  this);
    Bind(wxEVT_LEFT_DOWN,    &Schematic::OnLeftClick,   this);
    Bind(wxEVT_RIGHT_DOWN,   &Schematic::OnRightDown,   this);
    Bind(wxEVT_RIGHT_UP,     &Schematic::OnRightUp,     this);
    Bind(wxEVT_MOTION,       &Schematic::OnMouseMove,   this);
    Bind(wxEVT_CONTEXT_MENU, &Schematic::OnContextMenu, this);
}

//** Schematic : public API **//
void RTLViewPanel::Schematic::SetData(const VHDLEntity* ent, const RTLNetlist* net,
                                       bool hasContent, bool hasGates)
{
    wxString oldName = m_entity ? m_entity->name : wxString();
    wxString newName = ent      ? ent->name      : wxString();
    bool     sameEntity = !oldName.IsEmpty() && oldName == newName;
    int sx = 0, sy = 0;
    int prevSel = -1;
    if (sameEntity)
    {
        GetViewStart(&sx, &sy);
        if (m_selGate >= 0 && net && m_selGate < (int)net->gates.size())
            prevSel = m_selGate;
    }

    m_entity     = ent;
    m_netlist    = net;
    m_hasContent = hasContent;
    m_hasGates   = hasGates;
    m_selGate    = prevSel;
    ComputeLayout();
    UpdateVirtualSize();
    Scroll(sx, sy);
    Refresh();
}

void RTLViewPanel::Schematic::Clear()
{
    m_entity     = nullptr;
    m_netlist    = nullptr;
    m_hasContent = false;
    m_hasGates   = false;
    m_selGate    = -1;
    m_gpos.clear();
    m_logH       = 500;
    SetVirtualSize(0, 0);
    Refresh();
}

void RTLViewPanel::Schematic::SetZoom(double z)
{
    m_zoom = std::max(0.15, std::min(4.0, z));
    UpdateVirtualSize();
    m_owner->UpdateInfoLabel();
    Refresh();
}

void RTLViewPanel::Schematic::FitToWindow()
{
    wxSize client = GetClientSize();
    if (client.x <= 10 || client.y <= 10) return;
    double zx = (double)client.x / m_logW;
    double zy = (double)client.y / m_logH;
    SetZoom(std::min(zx, zy) * 0.93);
}

void RTLViewPanel::Schematic::SelectGate(int idx)
{
    m_selGate = idx;
    Refresh();
}

wxString RTLViewPanel::Schematic::GetZoomText() const
{
    return wxString::Format("%d%%", (int)(m_zoom * 100 + 0.5));
}

wxString RTLViewPanel::Schematic::GetInfoText() const
{
    if (m_selGate < 0 || !m_netlist ||
        m_selGate >= (int)m_netlist->gates.size())
        return "Click a gate to inspect";

    const RTLGateInst& g = m_netlist->gates[m_selGate];
    wxString inputs;
    for (size_t i = 0; i < g.inputSignals.GetCount(); ++i)
    {
        if (i) inputs += ", ";
        inputs += g.inputSignals[i];
    }
    return RTLGateLabel(g.gateType) + ":   " + inputs + "  ->" + g.outputSignal;
}

void RTLViewPanel::Schematic::ReapplyTheme()
{
    SetBackgroundColour(OCXTheme::BgPanel());
    Refresh();
}

//** Schematic : export **//
void RTLViewPanel::Schematic::ExportPNG()
{
    if (!m_hasContent) return;

    wxFileDialog dlg(this, "Export Schematic as PNG", wxEmptyString, "schematic.png",
                     "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    wxBitmap bmp(m_logW, m_logH, 32);
    {
        wxMemoryDC memDC(bmp);
        wxGCDC     gcdc(memDC);
        wxDC&      dc = gcdc;

        dc.SetBackground(wxBrush(OCXTheme::BgPanel()));
        dc.Clear();

        wxSize             logSz(m_logW, m_logH);
        wxGraphicsContext* gc = gcdc.GetGraphicsContext();

        DrawEntity(dc, logSz);
        if (m_hasGates)
            DrawGateSchematic(dc, gc, logSz, GateTopY());
    }

    if (!bmp.SaveFile(dlg.GetPath(), wxBITMAP_TYPE_PNG))
        wxMessageBox("Failed to save PNG.", "Export Error",
                     wxOK | wxICON_ERROR, this);
}

//** Schematic : layout helpers **//
int RTLViewPanel::Schematic::GateTopY() const
{
    if (!m_entity) return 200;
    return 60 + EntityBlockHeight(*m_entity) + 24 + 60;
}

void RTLViewPanel::Schematic::ComputeLayout()
{
    m_gpos.clear();

    if (!m_entity)
    {
        m_logH = 500;
        return;
    }

    int entityH = EntityBlockHeight(*m_entity) + 80;

    if (!m_hasGates || !m_netlist || m_netlist->gates.empty())
    {
        m_logH = std::max(500, 60 + entityH + 40);
        return;
    }

    const int GATE_W = 80, GATE_H = 50, COL_W = 220, ROW_H = 100, PORT_COL = 110;
    int n = (int)m_netlist->gates.size();

    std::map<wxString, int> sl;
    for (size_t i = 0; i < m_netlist->inputPorts.GetCount(); ++i)
        sl[m_netlist->inputPorts[i]] = 0;
    std::vector<int> lv(n, 1);
    for (int pass = 0; pass <= n; ++pass)
        for (int i = 0; i < n; ++i)
        {
            int mx = 0;
            for (size_t pi = 0; pi < m_netlist->gates[i].inputSignals.GetCount(); ++pi)
            {
                auto it = sl.find(m_netlist->gates[i].inputSignals[pi]);
                if (it != sl.end()) mx = std::max(mx, it->second);
            }
            lv[i] = mx + 1;
            sl[m_netlist->gates[i].outputSignal] = lv[i];
        }

    int maxLvl = *std::max_element(lv.begin(), lv.end());

    std::vector<std::vector<int>> cols(maxLvl);
    for (int i = 0; i < n; ++i) cols[lv[i] - 1].push_back(i);

    int maxRows = 0;
    for (auto& c : cols) maxRows = std::max(maxRows, (int)c.size());
    int inC  = (int)m_netlist->inputPorts.GetCount();
    int outC = (int)m_netlist->outputPorts.GetCount();
    maxRows  = std::max(maxRows, std::max(inC, outC));

    int totalW   = PORT_COL + maxLvl * COL_W + PORT_COL;
    int originX  = std::max(20, (m_logW - totalW) / 2);
    int gateBand = originX + PORT_COL;
    int areaH    = maxRows * ROW_H;
    int gateTop  = GateTopY();

    m_gpos.resize(n);
    for (int c = 0; c < maxLvl; ++c)
    {
        int rows   = (int)cols[c].size();
        int colH   = rows * ROW_H;
        int startY = gateTop + (areaH - colH) / 2 + (ROW_H - GATE_H) / 2;
        int colCX  = gateBand + c * COL_W + COL_W / 2;
        for (int r = 0; r < rows; ++r)
        {
            int gi = cols[c][r];
            m_gpos[gi] = wxPoint(colCX - GATE_W / 2, startY + r * ROW_H);
        }
    }

    int gateH = 100 + maxRows * ROW_H + 80;
    m_logH = std::max(500, 60 + entityH + gateH + 40);
}

void RTLViewPanel::Schematic::UpdateVirtualSize()
{
    SetVirtualSize((int)(m_logW * m_zoom), (int)(m_logH * m_zoom));
}
