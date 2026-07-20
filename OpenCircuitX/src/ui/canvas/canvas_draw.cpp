#include "circuit_canvas.h"
#include "ui/canvas/circuit_canvas_private.h"
#include "ui/canvas/gate_renderer_ansi.h"
#include "ui/shell/app_theme.h"
#include <algorithm>
#include <cmath>
#include <wx/graphics.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/textdlg.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <climits>
#include <map>
#include <wx/file.h>

void CircuitCanvas::DrawGrid(wxDC& dc) const
{
    dc.SetPen(wxPen(wxColour(45, 45, 45), 1));
    wxSize vs = GetVirtualSize();
    int gScaled = std::max(4, (int)(GRID * m_zoom));
    for (int x = 0; x < int(vs.GetWidth() * m_zoom);  x += gScaled)
        dc.DrawLine(x, 0, x, int(vs.GetHeight() * m_zoom));
    for (int y = 0; y < int(vs.GetHeight() * m_zoom); y += gScaled)
        dc.DrawLine(0, y, int(vs.GetWidth() * m_zoom), y);
}

void CircuitCanvas::DrawGate(wxDC& dc, const Gate& gate, wxGraphicsContext* gc) const
{
    wxRect bounds = gate.GetBounds();

    // Scale bounds for zoom
    wxRect sb(int(bounds.x * m_zoom), int(bounds.y * m_zoom),
              int(bounds.width * m_zoom), int(bounds.height * m_zoom));

    bool sel = (dragGateId == gate.id && dragging);

    // Body colour by type
    wxColour bodyColour, borderColour;
    if (gate.type == GateType::CLOCK)
    {
        bodyColour   = sel ? wxColour(90, 70, 20) : wxColour(70, 55, 15);
        borderColour = wxColour(200, 160, 40);
    }
    else if (gate.type == GateType::INPUT)
    {
        bodyColour   = sel ? wxColour(20, 80, 50) : wxColour(15, 60, 35);
        borderColour = wxColour(60, 200, 120);
    }
    else if (gate.type == GateType::OUTPUT)
    {
        bodyColour   = sel ? wxColour(70, 20, 70) : wxColour(50, 15, 50);
        borderColour = wxColour(200, 80, 200);
    }
    else if (gate.type == GateType::SRLATCH)
    {
        bodyColour   = sel ? wxColour(20, 70, 65) : wxColour(12, 52, 48);
        borderColour = wxColour(60, 200, 180);
    }
    else if (gate.type == GateType::TRISTATE)
    {
        bodyColour   = sel ? wxColour(65, 45, 20) : wxColour(48, 33, 12);
        borderColour = wxColour(220, 140,  40);   // amber - high-Z colour
    }
    else if (gate.type == GateType::DEMUX)
    {
        bodyColour   = sel ? wxColour(45, 30, 70) : wxColour(32, 20, 52);
        borderColour = wxColour(160,  90, 230);   // purple
    }
    else
    {
        bodyColour   = sel ? wxColour(55, 75, 110) : wxColour(40, 55, 80);
        borderColour = OCXTheme::Accent();
    }

    // Selection highlight
    if (m_selectedGates.count(gate.id))
    {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(100, 160, 255), 2));
        dc.DrawRoundedRectangle(sb.Inflate(5, 5), 7);
    }

    // Pulse glow
    if (m_changedGates.count(gate.id))
    {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(255, 240, 100), 2));
        dc.DrawRoundedRectangle(sb.Inflate(3, 3), 6);
    }

    // Gate body - ANSI Bezier renderer for distinctive shapes, rectangle fallback otherwise
    bool drawn = false;
    if (gc && m_renderer && GateHasDistinctiveShape(gate.type))
    {
        m_renderer->Draw(gc, gate.type, sb, bodyColour, borderColour);
        drawn = true;
    }
    if (!drawn)
    {
        dc.SetBrush(wxBrush(bodyColour));
        dc.SetPen(wxPen(borderColour, 1));
        dc.DrawRoundedRectangle(sb, 4);
    }

    // Suppress the text label when the ANSI shape itself communicates the gate type.
    // Only suppress if the shape was actually drawn (gc available).
    const bool suppressLabel = drawn;
    // Gate label (centre)
    int fontSize = std::max(7, int(OCXTheme::FontSize() * m_zoom) - 2);
    wxFont f(fontSize, wxFONTFAMILY_DEFAULT,
             wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, OCXTheme::FontFace());
    dc.SetFont(f);
    dc.SetTextForeground(OCXTheme::FgText());

    wxString mainLabel = gate.GetLabel();
    if (gate.type == GateType::CLOCK)
        mainLabel = wxString::Format("CLK\n[%d]", gate.outputState ? 1 : 0);
    else if (gate.type == GateType::INPUT)
        mainLabel = (gate.label.IsEmpty() ? "IN" : gate.label)
                    + wxString::Format("\n[%d]", gate.outputState ? 1 : 0);

    if (!suppressLabel)
    {
        wxSize tSz = dc.GetMultiLineTextExtent(mainLabel);
        dc.DrawText(mainLabel,
            sb.x + (sb.width  - tSz.GetWidth())  / 2,
            sb.y + (sb.height - tSz.GetHeight()) / 2);
    }

    // Pin labels (small font, near pins)
    int pinFontSize = std::max(6, int((OCXTheme::FontSize() - 3) * m_zoom));
    wxFont pf(pinFontSize, wxFONTFAMILY_DEFAULT,
              wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, OCXTheme::FontFace());
    dc.SetFont(pf);
    dc.SetTextForeground(OCXTheme::FgDim());

    int stubLen = int(PIN_STUB * m_zoom);

    // Determine which inputs are wire-driven
    bool inputDriven[4] = { false, false, false, false };
    for (const Wire& w : wires)
        if (w.toGateId == gate.id && w.toPinIndex < 4)
            inputDriven[w.toPinIndex] = true;

    // Input pin stubs, circles, labels
    for (int i = 0; i < gate.GetInputCount(); ++i)
    {
        wxPoint pin = gate.GetInputPinPos(i);
        wxPoint sPin(int(pin.x * m_zoom), int(pin.y * m_zoom));

        bool    drn = inputDriven[i] || gate.inputState[i];
        wxColour col = PinColour(gate.inputState[i], drn);
        dc.SetPen(wxPen(col, 1));
        dc.DrawLine(sPin.x - stubLen, sPin.y, sPin.x, sPin.y);
        dc.SetBrush(wxBrush(col));
        dc.DrawCircle(sPin, int(PIN_R * m_zoom));

        wxString lbl = gate.GetInputLabel(i);
        if (!lbl.IsEmpty())
        {
            dc.SetTextForeground(OCXTheme::FgDim());
            dc.DrawText(lbl, sPin.x + 3, sPin.y - int(pinFontSize * 1.1));
        }
    }

    // Output pin stubs, circles, labels
    for (int i = 0; i < gate.GetOutputCount(); ++i)
    {
        wxPoint pin = gate.GetOutputPinPos(i);
        wxPoint sPin(int(pin.x * m_zoom), int(pin.y * m_zoom));

        bool outVal = (i == 0) ? gate.outputState : gate.outputState2;
        wxColour outCol = PinColour(outVal, gate.type != GateType::OUTPUT);
        dc.SetPen(wxPen(outCol, 1));
        dc.DrawLine(sPin.x, sPin.y, sPin.x + stubLen, sPin.y);
        dc.SetBrush(wxBrush(outCol));
        dc.DrawCircle(sPin, int(PIN_R * m_zoom));

        wxString lbl = gate.GetOutputLabel(i);
        if (!lbl.IsEmpty())
        {
            dc.SetTextForeground(OCXTheme::FgDim());
            wxSize ls = dc.GetTextExtent(lbl);
            dc.DrawText(lbl, sPin.x - ls.GetWidth() - 3,
                        sPin.y - int(pinFontSize * 1.1));
        }
    }

    // OUTPUT gate: show input pin value visually
    if (gate.type == GateType::OUTPUT && gate.GetInputCount() > 0)
    {
        wxColour valCol = PinColour(gate.inputState[0], true);
        dc.SetFont(f);
        wxString valStr = wxString::Format("[%d]", gate.inputState[0] ? 1 : 0);
        wxSize   vs     = dc.GetTextExtent(valStr);
        dc.SetTextForeground(valCol);
        dc.DrawText(valStr,
            sb.x + (sb.width  - vs.GetWidth())  / 2,
            sb.y + (sb.height - vs.GetHeight()) / 2);
    }
}

void CircuitCanvas::DrawWires(wxDC& dc) const
{
    for (const Wire& w : wires)
    {
        const Gate* from = FindGate(w.fromGateId);
        const Gate* to   = FindGate(w.toGateId);
        if (!from || !to) continue;

        wxPoint p1 = from->GetOutputPinPos(w.fromPinIndex);
        p1.x += GRID;
        wxPoint p2 = to->GetInputPinPos(w.toPinIndex);
        p2.x -= GRID;

        // Scale
        wxPoint s1(int(p1.x * m_zoom), int(p1.y * m_zoom));
        wxPoint s2(int(p2.x * m_zoom), int(p2.y * m_zoom));

        bool srcVal = (w.fromPinIndex == 0) ? from->outputState : from->outputState2;
        bool pulsing = m_changedGates.count(w.fromGateId) > 0;
        wxColour col = pulsing
            ? (srcVal ? wxColour(130, 255, 130) : wxColour(255, 130, 130))
            : PinColour(srcVal, true);
        dc.SetPen(wxPen(col, pulsing ? 3 : 2));

        int midX = (s1.x + s2.x) / 2;
        dc.DrawLine(s1.x, s1.y, midX,  s1.y);
        dc.DrawLine(midX, s1.y, midX,  s2.y);
        dc.DrawLine(midX, s2.y, s2.x,  s2.y);
    }
}

//--
// Paint
//--
void CircuitCanvas::OnPaint(wxPaintEvent& /*event*/)
{
    wxPaintDC paintDC(this);
    PrepareDC(paintDC);
    wxDC& dc = paintDC;
    dc.SetBackground(wxBrush(OCXTheme::BgEditor()));
    dc.Clear();

    // Create an anti-aliased GC for ANSI gate shapes.
    // wxGraphicsContext does not use the GDI viewport origin from PrepareDC,
    // so we apply the scroll translation manually via Translate.
    std::unique_ptr<wxGraphicsContext> gcOwner(wxGraphicsContext::Create(paintDC));
    if (gcOwner)
    {
        wxPoint dOrg;
        paintDC.GetDeviceOrigin(&dOrg.x, &dOrg.y);
        if (dOrg.x != 0 || dOrg.y != 0)
            gcOwner->Translate((double)dOrg.x, (double)dOrg.y);
    }
    wxGraphicsContext* gc = gcOwner.get();   // may be null on non-GDI+ builds

    DrawGrid(dc);
    DrawWires(dc);
    for (const Gate& g : gates)
        DrawGate(dc, g, gc);
    DrawAnnotations(dc);

    // Wire in progress
    if (drawingWire)
    {
        const Gate* from = FindGate(wireFromGateId);
        if (from)
        {
            wxPoint p1 = from->GetOutputPinPos(wireFromPinIndex);
            p1.x += GRID;
            wxPoint s1(int(p1.x * m_zoom), int(p1.y * m_zoom));
            wxPoint sc(int(wireCursorPos.x * m_zoom), int(wireCursorPos.y * m_zoom));
            dc.SetPen(wxPen(wxColour(200, 200, 60), 2, wxPENSTYLE_DOT));
            dc.DrawLine(s1, sc);
        }
    }

    // Placement ghost
    if (mode == CanvasMode::PlaceGate)
    {
        Gate tmp;
        tmp.type  = pendingType;
        tmp.pos   = ghostPos;
        tmp.label = wxEmptyString;
        wxRect  gb = tmp.GetBounds();
        wxRect  sb(int(gb.x * m_zoom), int(gb.y * m_zoom),
                   int(gb.width * m_zoom), int(gb.height * m_zoom));

        dc.SetBrush(wxBrush(wxColour(40, 65, 95)));
        dc.SetPen(wxPen(wxColour(100, 160, 220), 1, wxPENSTYLE_DOT));
        dc.DrawRoundedRectangle(sb, 4);

        int fontSize = std::max(7, int(OCXTheme::FontSize() * m_zoom) - 2);
        wxFont f(fontSize, wxFONTFAMILY_DEFAULT,
                 wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, OCXTheme::FontFace());
        dc.SetFont(f);
        dc.SetTextForeground(wxColour(180, 180, 180));
        wxString label = tmp.GetLabel();
        wxSize   tSz   = dc.GetTextExtent(label);
        dc.DrawText(label,
            sb.x + (sb.width  - tSz.GetWidth())  / 2,
            sb.y + (sb.height - tSz.GetHeight()) / 2);

        // Placement mode banner - top-left corner hint
        {
            Gate hintGate; hintGate.type = pendingType;
            wxString hint = "Placing: " + hintGate.GetLabel()
                + "  |  Click to place  |  Shift+Click for multiple  |  ESC to cancel";
            wxFont hf(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                      wxFONTWEIGHT_NORMAL, false, OCXTheme::FontFace());
            dc.SetFont(hf);
            wxSize  hsz = dc.GetTextExtent(hint);
            // Background pill
            dc.SetBrush(wxBrush(wxColour(30, 50, 70)));
            dc.SetPen(wxPen(wxColour(80, 140, 200), 1));
            dc.DrawRoundedRectangle(6, 6, hsz.GetWidth() + 16, hsz.GetHeight() + 8, 4);
            dc.SetTextForeground(wxColour(160, 200, 255));
            dc.DrawText(hint, 14, 10);
        }
    }

    // Zoom indicator (bottom-right)
    if (m_zoom != 1.0f)
    {
        wxString zoomStr = wxString::Format("%.0f%%", m_zoom * 100.0f);
        wxFont zf(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                  wxFONTWEIGHT_NORMAL, false, OCXTheme::FontFace());
        dc.SetFont(zf);
        dc.SetTextForeground(OCXTheme::FgDim());
        wxSize  sz     = dc.GetTextExtent(zoomStr);
        wxSize  client = GetClientSize();
        dc.DrawText(zoomStr, client.GetWidth() - sz.GetWidth() - 8,
                    client.GetHeight() - sz.GetHeight() - 4);
    }

    // Rubber-band selection rectangle
    if (m_rubberBand)
    {
        int rx = int(std::min(m_rubberStart.x, m_rubberCurrent.x) * m_zoom);
        int ry = int(std::min(m_rubberStart.y, m_rubberCurrent.y) * m_zoom);
        int rw = int(std::abs(m_rubberCurrent.x - m_rubberStart.x) * m_zoom);
        int rh = int(std::abs(m_rubberCurrent.y - m_rubberStart.y) * m_zoom);
        dc.SetBrush(wxBrush(wxColour(100, 160, 255, 30)));
        dc.SetPen(wxPen(wxColour(100, 160, 255), 1, wxPENSTYLE_DOT));
        dc.DrawRectangle(rx, ry, rw, rh);
    }
}

//--