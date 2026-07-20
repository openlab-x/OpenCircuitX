#pragma once
#include <wx/wx.h>
#include <functional>

class CircuitCanvas;
class LogicEditorPanel;

// Left palette - component placement only (gates, combinational, sequential, ports).
// Action controls (run, animate, zoom, align, export…) live in CanvasActionPanel.
class ComponentPalette : public wxPanel
{
public:
    ComponentPalette(wxWindow* parent, CircuitCanvas* canvas,
                     LogicEditorPanel* editor         = nullptr,
                     std::function<void()> switchToEditor = nullptr);

private:
    CircuitCanvas* canvas;

    void OnPlaceGate(wxCommandEvent& event);
};
