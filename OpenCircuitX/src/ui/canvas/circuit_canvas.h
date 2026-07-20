#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/timer.h>
#include "core/circuit/gate.h"
#include "ui/canvas/gate_renderer.h"
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <memory>

enum class CanvasMode { Select, PlaceGate };

// Freeform text note placed on the canvas schematic.
struct CanvasAnnotation
{
    int      id;
    wxPoint  pos;    // top-left in logical (unzoomed) canvas coordinates
    wxString text;
};

// Snapshot for undo/redo
struct CanvasSnapshot
{
    std::vector<Gate>             gates;
    std::vector<Wire>             wires;
    std::vector<CanvasAnnotation> annotations;
    int nextGateId;
    int nextAnnotId;
};

class CircuitCanvas : public wxScrolledWindow
{
public:
    explicit CircuitCanvas(wxWindow* parent);

    void     SetPlacementMode(GateType type);
    void     SetSelectMode();
    void     ClearAll();
    void     RunSimulation();
    void     TickClocks();

    // Multi-gate selection
    void     SelectAll();
    void     DeleteSelected();
    void     CopySelected();
    void     PasteClipboard();
    void     DuplicateSelected();

    // Alignment - operate on m_selectedGates (no-op if fewer than 2 selected)
    void     AlignLeft();
    void     AlignRight();
    void     AlignTop();
    void     AlignBottom();
    void     AlignCenterH();   // distribute centres along a horizontal axis
    void     AlignCenterV();   // distribute centres along a vertical axis
    bool     SaveCanvas(const wxString& filePath) const;
    bool     LoadCanvas(const wxString& filePath);
    bool     ExportToPNG(const wxString& filePath) const;
    wxString ExportToVHDL() const;
    wxString ExportToVerilog() const;

    // Undo / Redo
    void UndoCanvas();
    void RedoCanvas();
    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }

    // Animation
    void StartAnimation();
    void StopAnimation();
    void StepAnimation();
    void SetAnimSpeed(int ms);
    bool IsAnimating() const { return m_animating; }

    // Zoom (1.0 = 100 %)
    void  ZoomIn();
    void  ZoomOut();
    void  ZoomReset();
    void  ZoomToFit();   // scales zoom so all gates fill the visible area
    float GetZoom() const { return m_zoom; }

    // Annotations - freeform text notes on the canvas.
    // These are placed via right-click → "Add Note Here...", edited by double-click,
    // deleted by right-click → "Delete Note", and dragged by left-click+drag.
    // They are saved/loaded with the .ocxschem file and included in undo/redo.

    // Gate shape scheme (ANSI by default).  IEC and BS3939 are added in v1.1/v1.2.
    void SetGateScheme(GateScheme scheme);

    // Hover status - fires with a descriptive string when the mouse enters/leaves a gate.
    void SetStatusCallback(std::function<void(const wxString&)> cb) { m_statusCb = cb; }

    // VCD recording - record OUTPUT/INPUT states during animation, export as .vcd.
    // cb fires with the generated file path when recording stops.
    void StartRecording(const wxString& outputPath);
    void StopRecording();
    bool IsRecording() const { return m_recording; }
    void SetSimVCDCallback(std::function<void(const wxString&)> cb) { m_simVCDCb = cb; }

private:
    std::vector<Gate> gates;
    std::vector<Wire> wires;

    CanvasMode mode;
    GateType   pendingType;
    int        nextGateId;

    // Wire drawing state
    bool    drawingWire;
    int     wireFromGateId;
    int     wireFromPinIndex;  // which output pin (0=primary, 1=secondary)
    wxPoint wireCursorPos;

    // Drag state
    int     dragGateId;
    wxPoint dragOffset;
    bool    dragging;

    // Ghost position for placement mode
    wxPoint ghostPos;

    // Zoom
    float   m_zoom = 1.0f;

    // Multi-gate selection
    std::set<int>          m_selectedGates;
    std::vector<Gate>      m_clipboard;
    std::map<int, wxPoint> m_selInitialPos;  // gate positions at start of multi-drag
    wxPoint                m_dragStartPt;    // mouse position at start of multi-drag

    // Rubber-band selection
    bool    m_rubberBand    = false;
    wxPoint m_rubberStart;    // logical coords
    wxPoint m_rubberCurrent;  // logical coords

    // Undo / Redo stacks
    std::vector<CanvasSnapshot> m_undoStack;
    std::vector<CanvasSnapshot> m_redoStack;

    void PushUndoState();

    // Helpers
    wxPoint     SnapToGrid(wxPoint p) const;
    wxPoint     CanvasToLogical(wxPoint screen) const; // accounts for zoom
    Gate*       FindGate(int id);
    const Gate* FindGate(int id) const;
    int         HitTestGate(wxPoint p) const;
    bool        HitTestOutputPin(wxPoint p, int& gateId, int& pinIdx) const;
    bool        HitTestInputPin(wxPoint p, int& gateId, int& pinIdx) const;
    int         HitTestWire(wxPoint p) const;  // returns index into wires, or -1

    // Gate renderer - owns the active scheme object
    std::unique_ptr<GateRenderer> m_renderer;
    GateScheme                    m_gateScheme = GateScheme::ANSI;

    void DrawGrid(wxDC& dc) const;
    void DrawGate(wxDC& dc, const Gate& gate, wxGraphicsContext* gc) const;
    void DrawWires(wxDC& dc) const;
    void DrawAnnotations(wxDC& dc) const;

    int  HitTestAnnotation(wxPoint logicalPt) const;  // returns index, -1 if none

    void SimulateOnce();
    void DeleteGate(int id);

    // Annotations
    std::vector<CanvasAnnotation> m_annotations;
    int m_nextAnnotId  = 1;
    int m_dragAnnotIdx = -1;   // index into m_annotations being dragged (-1 = none)
    wxPoint m_dragAnnotOffset; // logical offset from annotation pos to mouse at drag start

    // Hover status callback
    std::function<void(const wxString&)> m_statusCb;
    int m_hoveredGateId = -1;  // gate under mouse cursor (-1 = none)

    // Animation internals
    wxTimer              m_animTimer;
    bool                 m_animating      = false;
    int                  m_animIntervalMs = 500;
    std::map<int, bool>  m_prevOutputs;
    std::set<int>        m_changedGates;

    void CapturePrevOutputs();
    void FindChangedGates();
    void OnAnimTimer(wxTimerEvent& event);

    // VCD recording internals
    struct SimSample {
        long long tick;
        int       gateId;
        bool      value;
    };
    bool                                   m_recording    = false;
    long long                              m_recordTick   = 0;
    wxString                               m_recordPath;
    std::vector<SimSample>                 m_recordSamples;
    std::function<void(const wxString&)>   m_simVCDCb;

    bool WriteSimVCD() const;   // writes m_recordSamples → m_recordPath

    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnLeftDblClick(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
};
