#include "circuit_canvas.h"
#include "ui/canvas/gate_renderer_ansi.h"
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
#include <climits>
#include <map>
#include <wx/file.h>
#include "ui/canvas/circuit_canvas_private.h"


//--
CircuitCanvas::CircuitCanvas(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxHSCROLL | wxVSCROLL | wxBORDER_NONE | wxWANTS_CHARS)
    , mode(CanvasMode::Select)
    , pendingType(GateType::AND)
    , nextGateId(1)
    , drawingWire(false)
    , wireFromGateId(-1)
    , wireFromPinIndex(0)
    , dragGateId(-1)
    , dragging(false)
{
    SetBackgroundColour(OCXTheme::BgEditor());
    SetVirtualSize(2000, 2000);
    SetScrollRate(20, 20);

    m_renderer = std::make_unique<ANSIGateRenderer>();

    m_animTimer.SetOwner(this);
    Bind(wxEVT_TIMER,       &CircuitCanvas::OnAnimTimer,   this);
    Bind(wxEVT_PAINT,       &CircuitCanvas::OnPaint,       this);
    Bind(wxEVT_LEFT_DOWN,   &CircuitCanvas::OnLeftDown,    this);
    Bind(wxEVT_LEFT_UP,     &CircuitCanvas::OnLeftUp,      this);
    Bind(wxEVT_LEFT_DCLICK, &CircuitCanvas::OnLeftDblClick,this);
    Bind(wxEVT_MOTION,      &CircuitCanvas::OnMouseMove,   this);
    Bind(wxEVT_RIGHT_DOWN,  &CircuitCanvas::OnRightDown,   this);
    Bind(wxEVT_MOUSEWHEEL,  &CircuitCanvas::OnMouseWheel,  this);
    Bind(wxEVT_KEY_DOWN,    &CircuitCanvas::OnKeyDown,     this);
}

//--
// Gate scheme
//--
void CircuitCanvas::SetGateScheme(GateScheme scheme)
{
    m_gateScheme = scheme;
    switch (scheme)
    {
    case GateScheme::ANSI:
    default:
        m_renderer = std::make_unique<ANSIGateRenderer>();
        break;
    // IEC and BS3939 renderers added in v1.1 / v1.2
    }
    Refresh();
}

//--
// Logical coordinate helper
//--
wxPoint CircuitCanvas::CanvasToLogical(wxPoint screen) const
{
    // screen already unscrolled by CalcUnscrolledPosition before calling here
    return wxPoint(int(screen.x / m_zoom), int(screen.y / m_zoom));
}

//--
// Undo / Redo
//--
void CircuitCanvas::PushUndoState()
{
    CanvasSnapshot snap;
    snap.gates       = gates;
    snap.wires       = wires;
    snap.annotations = m_annotations;
    snap.nextGateId  = nextGateId;
    snap.nextAnnotId = m_nextAnnotId;
    m_undoStack.push_back(snap);
    if ((int)m_undoStack.size() > UNDO_LIMIT)
        m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();
}

void CircuitCanvas::UndoCanvas()
{
    if (m_undoStack.empty()) return;
    CanvasSnapshot cur;
    cur.gates       = gates;
    cur.wires       = wires;
    cur.annotations = m_annotations;
    cur.nextGateId  = nextGateId;
    cur.nextAnnotId = m_nextAnnotId;
    m_redoStack.push_back(cur);

    CanvasSnapshot& snap = m_undoStack.back();
    gates         = snap.gates;
    wires         = snap.wires;
    m_annotations = snap.annotations;
    nextGateId    = snap.nextGateId;
    m_nextAnnotId = snap.nextAnnotId;
    m_undoStack.pop_back();
    m_selectedGates.clear();
    Refresh();
}

void CircuitCanvas::RedoCanvas()
{
    if (m_redoStack.empty()) return;
    CanvasSnapshot cur;
    cur.gates       = gates;
    cur.wires       = wires;
    cur.annotations = m_annotations;
    cur.nextGateId  = nextGateId;
    cur.nextAnnotId = m_nextAnnotId;
    m_undoStack.push_back(cur);

    CanvasSnapshot& snap = m_redoStack.back();
    gates         = snap.gates;
    wires         = snap.wires;
    m_annotations = snap.annotations;
    nextGateId    = snap.nextGateId;
    m_nextAnnotId = snap.nextAnnotId;
    m_redoStack.pop_back();
    m_selectedGates.clear();
    Refresh();
}

//--
// Mode control
//--
void CircuitCanvas::SetPlacementMode(GateType type)
{
    mode        = CanvasMode::PlaceGate;
    pendingType = type;
    drawingWire = false;
    dragging    = false;
    dragGateId  = -1;
    SetCursor(wxCursor(wxCURSOR_CROSS));
    Refresh();
}

void CircuitCanvas::SetSelectMode()
{
    mode        = CanvasMode::Select;
    drawingWire = false;
    dragging    = false;
    dragGateId  = -1;
    SetCursor(wxNullCursor);
    Refresh();
}

void CircuitCanvas::ClearAll()
{
    StopAnimation();
    gates.clear();
    wires.clear();
    m_annotations.clear();
    nextGateId    = 1;
    m_nextAnnotId = 1;
    drawingWire   = false;
    dragging      = false;
    dragGateId    = -1;
    m_dragAnnotIdx = -1;
    mode        = CanvasMode::Select;
    m_changedGates.clear();
    m_prevOutputs.clear();
    m_undoStack.clear();
    m_redoStack.clear();
    m_selectedGates.clear();
    m_clipboard.clear();
    m_rubberBand = false;
    SetCursor(wxNullCursor);
    Refresh();
}

//--
// Zoom
//--
void CircuitCanvas::ZoomIn()
{
    m_zoom = std::min(ZOOM_MAX, m_zoom + ZOOM_STEP);
    Refresh();
}

void CircuitCanvas::ZoomOut()
{
    m_zoom = std::max(ZOOM_MIN, m_zoom - ZOOM_STEP);
    Refresh();
}

void CircuitCanvas::ZoomReset()
{
    m_zoom = 1.0f;
    Refresh();
}

void CircuitCanvas::ZoomToFit()
{
    if (gates.empty()) { ZoomReset(); return; }

    int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
    for (const Gate& g : gates)
    {
        wxRect b = g.GetBounds();
        minX = std::min(minX, b.GetLeft());
        minY = std::min(minY, b.GetTop());
        maxX = std::max(maxX, b.GetRight());
        maxY = std::max(maxY, b.GetBottom());
    }

    const int PAD = 40;
    int boxW = maxX - minX + PAD * 2;
    int boxH = maxY - minY + PAD * 2;

    wxSize client = GetClientSize();
    if (client.GetWidth() < 1 || client.GetHeight() < 1) return;

    float zx = (float)client.GetWidth()  / boxW;
    float zy = (float)client.GetHeight() / boxH;
    m_zoom = std::min(zx, zy);
    m_zoom = std::max(ZOOM_MIN, std::min(ZOOM_MAX, m_zoom));

    // Scroll so the bounding box top-left is visible with padding
    int scrollX = std::max(0, int((minX - PAD) * m_zoom / 20) - 1);
    int scrollY = std::max(0, int((minY - PAD) * m_zoom / 20) - 1);
    Scroll(scrollX, scrollY);
    Refresh();
}

//--
// Hit testing helpers
//--
wxPoint CircuitCanvas::SnapToGrid(wxPoint p) const
{
    return wxPoint((p.x / GRID) * GRID, (p.y / GRID) * GRID);
}

Gate* CircuitCanvas::FindGate(int id)
{
    for (Gate& g : gates)
        if (g.id == id) return &g;
    return nullptr;
}

const Gate* CircuitCanvas::FindGate(int id) const
{
    for (const Gate& g : gates)
        if (g.id == id) return &g;
    return nullptr;
}

int CircuitCanvas::HitTestGate(wxPoint p) const
{
    for (int i = (int)gates.size() - 1; i >= 0; --i)
        if (gates[i].GetBounds().Contains(p))
            return gates[i].id;
    return -1;
}

bool CircuitCanvas::HitTestOutputPin(wxPoint p, int& gateId, int& pinIdx) const
{
    for (const Gate& g : gates)
    {
        for (int i = 0; i < g.GetOutputCount(); ++i)
        {
            if (Dist2(p, g.GetOutputPinPos(i)) <= PIN_HIT * PIN_HIT)
            {
                gateId = g.id;
                pinIdx = i;
                return true;
            }
        }
    }
    return false;
}

bool CircuitCanvas::HitTestInputPin(wxPoint p, int& gateId, int& pinIdx) const
{
    for (const Gate& g : gates)
    {
        for (int i = 0; i < g.GetInputCount(); ++i)
        {
            if (Dist2(p, g.GetInputPinPos(i)) <= PIN_HIT * PIN_HIT)
            {
                gateId = g.id;
                pinIdx = i;
                return true;
            }
        }
    }
    return false;
}

int CircuitCanvas::HitTestWire(wxPoint p) const
{
    const int WIRE_HIT_D2 = 64; // 8px tolerance
    for (int wi = 0; wi < (int)wires.size(); ++wi)
    {
        const Wire&  w    = wires[wi];
        const Gate*  from = FindGate(w.fromGateId);
        const Gate*  to   = FindGate(w.toGateId);
        if (!from || !to) continue;

        wxPoint p1 = from->GetOutputPinPos(w.fromPinIndex);
        p1.x += GRID;
        wxPoint p2 = to->GetInputPinPos(w.toPinIndex);
        p2.x -= GRID;

        int midX = (p1.x + p2.x) / 2;
        wxPoint m1(midX, p1.y), m2(midX, p2.y);

        if (SegDist2(p, p1, m1) <= WIRE_HIT_D2) return wi;
        if (SegDist2(p, m1, m2) <= WIRE_HIT_D2) return wi;
        if (SegDist2(p, m2, p2) <= WIRE_HIT_D2) return wi;
    }
    return -1;
}

void CircuitCanvas::DeleteGate(int id)
{
    wires.erase(
        std::remove_if(wires.begin(), wires.end(),
            [id](const Wire& w){ return w.fromGateId == id || w.toGateId == id; }),
        wires.end());
    gates.erase(
        std::remove_if(gates.begin(), gates.end(),
            [id](const Gate& g){ return g.id == id; }),
        gates.end());
    Refresh();
}

//--
// Simulation
//--
void CircuitCanvas::RunSimulation()
{
    SimulateOnce();
    Refresh();
}

void CircuitCanvas::SimulateOnce()
{
    // Several forward passes to propagate through chains
    for (int pass = 0; pass < 5; ++pass)
    {
        for (Gate& g : gates)
        {
            g.outputState  = g.Evaluate();
            g.outputState2 = g.EvaluateSecondary();
        }

        for (const Wire& w : wires)
        {
            const Gate* from = FindGate(w.fromGateId);
            Gate*       to   = FindGate(w.toGateId);
            if (!from || !to) continue;
            bool sig = (w.fromPinIndex == 0) ? from->outputState : from->outputState2;
            to->inputState[w.toPinIndex] = sig;
        }
    }
}

//--
// Drawing helpers
//--