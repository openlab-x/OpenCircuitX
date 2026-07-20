#include "circuit_canvas.h"
#include "ui/canvas/circuit_canvas_private.h"
#include "ui/shell/app_theme.h"
#include <algorithm>
#include <cmath>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/textdlg.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>
#include <climits>
#include <map>
#include <wx/file.h>
#include <memory>

// Multi-gate selection
//--
void CircuitCanvas::SelectAll()
{
    m_selectedGates.clear();
    for (const Gate& g : gates)
        m_selectedGates.insert(g.id);
    Refresh();
}

void CircuitCanvas::DeleteSelected()
{
    if (m_selectedGates.empty()) return;
    PushUndoState();
    for (int id : m_selectedGates)
        DeleteGate(id);
    m_selectedGates.clear();
    dragGateId = -1;
    dragging   = false;
    if (HasCapture()) ReleaseMouse();
    SimulateOnce();
    Refresh();
}

void CircuitCanvas::CopySelected()
{
    m_clipboard.clear();
    for (const Gate& g : gates)
        if (m_selectedGates.count(g.id))
            m_clipboard.push_back(g);
}

void CircuitCanvas::PasteClipboard()
{
    if (m_clipboard.empty()) return;
    PushUndoState();

    std::map<int, int> idMap;
    m_selectedGates.clear();
    for (const Gate& g : m_clipboard)
    {
        Gate ng = g;
        ng.id  = nextGateId++;
        ng.pos = wxPoint(g.pos.x + 40, g.pos.y + 40);
        idMap[g.id] = ng.id;
        gates.push_back(ng);
        m_selectedGates.insert(ng.id);
    }

    // Re-create wires between pasted gates (internal connections)
    for (const Wire& w : wires)
    {
        if (idMap.count(w.fromGateId) && idMap.count(w.toGateId))
        {
            Wire nw       = w;
            nw.fromGateId = idMap[w.fromGateId];
            nw.toGateId   = idMap[w.toGateId];
            wires.push_back(nw);
        }
    }
    SimulateOnce();
    Refresh();
}

void CircuitCanvas::DuplicateSelected()
{
    CopySelected();
    PasteClipboard();
}

//--
// Gate alignment (operates on m_selectedGates)
//--
void CircuitCanvas::AlignLeft()
{
    if (m_selectedGates.size() < 2) return;
    PushUndoState();
    int minX = INT_MAX;
    for (int id : m_selectedGates) { const Gate* g = FindGate(id); if (g) minX = std::min(minX, g->pos.x); }
    for (int id : m_selectedGates) { Gate* g = FindGate(id); if (g) g->pos.x = minX; }
    Refresh();
}

void CircuitCanvas::AlignRight()
{
    if (m_selectedGates.size() < 2) return;
    PushUndoState();
    int maxR = INT_MIN;
    for (int id : m_selectedGates) { const Gate* g = FindGate(id); if (g) maxR = std::max(maxR, g->pos.x + GATE_W); }
    for (int id : m_selectedGates) { Gate* g = FindGate(id); if (g) g->pos.x = maxR - GATE_W; }
    Refresh();
}

void CircuitCanvas::AlignTop()
{
    if (m_selectedGates.size() < 2) return;
    PushUndoState();
    int minY = INT_MAX;
    for (int id : m_selectedGates) { const Gate* g = FindGate(id); if (g) minY = std::min(minY, g->pos.y); }
    for (int id : m_selectedGates) { Gate* g = FindGate(id); if (g) g->pos.y = minY; }
    Refresh();
}

void CircuitCanvas::AlignBottom()
{
    if (m_selectedGates.size() < 2) return;
    PushUndoState();
    int maxB = INT_MIN;
    for (int id : m_selectedGates) { const Gate* g = FindGate(id); if (g) maxB = std::max(maxB, g->pos.y + g->GetHeight()); }
    for (int id : m_selectedGates) { Gate* g = FindGate(id); if (g) g->pos.y = maxB - g->GetHeight(); }
    Refresh();
}

void CircuitCanvas::AlignCenterH()
{
    if (m_selectedGates.size() < 2) return;
    PushUndoState();
    int minY = INT_MAX, maxB = INT_MIN;
    for (int id : m_selectedGates)
    {
        const Gate* g = FindGate(id);
        if (!g) continue;
        minY = std::min(minY, g->pos.y);
        maxB = std::max(maxB, g->pos.y + g->GetHeight());
    }
    int centerY = (minY + maxB) / 2;
    for (int id : m_selectedGates)
    {
        Gate* g = FindGate(id);
        if (g) g->pos.y = centerY - g->GetHeight() / 2;
    }
    Refresh();
}

void CircuitCanvas::AlignCenterV()
{
    if (m_selectedGates.size() < 2) return;
    PushUndoState();
    int minX = INT_MAX, maxR = INT_MIN;
    for (int id : m_selectedGates)
    {
        const Gate* g = FindGate(id);
        if (!g) continue;
        minX = std::min(minX, g->pos.x);
        maxR = std::max(maxR, g->pos.x + GATE_W);
    }
    int centerX = (minX + maxR) / 2;
    for (int id : m_selectedGates)
    {
        Gate* g = FindGate(id);
        if (g) g->pos.x = centerX - GATE_W / 2;
    }
    Refresh();
}

//--
// PNG export
//--
bool CircuitCanvas::ExportToPNG(const wxString& filePath) const
{
    if (gates.empty()) return false;

    // Compute bounding box of all gates (include pin stub overhang)
    int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
    for (const Gate& g : gates)
    {
        wxRect b = g.GetBounds();
        minX = std::min(minX, b.x - PIN_STUB - 4);
        minY = std::min(minY, b.y - 4);
        maxX = std::max(maxX, b.x + b.width + PIN_STUB + 4);
        maxY = std::max(maxY, b.y + b.height + 4);
    }

    constexpr int PAD = 40;
    int bmpW = (maxX - minX) + PAD * 2;
    int bmpH = (maxY - minY) + PAD * 2;
    if (bmpW < 1 || bmpH < 1) return false;

    wxBitmap bmp(bmpW, bmpH, 24);
    wxMemoryDC dc(bmp);

    // Background
    dc.SetBackground(wxBrush(OCXTheme::BgEditor()));
    dc.Clear();

    // Light grid
    dc.SetPen(wxPen(wxColour(45, 45, 45), 1));
    for (int x = 0; x < bmpW; x += GRID) dc.DrawLine(x, 0, x, bmpH);
    for (int y = 0; y < bmpH; y += GRID) dc.DrawLine(0, y, bmpW, y);

    // Translate origin so gates start at (PAD - minX, PAD - minY)
    dc.SetDeviceOrigin(PAD - minX, PAD - minY);

    // Create GC for ANSI gate shapes; apply the same device-origin translation.
    std::unique_ptr<wxGraphicsContext> gcOwner(wxGraphicsContext::Create(dc));
    if (gcOwner)
        gcOwner->Translate((double)(PAD - minX), (double)(PAD - minY));
    wxGraphicsContext* gc = gcOwner.get();

    // Temporarily set zoom to 1 for export
    float savedZoom = m_zoom;
    const_cast<CircuitCanvas*>(this)->m_zoom = 1.0f;

    DrawWires(dc);
    for (const Gate& g : gates)
        DrawGate(dc, g, gc);
    DrawAnnotations(dc);

    const_cast<CircuitCanvas*>(this)->m_zoom = savedZoom;

    wxImage img = bmp.ConvertToImage();
    return img.SaveFile(filePath, wxBITMAP_TYPE_PNG);
}

//--
//--
// Annotations
//--
// Compute the bounding rect of an annotation in logical coords.
// Approximates text width (7px per char at size 9) with a minimum width of 80px.
static wxRect AnnotBounds(const CanvasAnnotation& a)
{
    int w = wxMax(80, (int)a.text.Length() * 7 + 16);
    return wxRect(a.pos.x, a.pos.y, w, 26);
}

void CircuitCanvas::DrawAnnotations(wxDC& dc) const
{
    if (m_annotations.empty()) return;

    wxFont f(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
             wxFONTWEIGHT_NORMAL, false, "Consolas");
    dc.SetFont(f);

    for (const CanvasAnnotation& a : m_annotations)
    {
        wxRect lr = AnnotBounds(a);  // logical rect

        // Scale to screen
        wxRect sr(int(lr.x * m_zoom), int(lr.y * m_zoom),
                  int(lr.width * m_zoom), int(lr.height * m_zoom));

        // Background + border
        dc.SetBrush(wxBrush(wxColour(35, 40, 30)));
        dc.SetPen(wxPen(wxColour(160, 140, 60), 1));
        dc.DrawRoundedRectangle(sr, 3);

        // Text
        dc.SetTextForeground(wxColour(210, 190, 90));
        wxSize tsz = dc.GetTextExtent(a.text);
        int tx = sr.x + 6;
        int ty = sr.y + (sr.height - tsz.GetHeight()) / 2;
        dc.DrawText(a.text, tx, ty);
    }
}

int CircuitCanvas::HitTestAnnotation(wxPoint pt) const
{
    for (int i = 0; i < (int)m_annotations.size(); ++i)
        if (AnnotBounds(m_annotations[i]).Contains(pt))
            return i;
    return -1;
}

