#include "waveform_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/dcbuffer.h>
#include <wx/filedlg.h>
#include <wx/file.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/grid.h>
#include <wx/menu.h>
#include <wx/splitter.h>
#include <functional>
#include <climits>
#include <set>
#include <map>

// Forward declaration so WfNamesPanel can hold a WfCanvas pointer
#include "ui/waveform/waveform_internal.h"

void WfNamesPanel::RenderToDC(wxDC& dc, int scrollY, int totalH) const
{
    dc.SetBackground(wxBrush(OCXTheme::BgOutput()));
    dc.Clear();

    if (!m_tracks || m_tracks->empty()) return;

    wxFont f(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
             wxFONTWEIGHT_NORMAL, false, "Consolas");
    dc.SetFont(f);

    int headerY = TIME_AXIS_H - scrollY;
    dc.SetBrush(wxBrush(OCXTheme::BgPanel()));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, NAMES_W, wxMax(0, wxMin(headerY, totalH)));

    int numSig = (int)m_tracks->size();
    for (int i = 0; i < numSig; ++i)
    {
        int rowY = headerY + i * ROW_H;
        if (rowY + ROW_H < 0 || rowY > totalH) continue;

        wxColour bg = (i % 2 == 0) ? OCXTheme::BgLineCur() : OCXTheme::BgOutput();
        dc.SetBrush(wxBrush(bg));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, rowY, NAMES_W, ROW_H);

        const WfSignal& sig = (*m_tracks)[i].signal;
        wxString label = sig.name;
        if (sig.width > 1)
            label += wxString::Format("[%d:0]", sig.width - 1);

        wxString valStr;
        if (m_cursorTime >= 0)
        {
            wxString v = (*m_tracks)[i].ValueAt(m_cursorTime);
            if (sig.width == 1)
                valStr = v;
            else
            {
                bool showBin = m_canvas && m_canvas->IsBusShowingBinary(i);
                if (v.StartsWith("b") || v.StartsWith("B"))
                    valStr = showBin ? v.Mid(1) : BinToHex(v, sig.width);
                else
                    valStr = v;
            }
        }

        // ---- Top half: signal name + cursor value ----
        // Name and cursor value sit in the upper ~18px of the row.
        const int nameY = rowY + 3;

        wxFont valFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                       wxFONTWEIGHT_BOLD, false, "Consolas");
        wxCoord vw = 0, vh = 0;
        if (!valStr.IsEmpty())
        {
            dc.SetFont(valFont);
            dc.GetTextExtent(valStr, &vw, &vh);
            vw += 6;
        }

        dc.SetFont(f);
        int maxNameW = NAMES_W - 12 - vw;
        wxCoord tw, th;
        dc.GetTextExtent(label, &tw, &th);
        while (tw > maxNameW && label.Length() > 4)
        {
            label = label.Left(label.Length() - 2);
            dc.GetTextExtent(label + "..", &tw, &th);
        }
        if (tw > maxNameW) label += "..";

        dc.SetTextForeground(OCXTheme::FgText());
        dc.DrawText(label, 8, nameY);

        if (!valStr.IsEmpty())
        {
            wxColour valCol = (valStr == "1" || valStr == "h" || valStr == "H")
                ? wxColour(80, 200, 80)
                : (valStr == "0" ? wxColour(200, 80, 80) : OCXTheme::FgDim());
            dc.SetFont(valFont);
            dc.SetTextForeground(valCol);
            dc.DrawText(valStr, NAMES_W - vw - 2, nameY);
        }

        // ---- Bottom half: value sequence ----
        // ROW_H=40; sequence text sits at approximately rowY+21.
        const int statsY  = rowY + 21;
        const int barX    = 8;
        const int barMaxW = NAMES_W - 16;

        wxFont statFont(7, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                        wxFONTWEIGHT_NORMAL, false, "Consolas");
        dc.SetFont(statFont);

        const WfTrack& trk = (*m_tracks)[i];

        // Build the value sequence string shown in the lower row area.
        // 1-bit:  "0 1 0 0 1 1 0 1 ..."   (space-separated 0/1/x/z)
        // Bus:    "00 FF 3C A0 ..."        (space-separated hex)
        wxString seq;
        int numChanges = (int)trk.changes.size();
        for (int k = 0; k < numChanges; ++k)
        {
            if (k > 0) seq += " ";
            const wxString& v = trk.changes[k].value;
            if (sig.width == 1)
            {
                seq += v; // "0", "1", "x", "z"
            }
            else
            {
                // Convert binary VCD value to hex; leave x/z as-is
                if (v.StartsWith("b") || v.StartsWith("B"))
                    seq += BinToHex(v, sig.width);
                else
                    seq += v;
            }
        }

        // Measure how many characters fit in barMaxW and truncate if needed
        wxString displayed = seq;
        wxCoord sw, sh;
        dc.GetTextExtent(displayed, &sw, &sh);
        if (sw > barMaxW && displayed.Length() > 3)
        {
            while (sw > barMaxW && displayed.Length() > 3)
            {
                displayed = displayed.Left(displayed.Length() - 1);
                dc.GetTextExtent(displayed + "...", &sw, &sh);
            }
            displayed += "...";
        }

        dc.SetTextForeground(sig.width == 1 ? wxColour(100, 200, 100) : wxColour(100, 160, 220));
        dc.DrawText(displayed, barX, statsY);

        dc.SetPen(wxPen(OCXTheme::BgSash()));
        dc.DrawLine(0, rowY + ROW_H - 1, NAMES_W, rowY + ROW_H - 1);
    }

    // Right border
    dc.SetPen(wxPen(OCXTheme::BgSash()));
    dc.DrawLine(NAMES_W - 1, 0, NAMES_W - 1, totalH);
}

int WfNamesPanel::RowAtY(int y) const
{
    int scrollY = m_canvas ? m_canvas->GetScrollPos(wxVERTICAL) : 0;
    int headerH = TIME_AXIS_H - scrollY;
    int row = (y - headerH) / ROW_H;
    if (!m_tracks || row < 0 || row >= (int)m_tracks->size()) return -1;
    return row;
}

void WfNamesPanel::OnRightClick(wxMouseEvent& event)
{
    int row = RowAtY(event.GetY());
    if (row < 0) return;

    const wxString& name = (*m_tracks)[row].signal.name;

    wxMenu menu;
    menu.Append(1, "Add to Watch: " + name);
    menu.Append(2, "Remove from view");

    const WfSignal& sig = (*m_tracks)[row].signal;
    if (sig.width > 1)
    {
        menu.AppendSeparator();
        bool isBin = m_canvas && m_canvas->IsBusShowingBinary(row);
        menu.Append(3, isBin ? "Show as Hex" : "Show as Binary");
    }

    int choice = GetPopupMenuSelectionFromUser(menu, event.GetPosition());
    if (choice == 1 && m_onWatch)  m_onWatch(name);
    if (choice == 2 && m_onRemove) m_onRemove(row);
    if (choice == 3 && m_canvas)   m_canvas->ToggleBusFormat(row);
}

void WfNamesPanel::OnDblClick(wxMouseEvent& event)
{
    int row = RowAtY(event.GetY());
    if (row < 0 || !m_onWatch) return;
    m_onWatch((*m_tracks)[row].signal.name);
}

void WfNamesPanel::OnLeftDown(wxMouseEvent& event)
{
    int row = RowAtY(event.GetY());
    if (row < 0) { event.Skip(); return; }
    m_dragRow  = row;
    m_dropRow  = row;
    m_dragging = false;
    CaptureMouse();
}

void WfNamesPanel::OnMotion(wxMouseEvent& event)
{
    if (m_dragRow < 0) { event.Skip(); return; }
    m_dragging = true;

    // Compute drop row from current Y, clamped to valid range
    int scrollY = m_canvas ? m_canvas->GetScrollPos(wxVERTICAL) : 0;
    int headerH = TIME_AXIS_H - scrollY;
    int numRows = m_tracks ? (int)m_tracks->size() : 0;
    int row = (event.GetY() - headerH) / ROW_H;
    if (row < 0)        row = 0;
    if (row >= numRows) row = wxMax(0, numRows - 1);

    if (row != m_dropRow)
    {
        m_dropRow = row;
        Refresh();
    }
    event.Skip();
}

void WfNamesPanel::OnLeftUp(wxMouseEvent& event)
{
    if (HasCapture()) ReleaseMouse();
    bool doReorder = m_dragging && m_dropRow >= 0 && m_dropRow != m_dragRow && m_onReorder;
    int from = m_dragRow, to = m_dropRow;
    m_dragRow  = -1;
    m_dropRow  = -1;
    m_dragging = false;
    Refresh();
    if (doReorder) m_onReorder(from, to);
    event.Skip();
}

//--
// WfCanvas::OnDraw
//--
void WfCanvas::OnDraw(wxDC& dc)
{
    // Empty state
    if (!m_tracks || m_tracks->empty())
    {
        dc.SetBackground(wxBrush(OCXTheme::BgOutput()));
        dc.Clear();
        dc.SetTextForeground(OCXTheme::FgDim());
        wxFont f(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                 wxFONTWEIGHT_NORMAL, false, "Consolas");
        dc.SetFont(f);
        dc.DrawText("No waveform loaded.", 20, 20);
        dc.DrawText("Run a simulation to generate a .vcd file,", 20, 44);
        dc.DrawText("or use the Load VCD button.", 20, 68);
        return;
    }

    long long effEnd = EffectiveEndTime();
    int canvasW = TimeToX(effEnd) + 80;
    int numSig  = (int)m_tracks->size();
    int canvasH = TIME_AXIS_H + numSig * ROW_H + 40;

    // ---- Background --------------------------------------------------------
    // Paint the full logical area (canvas + any client overflow) so no bleed-
    // through occurs from behind the scrolled window (wxBG_STYLE_PAINT).
    {
        int scrollX, scrollY, unitX = 1, unitY = 1;
        GetViewStart(&scrollX, &scrollY);
        GetScrollPixelsPerUnit(&unitX, &unitY);
        wxSize cs = GetClientSize();
        int fillW = wxMax(canvasW, scrollX * unitX + cs.GetWidth());
        int fillH = wxMax(canvasH, scrollY * unitY + cs.GetHeight());
        dc.SetBrush(wxBrush(OCXTheme::BgOutput()));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, fillW, fillH);
    }

    // ---- Alternating row bands and separators ------------------------------
    // Use fillW so bands span to the edge on wide windows
    {
        int scrollX, scrollY, unitX = 1, unitY = 1;
        GetViewStart(&scrollX, &scrollY);
        GetScrollPixelsPerUnit(&unitX, &unitY);
        wxSize cs = GetClientSize();
        int fillW = wxMax(canvasW, scrollX * unitX + cs.GetWidth());

        for (int i = 0; i < numSig; ++i)
        {
            int rowY = TIME_AXIS_H + i * ROW_H;
            if (i % 2 == 0)
            {
                dc.SetBrush(wxBrush(OCXTheme::BgLineCur()));
                dc.DrawRectangle(0, rowY, fillW, ROW_H);
            }
            dc.SetPen(wxPen(OCXTheme::BgSash()));
            dc.DrawLine(0, rowY + ROW_H - 1, fillW, rowY + ROW_H - 1);
        }
    }

    // ---- Time axis + grid lines --------------------------------------------

    DrawTimeAxis(dc, canvasW, canvasH);

    // ---- Waveforms ---------------------------------------------------------

    wxFont sigFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                   wxFONTWEIGHT_NORMAL, false, "Consolas");
    dc.SetFont(sigFont);

    for (int i = 0; i < numSig; ++i)
    {
        int rowY = TIME_AXIS_H + i * ROW_H;
        const WfTrack& track = (*m_tracks)[i];

        if (track.signal.width == 1)
            DrawSignal1bit(dc, track, rowY);
        else
            DrawSignalBus(dc, track, rowY, i);
    }

    // ---- Named markers -----------------------------------------------------

    {
        wxFont mFont(7, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                     wxFONTWEIGHT_BOLD, false, "Consolas");
        dc.SetFont(mFont);
        wxColour markerCol(255, 140, 40); // amber
        for (const auto& kv : m_markers)
        {
            int mx = TimeToX(kv.first);
            dc.SetPen(wxPen(markerCol, 1, wxPENSTYLE_DOT));
            dc.DrawLine(mx, 0, mx, canvasH);
            // Label above time axis
            dc.SetTextForeground(markerCol);
            wxCoord tw, th;
            dc.GetTextExtent(kv.second, &tw, &th);
            dc.DrawText(kv.second, mx + 2, 2);
        }
    }

    // ---- Reference line (delta time) ---------------------------------------

    if (m_refTime >= 0)
    {
        int rx = TimeToX(m_refTime);
        wxColour refCol(40, 200, 80); // green
        dc.SetPen(wxPen(refCol, 1, wxPENSTYLE_DOT));
        dc.DrawLine(rx, 0, rx, canvasH);
        // Small "REF" tag at top
        wxFont mFont(7, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                     wxFONTWEIGHT_BOLD, false, "Consolas");
        dc.SetFont(mFont);
        dc.SetTextForeground(refCol);
        dc.DrawText("REF", rx + 2, TIME_AXIS_H - 14);
    }

    // ---- Cursor ------------------------------------------------------------

    if (m_cursorTime >= 0)
    {
        int cx = TimeToX(m_cursorTime);
        dc.SetPen(wxPen(WF_COLOR_CURSOR(), 1, wxPENSTYLE_SHORT_DASH));
        dc.DrawLine(cx, 0, cx, canvasH);
    }
}

//--
// WfCanvas::DrawTimeAxis
//--
void WfCanvas::DrawTimeAxis(wxDC& dc, int canvasW, int canvasH)
{
    long long effEnd = EffectiveEndTime();

    // Pick a tick interval that gives ~8 ticks across the visible client width
    int clientW = GetClientSize().GetWidth();
    double timePerPx   = 1.0 / m_pixelsPerUnit;
    double timePerView = clientW * timePerPx;
    long long interval = NiceInterval((long long)(timePerView / 8.0));
    if (interval <= 0) interval = 1;

    // ---- Vertical grid lines through signal rows ---------------------------
    // Drawn first so everything else renders on top.
    {
        wxColour gridCol(34, 40, 34); // very subtle tinted-green dark line
        dc.SetPen(wxPen(gridCol, 1));
        for (long long t = interval; t <= effEnd + interval; t += interval)
        {
            int x = TimeToX(t);
            if (x > canvasW) break;
            dc.DrawLine(x, TIME_AXIS_H, x, canvasH);
        }
    }

    // ---- Axis header background --------------------------------------------
    dc.SetBrush(wxBrush(OCXTheme::BgPanel()));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, canvasW, TIME_AXIS_H);

    // Baseline
    dc.SetPen(wxPen(OCXTheme::BgSash()));
    dc.DrawLine(0, TIME_AXIS_H - 1, canvasW, TIME_AXIS_H - 1);

    // ---- Tick marks and labels ---------------------------------------------
    wxFont f(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
             wxFONTWEIGHT_NORMAL, false, "Consolas");
    dc.SetFont(f);
    dc.SetTextForeground(OCXTheme::FgDim());
    dc.SetPen(wxPen(OCXTheme::FgDim()));

    bool firstLabel = true;
    for (long long t = 0; t <= effEnd + interval; t += interval)
    {
        int x = TimeToX(t);
        if (x > canvasW) break;

        // Tick mark
        dc.DrawLine(x, TIME_AXIS_H - 7, x, TIME_AXIS_H - 1);

        // Label - use display unit if set, otherwise raw integer
        wxString label;
        if (!m_displayUnit.IsEmpty())
        {
            double v = (double)t / m_displayDivisor;
            if (v == (long long)v)
                label = wxString::Format("%lld", (long long)v);
            else
                label = wxString::Format("%.2f", v);
            if (firstLabel)
                label += " " + m_displayUnit;
        }
        else
        {
            label = wxString::Format("%lld", t);
            if (firstLabel && !m_timescale.IsEmpty())
                label += " " + m_timescale;
        }
        firstLabel = false;

        wxCoord tw, th;
        dc.GetTextExtent(label, &tw, &th);
        int lx = x - tw / 2;
        if (lx >= 0)
            dc.DrawText(label, lx, 4);
    }
}

//--
// WfCanvas::DrawSignal1bit
//--
void WfCanvas::DrawSignal1bit(wxDC& dc, const WfTrack& track, int rowY)
{
    if (track.changes.empty()) return;

    const int yHigh = rowY + SIG_PAD;
    const int yLow  = rowY + ROW_H - SIG_PAD;

    dc.SetPen(wxPen(WF_COLOR_1BIT(), 1));

    // Small font for "1"/"0" labels
    wxFont lblFont(7, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                   wxFONTWEIGHT_NORMAL, false, "Consolas");
    dc.SetFont(lblFont);
    dc.SetTextForeground(WF_COLOR_1BIT());

    wxCoord charW = 0, charH = 0;
    dc.GetTextExtent("0", &charW, &charH);

    long long effEnd = EffectiveEndTime();
    int numChanges = (int)track.changes.size();
    for (int j = 0; j < numChanges; ++j)
    {
        long long t0  = track.changes[j].time;
        long long t1  = (j + 1 < numChanges) ? track.changes[j + 1].time
                                              : effEnd;

        int x0 = TimeToX(t0);
        int x1 = TimeToX(t1);

        bool hi  = IsHigh(track.changes[j].value);
        int  y   = hi ? yHigh : yLow;
        int  segW = x1 - x0;

        // Fill HIGH region with dark green - instantly shows signal state
        if (hi && segW > 0)
        {
            dc.SetBrush(wxBrush(wxColour(18, 58, 26)));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(x0, yHigh + 1, segW, yLow - yHigh - 1);
            dc.SetPen(wxPen(WF_COLOR_1BIT(), 1)); // restore waveform pen
        }

        // Horizontal segment line (drawn on top of fill)
        dc.DrawLine(x0, y, x1, y);

        // Value label centred in segment when there is enough room
        if (segW > charW + 4)
        {
            wxString lbl = hi ? "1" : "0";
            int textX = x0 + (segW - charW) / 2;
            int textY = hi ? (yHigh + 2) : (yLow - charH - 2);
            if (textY < rowY) textY = rowY + 1;
            if (textY + charH > rowY + ROW_H) textY = rowY + ROW_H - charH - 1;
            dc.DrawText(lbl, textX, textY);
        }

        // Vertical transition at x1
        if (j + 1 < numChanges)
        {
            bool nextHi = IsHigh(track.changes[j + 1].value);
            int  nextY  = nextHi ? yHigh : yLow;
            if (y != nextY)
                dc.DrawLine(x1, y, x1, nextY);
        }
    }
}

//--
// WfCanvas::DrawSignalBus
//--
void WfCanvas::DrawSignalBus(wxDC& dc, const WfTrack& track, int rowY, int row)
{
    if (track.changes.empty()) return;

    const int yTop  = rowY + SIG_PAD;
    const int yBot  = rowY + ROW_H - SIG_PAD;
    const int yMid  = rowY + ROW_H / 2;
    const int slope = 5;

    dc.SetPen(wxPen(WF_COLOR_BUS(), 1));
    dc.SetTextForeground(OCXTheme::FgText());

    long long effEnd = EffectiveEndTime();
    int numChanges = (int)track.changes.size();
    for (int j = 0; j < numChanges; ++j)
    {
        long long t0 = track.changes[j].time;
        long long t1 = (j + 1 < numChanges) ? track.changes[j + 1].time
                                             : effEnd;

        int x0 = TimeToX(t0);
        int x1 = TimeToX(t1);
        if (x1 <= x0) continue;

        int ix0 = x0 + slope;
        int ix1 = x1 - slope;

        if (ix0 >= ix1)
        {
            // Too narrow: draw an X
            dc.DrawLine(x0, yTop, x1, yBot);
            dc.DrawLine(x0, yBot, x1, yTop);
            continue;
        }

        // Top + bottom lines
        dc.DrawLine(ix0, yTop, ix1, yTop);
        dc.DrawLine(ix0, yBot, ix1, yBot);

        // Left diagonal
        dc.DrawLine(x0, yMid, ix0, yTop);
        dc.DrawLine(x0, yMid, ix0, yBot);

        // Right diagonal
        dc.DrawLine(x1, yMid, ix1, yTop);
        dc.DrawLine(x1, yMid, ix1, yBot);

        // Value label (hex or binary depending on per-track format toggle)
        wxString val = track.changes[j].value;
        wxString hexVal;
        bool showBin = IsBusShowingBinary(row);
        if (val.StartsWith("b") || val.StartsWith("B"))
            hexVal = showBin ? val.Mid(1) : BinToHex(val, track.signal.width);
        else
            hexVal = val; // x, z etc.

        wxCoord tw, th;
        dc.GetTextExtent(hexVal, &tw, &th);
        int textX = (x0 + x1) / 2 - tw / 2;
        if (textX >= ix0 && textX + tw <= ix1)
            dc.DrawText(hexVal, textX, yMid - th / 2);
    }
}

//--
// WfNamesPanel::OnPaint
//--
void WfNamesPanel::OnPaint(wxPaintEvent&)
{
    wxBufferedPaintDC dc(this);
    wxSize clientSz = GetClientSize();

    int scrollY = m_canvas ? m_canvas->GetScrollPos(wxVERTICAL) : 0;
    RenderToDC(dc, scrollY, clientSz.GetHeight());

    // Drag-to-reorder feedback - drawn on top
    if (m_dragging && m_dragRow >= 0)
    {
        int headerY = TIME_AXIS_H - scrollY;
        int dragY = headerY + m_dragRow * ROW_H;
        dc.SetBrush(wxBrush(wxColour(80, 130, 200)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, dragY, NAMES_W, ROW_H);

        if (m_dropRow >= 0)
        {
            int lineY = headerY + m_dropRow * ROW_H;
            dc.SetPen(wxPen(wxColour(80, 160, 255), 2));
            dc.DrawLine(0, lineY, NAMES_W - 1, lineY);
        }
    }
}

