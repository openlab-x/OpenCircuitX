#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <functional>
#include <vector>
#include <climits>
#include <set>
#include <map>
#include "core/simulation/vcd_parser.h"
#include "ui/shell/app_theme.h"

class WfCanvas;

//--
// Layout constants
//--
static const int ROW_H       = 40;   // pixels per signal row
static const int TIME_AXIS_H = 28;   // pixels for time ruler at top
static const int SIG_PAD     = 9;    // padding inside each row (high/low offset)
static const int NAMES_W     = 190;  // width of signal-name column

// Waveform colors
static wxColour WF_COLOR_1BIT()  { return wxColour(100, 200, 120); } // green for 1-bit
static wxColour WF_COLOR_BUS()   { return wxColour( 86, 156, 214); } // blue  for bus
static wxColour WF_COLOR_CURSOR(){ return wxColour(255, 200,  60); } // yellow cursor

//--
// Helpers
//--
// Pick a "nice" interval (1, 2, 5, 10, 20, 50, ...) >= rough
static long long NiceInterval(long long rough)
{
    if (rough <= 1) return 1;
    long long base = 1;
    while (base * 10 <= rough) base *= 10;
    if (rough <= base)     return base;
    if (rough <= base * 2) return base * 2;
    if (rough <= base * 5) return base * 5;
    return base * 10;
}

// Convert a binary string (with or without leading 'b') to hex
static wxString BinToHex(const wxString& raw, int width)
{
    wxString b = raw.StartsWith("b") || raw.StartsWith("B") ? raw.Mid(1) : raw;

    // Pad to full width
    while ((int)b.Length() < width)
        b = "0" + b;

    wxString result;
    int len = (int)b.Length();
    int start = len % 4;
    if (start > 0)
    {
        int val = 0;
        for (int k = 0; k < start; ++k)
            val = val * 2 + (b[k] == '1' ? 1 : 0);
        result += wxString::Format("%X", val);
    }
    for (int i = start; i < len; i += 4)
    {
        int val = 0;
        for (int k = 0; k < 4; ++k)
            val = val * 2 + (b[i + k] == '1' ? 1 : 0);
        result += wxString::Format("%X", val);
    }
    return result.IsEmpty() ? "0" : result;
}

static bool IsHigh(const wxString& val)
{
    return val == "1" || val == "h" || val == "H";
}

//--
// WfNamesPanel - left column showing signal names
//--
class WfNamesPanel : public wxPanel
{
public:
    WfNamesPanel(wxWindow* parent, WfCanvas* canvas)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(NAMES_W, -1))
        , m_canvas(canvas)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT,       &WfNamesPanel::OnPaint,      this);
        Bind(wxEVT_RIGHT_DOWN,  &WfNamesPanel::OnRightClick, this);
        Bind(wxEVT_LEFT_DCLICK, &WfNamesPanel::OnDblClick,   this);
        Bind(wxEVT_LEFT_DOWN,   &WfNamesPanel::OnLeftDown,   this);
        Bind(wxEVT_MOTION,      &WfNamesPanel::OnMotion,     this);
        Bind(wxEVT_LEFT_UP,     &WfNamesPanel::OnLeftUp,     this);
    }

    void SetTracks(const std::vector<WfTrack>* tracks) { m_tracks = tracks; }

    void UpdateCursorTime(long long t) { m_cursorTime = t; Refresh(); }

    void SetRemoveCallback(std::function<void(int)> cb)              { m_onRemove  = cb; }
    void SetWatchCallback(std::function<void(const wxString&)> cb)   { m_onWatch   = cb; }
    void SetReorderCallback(std::function<void(int,int)> cb)         { m_onReorder = cb; }

    // Render the names panel into any DC - used by PNG export.
    // scrollY: vertical scroll offset in pixels (same unit as WfCanvas scrollY).
    // totalH:  total height of the destination bitmap.
    // Defined after WfCanvas is fully declared (needs IsBusShowingBinary).
    void RenderToDC(wxDC& dc, int scrollY, int totalH) const;

private:
    const std::vector<WfTrack>* m_tracks     = nullptr;
    WfCanvas*                   m_canvas;
    long long                   m_cursorTime = -1;

    std::function<void(int)>             m_onRemove;
    std::function<void(const wxString&)> m_onWatch;
    std::function<void(int,int)>         m_onReorder;

    // Drag-to-reorder state
    int  m_dragRow  = -1;   // row the user started dragging
    int  m_dropRow  = -1;   // row currently hovered (insertion target)
    bool m_dragging = false; // motion threshold crossed

    int RowAtY(int y) const;        // defined after WfCanvas is fully declared
    void OnRightClick(wxMouseEvent& event); // defined after WfCanvas is fully declared
    void OnDblClick(wxMouseEvent& event);   // defined after WfCanvas is fully declared
    void OnLeftDown(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);

    void OnPaint(wxPaintEvent& event);
};

//--
// WfCanvas - scrollable waveform drawing area
//--
class WfCanvas : public wxScrolledWindow
{
public:
    WfCanvas(wxWindow* parent, WfNamesPanel* names)
        : wxScrolledWindow(parent, wxID_ANY,
                           wxDefaultPosition, wxDefaultSize,
                           wxHSCROLL | wxVSCROLL | wxBORDER_NONE)
        , m_names(names)
        , m_pixelsPerUnit(1.0)
        , m_endTime(0)
        , m_cursorTime(-1)
        , m_tracks(nullptr)
        , m_timeLabel(nullptr)
        , m_timescale("")
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetScrollRate(1, 1);

        Bind(wxEVT_LEFT_DOWN,    &WfCanvas::OnLeftDown,    this);
        Bind(wxEVT_RIGHT_DOWN,   &WfCanvas::OnRightDown,   this);
        Bind(wxEVT_MOUSEWHEEL,   &WfCanvas::OnWheel,       this);
        Bind(wxEVT_SIZE,         &WfCanvas::OnSize,        this);
        Bind(wxEVT_MOTION,       &WfCanvas::OnMouseHover,  this);
        Bind(wxEVT_LEAVE_WINDOW, &WfCanvas::OnMouseLeave,  this);
    }

    void SetData(const std::vector<WfTrack>* tracks,
                 long long endTime,
                 const wxString& timescale)
    {
        m_tracks    = tracks;
        m_endTime   = endTime;
        m_timescale = timescale;
        m_cursorTime = -1;
        m_refTime    = -1;
        UpdateLayout();
    }

    void SetTimeLabelCtrl(wxStaticText* lbl)  { m_timeLabel  = lbl; }
    void SetHoverLabelCtrl(wxStaticText* lbl) { m_hoverLabel = lbl; }
    void SetCursorCallback(std::function<void(long long)> cb) { m_cursorCallback = cb; }

    // Move cursor by delta time units (positive = forward, negative = back).
    void StepCursor(long long delta)
    {
        if (!m_tracks || m_tracks->empty()) return;
        m_cursorTime += delta;
        if (m_cursorTime < 0)         m_cursorTime = 0;
        if (m_cursorTime > m_endTime) m_cursorTime = m_endTime;
        UpdateTimeLabelAndCallback();
        Refresh();
    }

    // Jump cursor to the next signal transition after current time.
    void StepToNextTransition()
    {
        if (!m_tracks || m_tracks->empty()) return;
        long long best = LLONG_MAX;
        for (const WfTrack& t : *m_tracks)
            for (const WfChange& c : t.changes)
                if (c.time > m_cursorTime && c.time < best)
                    best = c.time;
        if (best != LLONG_MAX)
        {
            m_cursorTime = best;
            UpdateTimeLabelAndCallback();
            Refresh();
        }
    }

    // Jump cursor to the previous signal transition before current time.
    void StepToPrevTransition()
    {
        if (!m_tracks || m_tracks->empty()) return;
        long long best = LLONG_MIN;
        for (const WfTrack& t : *m_tracks)
            for (const WfChange& c : t.changes)
                if (c.time < m_cursorTime && c.time > best)
                    best = c.time;
        if (best != LLONG_MIN)
        {
            m_cursorTime = best;
            UpdateTimeLabelAndCallback();
            Refresh();
        }
    }

    long long GetCursorTime()      const { return m_cursorTime; }
    long long GetEndTime()         const { return m_endTime; }
    wxString  GetDisplayUnit()     const { return m_displayUnit; }
    double    GetDisplayDivisor()  const { return m_displayDivisor > 0 ? m_displayDivisor : 1.0; }

    void FitToWindow()
    {
        if (!m_tracks || m_tracks->empty()) return;
        int w = GetClientSize().GetWidth();
        if (w <= 4) return;   // layout not settled yet - Reload() defers via CallAfter
        // Compute exact ppu so the whole waveform fills the visible width.
        // No lower clamp - the waveform can be arbitrarily long.
        m_pixelsPerUnit = (double)(w - 4) / (double)EffectiveEndTime();
        if (m_pixelsPerUnit < 1e-9) m_pixelsPerUnit = 1e-9;
        m_fitted = true;
        UpdateLayout();
        Scroll(0, wxDefaultCoord);   // always start from t=0 after a fit
    }

    void ZoomIn()
    {
        ZoomAroundScreenX(2.0, VisibleAnchorScreenX());
    }

    void ZoomOut()
    {
        ZoomAroundScreenX(0.5, VisibleAnchorScreenX());
    }

    double GetPixelsPerUnit() const { return m_pixelsPerUnit; }

    void SetPixelsPerUnit(double ppu)
    {
        if (ppu > 0.0) m_pixelsPerUnit = ppu;
        UpdateLayout();
    }

    void SetCursorTimeDirect(long long t)
    {
        m_cursorTime = t;
        UpdateTimeLabelAndCallback();
        Refresh();
    }

    void ClearMarkers() { m_markers.clear(); Refresh(); }

    void SetMarkers(const std::map<long long, wxString>& markers)
    {
        m_markers = markers;
        Refresh();
    }

    // Control how raw VCD time units are formatted in labels.
    // divisor: raw units per display unit (1=raw, 1000=ns if VCD is ps, etc.)
    // unit: suffix string shown in labels ("ps","ns","us","ms","")
    void SetDisplayUnit(double divisor, const wxString& unit)
    {
        m_displayDivisor = (divisor > 0.0) ? divisor : 1.0;
        m_displayUnit    = unit;
        UpdateTimeLabelAndCallback();
        Refresh();
    }

protected:
    void OnDraw(wxDC& dc) override;

private:
    WfNamesPanel*               m_names;
    const std::vector<WfTrack>* m_tracks;
    long long                   m_endTime;
    wxString                    m_timescale;
    double                      m_pixelsPerUnit;
    long long                   m_cursorTime;
    wxStaticText*               m_timeLabel  = nullptr;
    wxStaticText*               m_hoverLabel = nullptr;
    long long                   m_refTime    = -1;  // delta-time reference (-1 = not set)
    bool                        m_fitted     = false; // true while Fit is active; re-fit on resize
    double                      m_displayDivisor = 1.0; // raw units per display unit
    wxString                    m_displayUnit;           // "ps","ns","us","ms", or ""

    std::function<void(long long)> m_cursorCallback;

    // Per-track display format for bus signals: false = hex (default), true = binary
    std::map<int, bool> m_showBinary;

    // Named time markers: time → label
    std::map<long long, wxString> m_markers;

public:
    bool IsBusShowingBinary(int row) const
    {
        auto it = m_showBinary.find(row);
        return it != m_showBinary.end() && it->second;
    }
    void ToggleBusFormat(int row)
    {
        m_showBinary[row] = !IsBusShowingBinary(row);
        Refresh();
    }

    // Find the next time after `afterTime` where the signal at `trackIdx`
    // has the given value.  Returns -1 if not found.
    long long FindNextValue(int trackIdx, const wxString& value, long long afterTime) const
    {
        if (!m_tracks || trackIdx < 0 || trackIdx >= (int)m_tracks->size())
            return -1;
        const auto& changes = (*m_tracks)[trackIdx].changes;
        for (const auto& c : changes)
            if (c.time > afterTime && c.value == value)
                return c.time;
        return -1;
    }

    // Find the previous time before `beforeTime` where the signal at `trackIdx`
    // has the given value.  Returns -1 if not found.
    long long FindPrevValue(int trackIdx, const wxString& value, long long beforeTime) const
    {
        if (!m_tracks || trackIdx < 0 || trackIdx >= (int)m_tracks->size())
            return -1;
        const auto& changes = (*m_tracks)[trackIdx].changes;
        long long result = -1;
        for (const auto& c : changes)
        {
            if (c.time >= beforeTime) break;
            if (c.value == value)    result = c.time;
        }
        return result;
    }

    void AddMarker(long long time, const wxString& label)
    {
        m_markers[time] = label;
        Refresh();
    }
    void RemoveMarkersNear(long long time, long long tolerance)
    {
        for (auto it = m_markers.begin(); it != m_markers.end(); )
        {
            if (std::abs(it->first - time) <= tolerance)
                it = m_markers.erase(it);
            else
                ++it;
        }
        Refresh();
    }
    const std::map<long long, wxString>& GetMarkers() const { return m_markers; }

    // Export combined waveform (names panel + canvas) to a PNG file.
    // namesPanel may be nullptr, in which case only the canvas is exported.
    bool ExportPNG(const wxString& filePath, WfNamesPanel* namesPanel = nullptr) const
    {
        wxSize canvasSz = GetClientSize();
        int namesW = namesPanel ? NAMES_W : 0;
        int totalW = namesW + canvasSz.GetWidth();
        int totalH = canvasSz.GetHeight();

        wxBitmap bmp(totalW, totalH);
        wxMemoryDC mdc(bmp);
        mdc.SetBackground(wxBrush(OCXTheme::BgOutput()));
        mdc.Clear();

        // Names panel (left portion)
        if (namesPanel)
        {
            wxBitmap namesBmp(namesW, totalH);
            wxMemoryDC namesDC;
            namesDC.SelectObject(namesBmp);
            int scrollY = GetScrollPos(wxVERTICAL);
            namesPanel->RenderToDC(namesDC, scrollY, totalH);
            namesDC.SelectObject(wxNullBitmap);
            mdc.DrawBitmap(namesBmp, 0, 0, false);
        }

        // Waveform canvas (right portion)
        int scrollX = GetScrollPos(wxHORIZONTAL);
        int scrollY = GetScrollPos(wxVERTICAL);
        mdc.SetDeviceOrigin(namesW - scrollX, -scrollY);
        const_cast<WfCanvas*>(this)->OnDraw(mdc);
        mdc.SetDeviceOrigin(0, 0);
        mdc.SelectObject(wxNullBitmap);

        return bmp.SaveFile(filePath, wxBITMAP_TYPE_PNG);
    }

    // Return time at a canvas x coordinate accounting for scroll
    long long XToTimeFromScreen(int screenX) const
    {
        int lx, ly;
        const_cast<WfCanvas*>(this)->CalcUnscrolledPosition(screenX, 0, &lx, &ly);
        return XToTime(lx);
    }

private:
    // Simulation end time extended by a 5% right margin so the last value
    // change always has some visible horizontal extent.  Everything that
    // determines canvas width and waveform drawing length uses this value.
    long long EffectiveEndTime() const
    {
        long long t = m_endTime > 0 ? m_endTime : 100;
        return t + wxMax(t / 20, (long long)1);
    }

    int  TimeToX(long long t)  const { return (int)(t * m_pixelsPerUnit); }
    long long XToTime(int x)   const
    {
        double t = x / m_pixelsPerUnit;
        return (long long)(t < 0 ? 0 : t);
    }

    void UpdateTimeLabelAndCallback()
    {
        if (m_timeLabel && m_cursorTime >= 0)
        {
            wxString unitStr;
            wxString valStr;
            if (!m_displayUnit.IsEmpty())
            {
                double v = (double)m_cursorTime / m_displayDivisor;
                valStr  = wxString::Format("%.3f", v);
                unitStr = " " + m_displayUnit;
            }
            else
            {
                valStr  = wxString::Format("%lld", m_cursorTime);
                unitStr = m_timescale.IsEmpty() ? "" : " " + m_timescale;
            }
            wxString label = "T = " + valStr + unitStr;

            // Append ΔT if a reference is set
            if (m_refTime >= 0)
            {
                long long delta = m_cursorTime - m_refTime;
                wxString dStr;
                if (!m_displayUnit.IsEmpty())
                    // Delta (U+0394) via explicit UTF-8 bytes, not a \uXXXX
                    // escape in a narrow literal - same mojibake risk as
                    // the About dialog's middle dot, the escape's byte
                    // encoding depends on the compiler's execution
                    // charset, which doesn't reliably agree with wx's
                    // runtime UTF-8 decode.
                    dStr = "  " + wxString::FromUTF8("\xCE\x94")
                         + wxString::Format("T = %.3f%s", (double)delta / m_displayDivisor, " " + m_displayUnit);
                else
                    dStr = wxString::Format("  dT = %lld%s", delta, unitStr);
                label += dStr;
            }

            m_timeLabel->SetLabel(label);
            m_timeLabel->GetParent()->Layout();
        }
        if (m_cursorCallback)
            m_cursorCallback(m_cursorTime);
        // Update value display in names panel
        if (m_names)
            m_names->UpdateCursorTime(m_cursorTime);
    }

    void UpdateLayout()
    {
        if (!m_tracks)
        {
            SetVirtualSize(GetClientSize());
            Refresh();
            return;
        }
        // Ensure canvas is at least as wide as the client so background always fills
        int clientW = GetClientSize().GetWidth();
        int canvasW = wxMax(TimeToX(EffectiveEndTime()) + 80, clientW);
        int canvasH = TIME_AXIS_H + (int)m_tracks->size() * ROW_H + 40;
        SetVirtualSize(canvasW, canvasH);
        Refresh();
        if (m_names) m_names->Refresh();
    }

    void DrawTimeAxis(wxDC& dc, int canvasW, int canvasH);
    void DrawSignal1bit(wxDC& dc, const WfTrack& track, int rowY);
    void DrawSignalBus(wxDC& dc, const WfTrack& track, int rowY, int row);

    // Called by wxScrolledWindow on every scroll - reliable hook for sync
    void ScrollWindow(int dx, int dy, const wxRect* rect = nullptr) override
    {
        wxScrolledWindow::ScrollWindow(dx, dy, rect);
        if (m_names) m_names->Refresh();
    }

    void OnSize(wxSizeEvent& event)
    {
        event.Skip();
        // Re-fit whenever the panel is resized while Fit mode is active
        if (m_fitted && m_tracks && !m_tracks->empty())
            FitToWindow();
        if (m_names) m_names->Refresh();
    }

    void OnLeftDown(wxMouseEvent& event)
    {
        int lx, ly;
        CalcUnscrolledPosition(event.GetX(), event.GetY(), &lx, &ly);
        m_cursorTime = XToTime(lx);
        if (m_cursorTime < 0)         m_cursorTime = 0;
        if (m_cursorTime > m_endTime) m_cursorTime = m_endTime;
        UpdateTimeLabelAndCallback();
        Refresh();
    }

    void OnMouseHover(wxMouseEvent& event)
    {
        if (!m_hoverLabel || !m_tracks || m_tracks->empty())
            return;

        int lx, ly;
        CalcUnscrolledPosition(event.GetX(), event.GetY(), &lx, &ly);

        // Determine signal row from unscrolled Y
        int row = (ly - TIME_AXIS_H) / ROW_H;
        if (row < 0 || row >= (int)m_tracks->size())
        {
            m_hoverLabel->SetLabel("");
            return;
        }

        long long t = XToTime(lx);
        const WfTrack& track = (*m_tracks)[row];
        wxString value = track.ValueAt(t);

        wxString display;
        if (track.signal.width > 1)
            display = wxString::Format("%s = 0x%s", track.signal.name, BinToHex(value, track.signal.width));
        else
            display = wxString::Format("%s = %s", track.signal.name, value);

        m_hoverLabel->SetLabel(display);
        event.Skip();
    }

    void OnMouseLeave(wxMouseEvent& event)
    {
        if (m_hoverLabel)
            m_hoverLabel->SetLabel("");
        event.Skip();
    }

    void OnRightDown(wxMouseEvent& event)
    {
        int lx, ly;
        CalcUnscrolledPosition(event.GetX(), event.GetY(), &lx, &ly);
        long long clickTime = XToTime(lx);

        // Check if a marker is nearby (within 4 pixels tolerance)
        long long tolTime = (long long)(4.0 / m_pixelsPerUnit) + 1;
        bool nearMarker = false;
        long long nearTime = 0;
        for (const auto& kv : m_markers)
        {
            if (std::abs(kv.first - clickTime) <= tolTime)
            { nearMarker = true; nearTime = kv.first; break; }
        }

        wxMenu menu;
        if (nearMarker)
        {
            menu.Append(1, "Remove Marker: " + m_markers[nearTime]);
            menu.Append(2, "Rename Marker...");
        }
        else
        {
            menu.Append(3, "Add Marker Here...");
        }
        menu.AppendSeparator();
        menu.Append(4, "Set Reference Here");
        if (m_refTime >= 0)
            menu.Append(5, "Clear Reference");

        int choice = GetPopupMenuSelectionFromUser(menu, event.GetPosition());
        if (choice == 1)
        {
            m_markers.erase(nearTime);
            Refresh();
        }
        else if (choice == 2)
        {
            wxString newLabel = wxGetTextFromUser(
                "Marker label:", "Rename Marker", m_markers[nearTime], this);
            if (!newLabel.IsEmpty())
            { m_markers[nearTime] = newLabel; Refresh(); }
        }
        else if (choice == 3)
        {
            wxString label = wxGetTextFromUser(
                wxString::Format("Label for marker at time %lld:", clickTime),
                "Add Marker", "", this);
            if (!label.IsEmpty())
            { m_markers[clickTime] = label; Refresh(); }
        }
        else if (choice == 4)
        {
            m_refTime = clickTime;
            UpdateTimeLabelAndCallback();
            Refresh();
        }
        else if (choice == 5)
        {
            m_refTime = -1;
            UpdateTimeLabelAndCallback();
            Refresh();
        }
    }

    void OnWheel(wxMouseEvent& event)
    {
        if (event.ControlDown())
        {
            // Zoom anchored to the exact pixel under the mouse cursor
            double factor = event.GetWheelRotation() > 0 ? 1.25 : (1.0 / 1.25);
            ZoomAroundScreenX(factor, event.GetX());
        }
        else
        {
            event.Skip(); // pass to scrolled window for normal pan-scroll
        }
    }

    //--
    // Zoom helpers
    //--
    // Zoom by `factor` keeping the time at screen X coordinate `screenX` fixed.
    // After zooming, the same time point stays under `screenX` - no drift.
    // Zoom-out floor: the ppu that makes the full waveform exactly fill the
    // client width (i.e. the same as Fit).  Going further out makes no sense.
    void ZoomAroundScreenX(double factor, int screenX)
    {
        // Compute the fit-level ppu - this is the minimum useful zoom level.
        int clientW = GetClientSize().GetWidth();
        double fitPPU = (clientW > 4)
            ? (double)(clientW - 4) / (double)EffectiveEndTime()
            : 1e-9;
        if (fitPPU < 1e-9) fitPPU = 1e-9;

        double newPPU = m_pixelsPerUnit * factor;

        // If zooming out would go below the fit level, snap to Fit instead.
        if (newPPU <= fitPPU)
        {
            m_pixelsPerUnit = fitPPU;
            m_fitted = true;
            UpdateLayout();
            Scroll(0, wxDefaultCoord);   // full waveform, start at t=0
            return;
        }

        m_fitted = false;

        // Convert screen X → canvas (unscrolled) X → time
        int canvasX, dummy;
        CalcUnscrolledPosition(screenX, 0, &canvasX, &dummy);
        long long anchorTime = XToTime(canvasX);

        m_pixelsPerUnit = newPPU;
        UpdateLayout();

        // Scroll so that anchorTime lands back at screenX
        int newCanvasX = TimeToX(anchorTime);
        int newScrollX = newCanvasX - screenX;
        if (newScrollX < 0) newScrollX = 0;
        Scroll(newScrollX, wxDefaultCoord);
    }

    // Returns the screen X to use as anchor when zooming via toolbar buttons.
    // Prefers the cursor position (if visible); falls back to the centre of
    // the visible area.
    int VisibleAnchorScreenX() const
    {
        int clientW = GetClientSize().GetWidth();
        if (m_cursorTime >= 0)
        {
            int cursorCanvasX = TimeToX(m_cursorTime);
            int scrollX       = GetScrollPos(wxHORIZONTAL);
            int screenX       = cursorCanvasX - scrollX;
            if (screenX >= 0 && screenX < clientW)
                return screenX;
        }
        return clientW / 2;
    }
};
