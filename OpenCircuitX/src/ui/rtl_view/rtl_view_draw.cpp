#include "rtl_view_schematic_p.h"
#include <wx/dcgraph.h>
#include <wx/arrstr.h>

//** Schematic : drawing **//
void RTLViewPanel::Schematic::DrawEmptyState(wxDC& dc, const wxSize& sz)
{
    dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT,
                      wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(OCXTheme::FgDim());

    wxString msg = m_emptyMsg.IsEmpty()
                     ? "RTL View  --  save a VHDL file to render the schematic"
                     : m_emptyMsg;

    // Message may carry a second explanatory line.
    wxArrayString lines = wxSplit(msg, '\n');
    int lineH = dc.GetCharHeight() + 4;
    int blockY = (sz.y - lineH * (int)lines.GetCount()) / 2;

    for (size_t i = 0; i < lines.GetCount(); ++i)
    {
        wxSize tsz = dc.GetTextExtent(lines[i]);
        dc.DrawText(lines[i], (sz.x - tsz.x) / 2, blockY + (int)i * lineH);
    }
}

void RTLViewPanel::Schematic::DrawEntity(wxDC& dc, const wxSize& logSz)
{
    if (!m_entity) return;

    std::vector<const VHDLPort*> leftPorts, rightPorts;
    for (const auto& p : m_entity->ports)
        (p.dir == PortDir::In || p.dir == PortDir::InOut
             ? leftPorts : rightPorts).push_back(&p);

    const int STUB = 50, PORT_STP = 30, PAD_V = 20, HDR_H = 28, BOX_W = 220;
    int sideCount = (int)std::max(leftPorts.size(), rightPorts.size());
    int boxH      = std::max(90, HDR_H + PAD_V * 2 + sideCount * PORT_STP);
    int cx   = logSz.x / 2;
    int boxX = cx - BOX_W / 2;
    int boxY = 60;

    dc.SetPen(wxPen(wxColour(86, 156, 214), 2));
    dc.SetBrush(wxBrush(OCXTheme::BgEditor()));
    dc.DrawRectangle(boxX, boxY, BOX_W, boxH);

    dc.SetPen(wxPen(wxColour(86, 156, 214), 1));
    dc.SetBrush(wxBrush(wxColour(0, 72, 130)));
    dc.DrawRectangle(boxX, boxY, BOX_W, HDR_H);

    dc.SetFont(wxFont(10, wxFONTFAMILY_TELETYPE,
                      wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    dc.SetTextForeground(wxColour(210, 225, 255));
    wxSize nsz = dc.GetTextExtent(m_entity->name);
    dc.DrawText(m_entity->name,
                boxX + (BOX_W - nsz.x) / 2, boxY + (HDR_H - nsz.y) / 2);

    wxFont portFont(9, wxFONTFAMILY_TELETYPE,
                    wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wxFont typeFont(7, wxFONTFAMILY_TELETYPE,
                    wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL);

    int portAreaH   = (sideCount > 1) ? (sideCount - 1) * PORT_STP : 0;
    int portAreaTop = boxY + HDR_H + (boxH - HDR_H - portAreaH) / 2;

    //** left (input) ports **//
    for (int i = 0; i < (int)leftPorts.size(); ++i)
    {
        const VHDLPort* p   = leftPorts[i];
        int             py  = portAreaTop + i * PORT_STP;
        wxColour        col = ColourForDir(p->dir);

        dc.SetPen(wxPen(col, 2));
        dc.DrawLine(boxX - STUB, py, boxX, py);
        dc.DrawLine(boxX - 8, py - 4, boxX, py);
        dc.DrawLine(boxX - 8, py + 4, boxX, py);

        if (p->dir == PortDir::InOut)
        {
            dc.DrawLine(boxX - STUB + 8, py - 4, boxX - STUB, py);
            dc.DrawLine(boxX - STUB + 8, py + 4, boxX - STUB, py);
        }

        dc.SetBrush(wxBrush(col));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawCircle(boxX, py, 3);

        dc.SetFont(portFont);
        dc.SetTextForeground(OCXTheme::FgText());
        int nameH = dc.GetTextExtent("Ag").y;
        dc.DrawText(p->name, boxX + 8, py - nameH / 2);

        if (!p->typeName.IsEmpty())
        {
            dc.SetFont(typeFont);
            dc.SetTextForeground(OCXTheme::FgDim());
            wxSize tsz = dc.GetTextExtent(p->typeName);
            dc.DrawText(p->typeName, boxX - STUB - tsz.x - 4, py - tsz.y / 2);
        }
    }

    //** right (output) ports **//
    int re = boxX + BOX_W;
    for (int i = 0; i < (int)rightPorts.size(); ++i)
    {
        const VHDLPort* p   = rightPorts[i];
        int             py  = portAreaTop + i * PORT_STP;
        wxColour        col = ColourForDir(p->dir);

        dc.SetPen(wxPen(col, 2));
        dc.DrawLine(re, py, re + STUB, py);
        dc.DrawLine(re + STUB - 8, py - 4, re + STUB, py);
        dc.DrawLine(re + STUB - 8, py + 4, re + STUB, py);

        dc.SetBrush(wxBrush(col));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawCircle(re, py, 3);

        dc.SetFont(portFont);
        dc.SetTextForeground(OCXTheme::FgText());
        wxSize pnsz = dc.GetTextExtent(p->name);
        dc.DrawText(p->name, re - 8 - pnsz.x, py - pnsz.y / 2);

        if (!p->typeName.IsEmpty())
        {
            dc.SetFont(typeFont);
            dc.SetTextForeground(OCXTheme::FgDim());
            int th = dc.GetTextExtent("Ag").y;
            dc.DrawText(p->typeName, re + STUB + 6, py - th / 2);
        }
    }

    //** colour legend **//
    struct { PortDir d; const char* lbl; } items[] = {
        {PortDir::In,    "in"},
        {PortDir::Out,   "out"},
        {PortDir::InOut, "inout"},
        {PortDir::Buffer,"buffer"},
    };
    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT,
                      wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    int legY = boxY + boxH + 24;
    int lx   = boxX;
    for (auto& lg : items)
    {
        wxColour c = ColourForDir(lg.d);
        dc.SetBrush(wxBrush(c));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(lx, legY + 2, 10, 10);
        dc.SetTextForeground(OCXTheme::FgDim());
        dc.DrawText(lg.lbl, lx + 14, legY);
        lx += 14 + dc.GetTextExtent(lg.lbl).x + 16;
    }
}

void RTLViewPanel::Schematic::DrawGateSchematic(wxDC& dc, wxGraphicsContext* gc,
                                                 const wxSize& logSz, int topY)
{
    if (!m_netlist || m_netlist->gates.empty()) return;

    const int GATE_W   = 80;
    const int GATE_H   = 50;
    const int COL_W    = 220;
    const int ROW_H    = 100;
    const int PORT_W   = 34;
    const int PORT_H   = 22;
    const int PORT_COL = 110;

    int n = (int)m_netlist->gates.size();

    // Section header
    {
        dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT,
                          wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        dc.SetTextForeground(OCXTheme::FgDim());
        wxString title = "Gate-Level Schematic";
        wxSize   tsz   = dc.GetTextExtent(title);
        int cx = logSz.x / 2;
        dc.DrawText(title, cx - tsz.x / 2, topY - 36);
        dc.SetPen(wxPen(wxColour(60, 60, 70), 1));
        dc.DrawLine(cx - 200, topY - 14, cx + 200, topY - 14);
    }

    //** level computation **//
    std::map<wxString, int> sigLevel;
    for (size_t i = 0; i < m_netlist->inputPorts.GetCount(); ++i)
        sigLevel[m_netlist->inputPorts[i]] = 0;
    std::vector<int> lvl(n, 1);
    for (int pass = 0; pass <= n; ++pass)
        for (int i = 0; i < n; ++i)
        {
            int mx = 0;
            for (size_t pi = 0;
                 pi < m_netlist->gates[i].inputSignals.GetCount(); ++pi)
            {
                auto it = sigLevel.find(m_netlist->gates[i].inputSignals[pi]);
                if (it != sigLevel.end()) mx = std::max(mx, it->second);
            }
            lvl[i] = mx + 1;
            sigLevel[m_netlist->gates[i].outputSignal] = lvl[i];
        }

    int maxLvl = *std::max_element(lvl.begin(), lvl.end());

    std::vector<std::vector<int>> cols(maxLvl);
    for (int i = 0; i < n; ++i) cols[lvl[i] - 1].push_back(i);

    int maxRows = 0;
    for (auto& c : cols) maxRows = std::max(maxRows, (int)c.size());
    int inCount  = (int)m_netlist->inputPorts.GetCount();
    int outCount = (int)m_netlist->outputPorts.GetCount();
    maxRows = std::max(maxRows, std::max(inCount, outCount));

    //** pixel layout **//
    int totalW   = PORT_COL + maxLvl * COL_W + PORT_COL;
    int originX  = std::max(20, (logSz.x - totalW) / 2);
    int gateBand = originX + PORT_COL;
    int outPortX = gateBand + maxLvl * COL_W;
    int areaH    = maxRows * ROW_H;

    const bool useCached = ((int)m_gpos.size() == n);
    std::vector<wxPoint> localPos;
    const std::vector<wxPoint>& gpos = useCached ? m_gpos : localPos;

    if (!useCached)
    {
        localPos.resize(n);
        for (int c = 0; c < maxLvl; ++c)
        {
            int rows   = (int)cols[c].size();
            int colH   = rows * ROW_H;
            int startY = topY + (areaH - colH) / 2 + (ROW_H - GATE_H) / 2;
            int colCX  = gateBand + c * COL_W + COL_W / 2;
            for (int r = 0; r < rows; ++r)
            {
                int gi = cols[c][r];
                localPos[gi] = wxPoint(colCX - GATE_W / 2, startY + r * ROW_H);
            }
        }
    }

    int inStartY  = topY + (areaH - inCount  * ROW_H) / 2 + (ROW_H - PORT_H) / 2;
    int outStartY = topY + (areaH - outCount * ROW_H) / 2 + (ROW_H - PORT_H) / 2;

    //** driver and sink pin maps **//
    std::map<wxString, wxPoint>              drvPin;
    std::map<wxString, std::vector<wxPoint>> sinkMap;

    for (int i = 0; i < n; ++i)
        drvPin[m_netlist->gates[i].outputSignal] =
            wxPoint(gpos[i].x + GATE_W, gpos[i].y + GATE_H / 2);

    for (int i = 0; i < inCount; ++i)
        drvPin[m_netlist->inputPorts[i]] =
            wxPoint(originX + PORT_W, inStartY + i * ROW_H + PORT_H / 2);

    for (int gi = 0; gi < n; ++gi)
    {
        const RTLGateInst& g = m_netlist->gates[gi];
        int ic = (int)g.inputSignals.GetCount();
        for (int pi = 0; pi < ic; ++pi)
        {
            int pinY = (ic == 1) ? gpos[gi].y + GATE_H / 2
                                 : gpos[gi].y + GATE_H * (pi + 1) / (ic + 1);
            sinkMap[g.inputSignals[pi]].push_back(wxPoint(gpos[gi].x, pinY));
        }
    }
    for (int i = 0; i < outCount; ++i)
    {
        int py = outStartY + i * ROW_H + PORT_H / 2;
        sinkMap[m_netlist->outputPorts[i]].push_back(wxPoint(outPortX, py));
    }

    //** selection-aware wire colours **//
    wxArrayString selInputNets;
    wxString      selOutputNet;
    bool          hasSel = (m_selGate >= 0 && m_selGate < n);
    if (hasSel)
    {
        const RTLGateInst& sg = m_netlist->gates[m_selGate];
        selInputNets = sg.inputSignals;
        selOutputNet = sg.outputSignal;
    }

    auto netWireColour = [&](const wxString& net) -> wxColour {
        if (!hasSel)                                return wxColour( 80, 160, 230);
        if (net == selOutputNet)                    return wxColour( 60, 220, 100);
        if (selInputNets.Index(net) != wxNOT_FOUND) return wxColour(255, 160,  50);
        return wxColour(35, 65, 110);
    };
    auto netWireWidth = [&](const wxString& net) -> int {
        if (!hasSel) return 1;
        if (net == selOutputNet || selInputNets.Index(net) != wxNOT_FOUND) return 2;
        return 1;
    };

    //** channel allocation **//
    std::map<std::pair<int,int>, std::vector<wxString>> bands;
    for (auto& kv : sinkMap)
    {
        auto it = drvPin.find(kv.first);
        if (it == drvPin.end()) continue;
        int srcX  = it->second.x;
        int nearX = kv.second[0].x;
        for (size_t s = 1; s < kv.second.size(); ++s)
            nearX = std::min(nearX, kv.second[s].x);
        bands[{srcX, nearX}].push_back(kv.first);
    }

    std::map<wxString, int> netChanX;
    for (auto& bkv : bands)
    {
        int srcX  = bkv.first.first;
        int nearX = bkv.first.second;

        std::sort(bkv.second.begin(), bkv.second.end(),
            [&drvPin](const wxString& a, const wxString& b) {
                auto ia = drvPin.find(a), ib = drvPin.find(b);
                if (ia == drvPin.end()) return false;
                if (ib == drvPin.end()) return true;
                return ia->second.y < ib->second.y;
            });

        int count  = (int)bkv.second.size();
        int margin = 8;
        int usable = (nearX - srcX) - 2 * margin;
        int step   = (count > 1) ? usable / (count + 1) : usable / 2;
        if (step < 6) step = 6;

        for (int i = 0; i < count; ++i)
            netChanX[bkv.second[i]] = srcX + margin + step * (i + 1);
    }

    //** wires (H-V-H with junction dots on fan-out) **//
    for (auto& kv : sinkMap)
    {
        const wxString&             net   = kv.first;
        const std::vector<wxPoint>& sinks = kv.second;
        if (sinks.empty()) continue;
        auto it = drvPin.find(net);
        if (it == drvPin.end()) continue;

        wxPoint src   = it->second;
        auto    cxIt  = netChanX.find(net);
        int     chanX = (cxIt != netChanX.end()) ? cxIt->second : src.x + 20;

        wxColour wc = netWireColour(net);
        int      ww = netWireWidth(net);

        dc.SetPen(wxPen(wc, ww));
        dc.DrawLine(src.x, src.y, chanX, src.y);

        if (sinks.size() == 1)
        {
            dc.DrawLine(chanX, src.y,      chanX,      sinks[0].y);
            dc.DrawLine(chanX, sinks[0].y, sinks[0].x, sinks[0].y);
        }
        else
        {
            int minY = src.y, maxY = src.y;
            for (auto& sk : sinks) { minY = std::min(minY, sk.y); maxY = std::max(maxY, sk.y); }
            dc.DrawLine(chanX, minY, chanX, maxY);
            for (auto& sk : sinks)
                dc.DrawLine(chanX, sk.y, sk.x, sk.y);
            dc.SetBrush(wxBrush(wc));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawCircle(chanX, src.y, 4);
        }
    }

    //** input port symbols **//
    wxFont portFont(8, wxFONTFAMILY_TELETYPE,
                    wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wxColour inClrBase(78, 201, 176), outClrBase(106, 210, 106);

    for (int i = 0; i < inCount; ++i)
    {
        const wxString& portNet = m_netlist->inputPorts[i];
        bool     lit  = hasSel && selInputNets.Index(portNet) != wxNOT_FOUND;
        wxColour clr  = lit ? wxColour(255, 160, 50) : inClrBase;
        wxColour fill = lit ? wxColour( 60,  38,  8) : wxColour(18, 48, 52);

        int px = originX;
        int py = inStartY + i * ROW_H;
        wxPoint pts[3] = { {px, py}, {px, py + PORT_H}, {px + PORT_W, py + PORT_H / 2} };
        dc.SetPen(wxPen(clr, lit ? 2 : 1));
        dc.SetBrush(wxBrush(fill));
        dc.DrawPolygon(3, pts);

        dc.SetFont(portFont);
        dc.SetTextForeground(clr);
        wxSize tsz = dc.GetTextExtent(portNet);
        dc.DrawText(portNet, px - tsz.x - 6, py + PORT_H / 2 - tsz.y / 2);
    }

    //** output port symbols **//
    for (int i = 0; i < outCount; ++i)
    {
        const wxString& portNet = m_netlist->outputPorts[i];
        bool     lit  = hasSel && portNet == selOutputNet;
        wxColour clr  = lit ? wxColour( 60, 220, 100) : outClrBase;
        wxColour fill = lit ? wxColour(  8,  50,  18) : wxColour(18, 48, 22);

        int px = outPortX;
        int py = outStartY + i * ROW_H;
        wxPoint pts[3] = { {px, py}, {px, py + PORT_H}, {px + PORT_W, py + PORT_H / 2} };
        dc.SetPen(wxPen(clr, lit ? 2 : 1));
        dc.SetBrush(wxBrush(fill));
        dc.DrawPolygon(3, pts);

        dc.SetFont(portFont);
        dc.SetTextForeground(clr);
        wxSize tsz = dc.GetTextExtent(portNet);
        dc.DrawText(portNet, px + PORT_W + 6, py + PORT_H / 2 - tsz.y / 2);
    }

    //** gates **//
    ANSIGateRenderer renderer;
    wxFont typeFont(8, wxFONTFAMILY_TELETYPE,
                    wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    wxFont instFont(7, wxFONTFAMILY_TELETYPE,
                    wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL);

    for (int gi = 0; gi < n; ++gi)
    {
        const RTLGateInst& gate   = m_netlist->gates[gi];
        wxRect             bounds(gpos[gi], wxSize(GATE_W, GATE_H));

        if (gi == m_selGate)
        {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(wxPen(wxColour(255, 215, 0), 2));
            dc.DrawRoundedRectangle(bounds.x - 4, bounds.y - 4,
                                    bounds.width + 8, bounds.height + 8, 6);
        }

        wxColour body(30, 45, 70), border = OCXTheme::Accent();
        bool ansi = false;
        if (gc && RTLGateDistinctive(gate.gateType))
        {
            renderer.Draw(gc, gate.gateType, bounds, body, border);
            ansi = true;
        }
        if (!ansi)
        {
            dc.SetBrush(wxBrush(body));
            dc.SetPen(wxPen(border, 1));
            dc.DrawRoundedRectangle(bounds, 4);
            dc.SetFont(typeFont);
            dc.SetTextForeground(OCXTheme::FgText());
            wxString lbl = RTLGateLabel(gate.gateType);
            wxSize   lsz = dc.GetTextExtent(lbl);
            dc.DrawText(lbl, bounds.x + (GATE_W - lsz.x) / 2,
                             bounds.y + (GATE_H - lsz.y) / 2);
        }

        dc.SetFont(instFont);
        dc.SetTextForeground(gi == m_selGate ? wxColour(255, 215, 0)
                                              : OCXTheme::FgDim());
        wxSize isz = dc.GetTextExtent(gate.outputSignal);
        dc.DrawText(gate.outputSignal,
                    bounds.x + (GATE_W - isz.x) / 2,
                    bounds.y + GATE_H + 4);
    }
}
