#include "rtl_view_schematic_p.h"
#include <wx/dcgraph.h>

//** Schematic : events **//
void RTLViewPanel::Schematic::OnRightDown(wxMouseEvent& evt)
{
    m_panning   = true;
    m_hasPanned = false;
    m_panStart  = evt.GetPosition();
    m_panAccumX = 0;
    m_panAccumY = 0;
    CaptureMouse();
    SetCursor(wxCursor(wxCURSOR_HAND));
    evt.Skip();
}

void RTLViewPanel::Schematic::OnRightUp(wxMouseEvent& evt)
{
    if (m_panning)
    {
        m_panning = false;
        if (HasCapture()) ReleaseMouse();
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
    evt.Skip();
}

void RTLViewPanel::Schematic::OnMouseMove(wxMouseEvent& evt)
{
    if (!m_panning) { evt.Skip(); return; }

    wxPoint pos   = evt.GetPosition();
    wxPoint delta = pos - m_panStart;
    m_panStart    = pos;

    if (!m_hasPanned && (std::abs(delta.x) + std::abs(delta.y)) > 3)
        m_hasPanned = true;

    int sx, sy, ppuX, ppuY;
    GetViewStart(&sx, &sy);
    GetScrollPixelsPerUnit(&ppuX, &ppuY);

    m_panAccumX -= delta.x;
    m_panAccumY -= delta.y;

    int dsx = (ppuX > 0) ? m_panAccumX / ppuX : 0;
    int dsy = (ppuY > 0) ? m_panAccumY / ppuY : 0;
    m_panAccumX -= dsx * ppuX;
    m_panAccumY -= dsy * ppuY;

    Scroll(std::max(0, sx + dsx), std::max(0, sy + dsy));
}

void RTLViewPanel::Schematic::OnPaint(wxPaintEvent&)
{
    wxPaintDC paintDC(this);
    wxGCDC    gcdc(paintDC);
    wxDC&     dc = gcdc;

    dc.SetBackground(wxBrush(OCXTheme::BgPanel()));
    dc.Clear();

    if (!m_hasContent)
    {
        DrawEmptyState(dc, GetClientSize());
        return;
    }

    DoPrepareDC(gcdc);
    gcdc.SetUserScale(m_zoom, m_zoom);

    wxGraphicsContext* gc  = gcdc.GetGraphicsContext();
    wxSize             log(m_logW, m_logH);

    DrawEntity(dc, log);
    if (m_hasGates)
        DrawGateSchematic(dc, gc, log, GateTopY());
}

void RTLViewPanel::Schematic::OnMouseWheel(wxMouseEvent& evt)
{
    if (evt.GetModifiers() == wxMOD_CONTROL)
    {
        double factor = (evt.GetWheelRotation() > 0) ? 1.15 : (1.0 / 1.15);
        SetZoom(m_zoom * factor);
    }
    else
        evt.Skip();
}

void RTLViewPanel::Schematic::OnLeftClick(wxMouseEvent& evt)
{
    int sx, sy, ppuX, ppuY;
    GetViewStart(&sx, &sy);
    GetScrollPixelsPerUnit(&ppuX, &ppuY);
    double lx = (evt.GetX() + sx * ppuX) / m_zoom;
    double ly = (evt.GetY() + sy * ppuY) / m_zoom;

    const int GATE_W = 80, GATE_H = 50;
    int hit = -1;
    for (int i = 0; i < (int)m_gpos.size(); ++i)
    {
        if (lx >= m_gpos[i].x && lx <= m_gpos[i].x + GATE_W &&
            ly >= m_gpos[i].y && ly <= m_gpos[i].y + GATE_H)
        {
            hit = i;
            break;
        }
    }

    if (hit != m_selGate)
    {
        m_selGate = hit;
        m_owner->SyncTreeToGate(m_selGate);
        m_owner->UpdateInfoLabel();
        Refresh();
    }
    evt.Skip();
}

void RTLViewPanel::Schematic::OnContextMenu(wxContextMenuEvent&)
{
    // Right-click is used for panning; export is in the toolbar button.
    m_hasPanned = false;
}
