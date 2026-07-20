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
#include <climits>
#include <map>
#include <wx/file.h>

// Mouse / keyboard
//--
void CircuitCanvas::OnLeftDown(wxMouseEvent& event)
{
    SetFocus();
    wxPoint raw;
    CalcUnscrolledPosition(event.GetX(), event.GetY(), &raw.x, &raw.y);
    wxPoint pt = CanvasToLogical(raw);

    //** Placement mode **//
    if (mode == CanvasMode::PlaceGate)
    {
        PushUndoState();
        Gate g;
        g.id             = nextGateId++;
        g.type           = pendingType;
        g.pos            = SnapToGrid(pt);
        g.label          = wxEmptyString;
        g.inputState[0]  = g.inputState[1] = g.inputState[2] = g.inputState[3] = false;
        g.outputState    = false;
        g.outputState2   = false;

        // Prompt for port name for INPUT/OUTPUT
        if (g.type == GateType::INPUT || g.type == GateType::OUTPUT)
        {
            wxString dflt = (g.type == GateType::INPUT) ? "in0" : "out0";
            wxString name = wxGetTextFromUser(
                "Enter port name:", "Port Name", dflt, this);
            if (!name.IsEmpty())
                g.label = name;
        }

        gates.push_back(g);
        // Shift+click: stay in placement mode for rapid multi-drop.
        // Normal click: return to select mode after one placement.
        if (!event.ShiftDown())
            SetSelectMode();
        SimulateOnce();
        Refresh();
        return;
    }

    //** Finish wire draw **//
    if (drawingWire)
    {
        int inGateId = -1, inPinIdx = -1;
        if (HitTestInputPin(pt, inGateId, inPinIdx) && inGateId != wireFromGateId)
        {
            PushUndoState();
            // Remove existing wire to this input
            wires.erase(
                std::remove_if(wires.begin(), wires.end(),
                    [inGateId, inPinIdx](const Wire& w){
                        return w.toGateId == inGateId && w.toPinIndex == inPinIdx; }),
                wires.end());
            Wire w;
            w.fromGateId   = wireFromGateId;
            w.fromPinIndex = wireFromPinIndex;
            w.toGateId     = inGateId;
            w.toPinIndex   = inPinIdx;
            wires.push_back(w);
            SimulateOnce();
        }
        drawingWire = false;
        Refresh();
        return;
    }

    //** Check output pin - start wire **//
    int outGateId = -1, outPinIdx = -1;
    if (HitTestOutputPin(pt, outGateId, outPinIdx))
    {
        drawingWire      = true;
        wireFromGateId   = outGateId;
        wireFromPinIndex = outPinIdx;
        wireCursorPos    = pt;
        Refresh();
        return;
    }

    //** Hit annotation - start drag **//
    int annotIdx = HitTestAnnotation(pt);
    if (annotIdx != -1)
    {
        m_dragAnnotIdx    = annotIdx;
        m_dragAnnotOffset = pt - m_annotations[annotIdx].pos;
        CaptureMouse();
        return;
    }

    //** Hit gate body **//
    int gateId = HitTestGate(pt);
    if (gateId != -1)
    {
        Gate* g = FindGate(gateId);
        if (!g) return;

        // Toggle undriven input pin
        for (int i = 0; i < g->GetInputCount(); ++i)
        {
            if (Dist2(pt, g->GetInputPinPos(i)) <= PIN_HIT * PIN_HIT)
            {
                bool driven = false;
                for (const Wire& w : wires)
                    if (w.toGateId == gateId && w.toPinIndex == i)
                    { driven = true; break; }
                if (!driven)
                {
                    PushUndoState();
                    g->inputState[i] = !g->inputState[i];
                    SimulateOnce();
                    Refresh();
                    return;
                }
            }
        }

        // Toggle INPUT gate output (acts as a manual signal source)
        if (g->type == GateType::INPUT)
        {
            PushUndoState();
            g->outputState = !g->outputState;
            SimulateOnce();
            Refresh();
            return;
        }

        // Ctrl+click: toggle this gate in selection, no drag
        if (event.ControlDown())
        {
            if (m_selectedGates.count(gateId))
                m_selectedGates.erase(gateId);
            else
                m_selectedGates.insert(gateId);
            Refresh();
            return;
        }

        // Start drag - single or multi
        if (m_selectedGates.size() > 1 && m_selectedGates.count(gateId))
        {
            // Multi-drag: record initial positions relative to drag start
            m_dragStartPt = pt;
            m_selInitialPos.clear();
            for (int selId : m_selectedGates)
            {
                const Gate* sg = FindGate(selId);
                if (sg) m_selInitialPos[selId] = sg->pos;
            }
        }
        else
        {
            // Single gate: reset selection to just this gate
            m_selectedGates.clear();
            m_selectedGates.insert(gateId);
            m_selInitialPos.clear();
        }

        dragGateId  = gateId;
        dragOffset  = pt - g->pos;
        dragging    = true;
        CaptureMouse();
        Refresh();
    }
    else
    {
        // Empty canvas: clear selection and start rubber band
        if (!event.ControlDown())
            m_selectedGates.clear();
        m_rubberBand    = true;
        m_rubberStart   = pt;
        m_rubberCurrent = pt;
        drawingWire     = false;
        Refresh();
    }
}

void CircuitCanvas::OnLeftUp(wxMouseEvent& /*event*/)
{
    if (m_dragAnnotIdx >= 0)
    {
        if (HasCapture()) ReleaseMouse();
        m_dragAnnotIdx = -1;
        Refresh();
        return;
    }
    if (m_rubberBand)
    {
        m_rubberBand = false;
        wxRect sel(
            std::min(m_rubberStart.x, m_rubberCurrent.x),
            std::min(m_rubberStart.y, m_rubberCurrent.y),
            std::abs(m_rubberCurrent.x - m_rubberStart.x),
            std::abs(m_rubberCurrent.y - m_rubberStart.y));
        if (sel.width > 4 || sel.height > 4)
        {
            for (const Gate& g : gates)
            {
                if (sel.Contains(g.GetBounds()))
                    m_selectedGates.insert(g.id);
            }
        }
        Refresh();
        return;
    }
    if (dragging)
    {
        dragging = false;
        if (HasCapture())
            ReleaseMouse();
        m_selInitialPos.clear();
        SimulateOnce();
        Refresh();
    }
}

void CircuitCanvas::OnLeftDblClick(wxMouseEvent& event)
{
    SetFocus();
    wxPoint raw;
    CalcUnscrolledPosition(event.GetX(), event.GetY(), &raw.x, &raw.y);
    wxPoint pt = CanvasToLogical(raw);

    // Edit annotation on double-click
    int annotIdx = HitTestAnnotation(pt);
    if (annotIdx >= 0)
    {
        wxTextEntryDialog dlg(this, "Note text:", "Edit Note",
                              m_annotations[annotIdx].text);
        if (dlg.ShowModal() == wxID_OK)
        {
            PushUndoState();
            m_annotations[annotIdx].text = dlg.GetValue().Trim(true).Trim(false);
            Refresh();
        }
        return;
    }

    int gateId = HitTestGate(pt);
    if (gateId == -1) return;

    Gate* g = FindGate(gateId);
    if (!g) return;

    // Build dialog prompt based on gate type
    wxString prompt, title;
    if (g->type == GateType::INPUT || g->type == GateType::OUTPUT)
    {
        prompt = "Port name:";
        title  = "Rename Port";
    }
    else
    {
        prompt = "Gate label (leave empty for default):";
        title  = "Rename Gate";
    }

    wxString current = g->label;
    wxTextEntryDialog dlg(this, prompt, title, current);
    if (dlg.ShowModal() != wxID_OK) return;

    PushUndoState();
    g->label = dlg.GetValue().Trim(true).Trim(false);
    Refresh();
}

void CircuitCanvas::OnMouseMove(wxMouseEvent& event)
{
    wxPoint raw;
    CalcUnscrolledPosition(event.GetX(), event.GetY(), &raw.x, &raw.y);
    wxPoint pt = CanvasToLogical(raw);

    // Gate hover status
    if (m_statusCb)
    {
        int hov = HitTestGate(pt);
        if (hov != m_hoveredGateId)
        {
            m_hoveredGateId = hov;
            if (hov == -1)
            {
                m_statusCb(wxEmptyString);
            }
            else
            {
                const Gate* g = FindGate(hov);
                if (g)
                {
                    wxString info = wxString::Format(
                        "%s  |  %d in  |  %d out  |  pos (%d, %d)",
                        g->GetLabel(),
                        g->GetInputCount(), g->GetOutputCount(),
                        g->pos.x, g->pos.y);
                    m_statusCb(info);
                }
            }
        }
    }

    if (mode == CanvasMode::PlaceGate)
    {
        ghostPos = SnapToGrid(pt);
        Refresh();
        return;
    }

    // Annotation drag
    if (m_dragAnnotIdx >= 0 && m_dragAnnotIdx < (int)m_annotations.size())
    {
        m_annotations[m_dragAnnotIdx].pos = SnapToGrid(pt - m_dragAnnotOffset);
        Refresh();
        return;
    }

    if (dragging && dragGateId != -1)
    {
        if (m_selInitialPos.size() > 1)
        {
            // Multi-gate drag: move all selected gates by delta from drag start
            wxPoint delta = pt - m_dragStartPt;
            for (const auto& kv : m_selInitialPos)
            {
                Gate* sg = FindGate(kv.first);
                if (sg) sg->pos = SnapToGrid(kv.second + delta);
            }
        }
        else
        {
            Gate* g = FindGate(dragGateId);
            if (g) g->pos = SnapToGrid(pt - dragOffset);
        }
        Refresh();
        return;
    }

    if (m_rubberBand)
    {
        m_rubberCurrent = pt;
        Refresh();
        return;
    }

    if (drawingWire)
    {
        wireCursorPos = pt;
        Refresh();
    }
}

void CircuitCanvas::OnRightDown(wxMouseEvent& event)
{
    SetFocus();

    // Cancel placement or wire drawing on right-click
    if (mode == CanvasMode::PlaceGate || drawingWire)
    {
        SetSelectMode();
        return;
    }

    wxPoint raw;
    CalcUnscrolledPosition(event.GetX(), event.GetY(), &raw.x, &raw.y);
    wxPoint pt = CanvasToLogical(raw);

    //** Annotation context menu **//
    int annotIdx = HitTestAnnotation(pt);
    if (annotIdx >= 0)
    {
        wxMenu menu;
        menu.Append(1, "Edit Note...");
        menu.Append(2, "Delete Note");
        int choice = GetPopupMenuSelectionFromUser(menu, event.GetPosition());
        if (choice == 1)
        {
            wxTextEntryDialog dlg(this, "Note text:", "Edit Note",
                                  m_annotations[annotIdx].text);
            if (dlg.ShowModal() == wxID_OK)
            {
                PushUndoState();
                m_annotations[annotIdx].text = dlg.GetValue().Trim(true).Trim(false);
                Refresh();
            }
        }
        else if (choice == 2)
        {
            PushUndoState();
            m_annotations.erase(m_annotations.begin() + annotIdx);
            Refresh();
        }
        return;
    }

    //** Wire context menu **//
    int wireIdx = HitTestWire(pt);
    if (wireIdx != -1)
    {
        wxMenu menu;
        menu.Append(1, "Delete Wire");
        if (GetPopupMenuSelectionFromUser(menu, event.GetPosition()) == 1)
        {
            PushUndoState();
            wires.erase(wires.begin() + wireIdx);
            SimulateOnce();
            Refresh();
        }
        return;
    }

    //** Gate context menu **//
    int gateId = HitTestGate(pt);
    if (gateId != -1)
    {
        wxMenu menu;
        menu.Append(1, "Rename / Label");
        menu.Append(2, "Duplicate");
        menu.AppendSeparator();
        menu.Append(3, "Delete Gate");

        int choice = GetPopupMenuSelectionFromUser(menu, event.GetPosition());
        Gate* g = FindGate(gateId);
        if (!g) return;

        if (choice == 1)
        {
            wxString prompt = (g->type == GateType::INPUT || g->type == GateType::OUTPUT)
                              ? "Port name:" : "Gate label (leave empty for default):";
            wxTextEntryDialog dlg(this, prompt, "Rename", g->label);
            if (dlg.ShowModal() == wxID_OK)
            {
                PushUndoState();
                g->label = wxString(dlg.GetValue()).Trim(true).Trim(false);
                Refresh();
            }
        }
        else if (choice == 2)
        {
            PushUndoState();
            Gate copy      = *g;
            copy.id        = nextGateId++;
            copy.pos       = SnapToGrid(wxPoint(g->pos.x + GRID * 4, g->pos.y + GRID * 4));
            copy.outputState  = false;
            copy.outputState2 = false;
            for (auto& s : copy.inputState) s = false;
            gates.push_back(copy);
            SimulateOnce();
            Refresh();
        }
        else if (choice == 3)
        {
            PushUndoState();
            DeleteGate(gateId);
            if (dragGateId == gateId) dragGateId = -1;
            SimulateOnce();
            Refresh();
        }
        return;
    }

    //** Empty-space context menu **//
    wxMenu menu;
    if (!m_clipboard.empty())
        menu.Append(1, "Paste\tCtrl+V");
    menu.Append(2, "Select All\tCtrl+A");
    menu.AppendSeparator();
    menu.Append(4, "Add Note Here...");
    if (!gates.empty())
    {
        menu.AppendSeparator();
        menu.Append(3, "Zoom to Fit\tCtrl+Shift+F");
    }
    int choice = GetPopupMenuSelectionFromUser(menu, event.GetPosition());
    if      (choice == 1) PasteClipboard();
    else if (choice == 2) SelectAll();
    else if (choice == 3) ZoomToFit();
    else if (choice == 4)
    {
        wxTextEntryDialog dlg(this, "Note text:", "Add Note", "");
        if (dlg.ShowModal() == wxID_OK)
        {
            wxString text = dlg.GetValue().Trim(true).Trim(false);
            if (!text.IsEmpty())
            {
                PushUndoState();
                CanvasAnnotation a;
                a.id   = m_nextAnnotId++;
                a.pos  = SnapToGrid(pt);
                a.text = text;
                m_annotations.push_back(a);
                Refresh();
            }
        }
    }
}

void CircuitCanvas::OnMouseWheel(wxMouseEvent& event)
{
    if (event.ControlDown())
    {
        if (event.GetWheelRotation() > 0)
            ZoomIn();
        else
            ZoomOut();
    }
    else
    {
        event.Skip();
    }
}

void CircuitCanvas::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE)
    {
        SetSelectMode();
        return;
    }
    if (event.GetKeyCode() == WXK_DELETE)
    {
        if (!m_selectedGates.empty())
        {
            DeleteSelected();
        }
        else if (dragGateId != -1)
        {
            PushUndoState();
            DeleteGate(dragGateId);
            dragGateId = -1;
            dragging   = false;
            if (HasCapture()) ReleaseMouse();
            SimulateOnce();
            Refresh();
        }
        return;
    }
    if (event.ControlDown())
    {
        if (event.GetKeyCode() == 'A') { SelectAll();         return; }
        if (event.GetKeyCode() == 'C') { CopySelected();      return; }
        if (event.GetKeyCode() == 'V') { PasteClipboard();    return; }
        if (event.GetKeyCode() == 'D') { DuplicateSelected(); return; }
        if (event.GetKeyCode() == 'Z') { UndoCanvas();        return; }
        if (event.GetKeyCode() == 'Y') { RedoCanvas();        return; }
        if (event.GetKeyCode() == WXK_ADD ||
            event.GetKeyCode() == '=')      { ZoomIn();     return; }
        if (event.GetKeyCode() == WXK_SUBTRACT ||
            event.GetKeyCode() == '-')      { ZoomOut();    return; }
        if (event.GetKeyCode() == '0')      { ZoomReset();  return; }
        if (event.ShiftDown() &&
            event.GetKeyCode() == 'F')      { ZoomToFit();  return; }
    }

    // Gate placement hotkeys (no modifier required).
    // Press the key to enter placement mode for that gate type.
    // Hold Shift while placing to stay in placement mode for multiple drops.
    if (!event.ControlDown() && !event.AltDown() &&
        mode != CanvasMode::PlaceGate)
    {
        switch (event.GetKeyCode())
        {
            case 'A': SetPlacementMode(GateType::AND);        return;
            case 'O': SetPlacementMode(GateType::OR);         return;
            case 'N': SetPlacementMode(GateType::NOT);        return;
            case 'X': SetPlacementMode(GateType::XOR);        return;
            case 'G': SetPlacementMode(GateType::NAND);       return;
            case 'R': SetPlacementMode(GateType::NOR);        return;
            case 'P': SetPlacementMode(GateType::XNOR);       return;
            case 'I': SetPlacementMode(GateType::INPUT);      return;
            case 'U': SetPlacementMode(GateType::OUTPUT);     return;
            case 'C': SetPlacementMode(GateType::CLOCK);      return;
            case 'D': SetPlacementMode(GateType::DFLIPFLOP);  return;
            case 'M': SetPlacementMode(GateType::MUX);        return;
            case 'H': SetPlacementMode(GateType::HALFADDER);  return;
            case 'F': SetPlacementMode(GateType::FULLADDER);  return;
            default: break;
        }
    }

    event.Skip();
}

//--