#pragma once
#include <wx/wx.h>
#include <functional>

class CircuitCanvas;
class LogicEditorPanel;

// Right-side action panel in the Circuit Canvas tab.
// Contains: Simulation, Animation, Edit (Undo/Redo), Zoom, Align, Canvas ops, Export.
// Component placement buttons live in ComponentPalette (left panel).
class CanvasActionPanel : public wxPanel
{
public:
    CanvasActionPanel(wxWindow* parent,
                      CircuitCanvas*        canvas,
                      LogicEditorPanel*     editor         = nullptr,
                      std::function<void()> switchToEditor = nullptr);

    // Call after any animation state change to refresh Play/Pause button colors.
    void UpdateAnimButtons();

private:
    CircuitCanvas*        m_canvas;
    LogicEditorPanel*     m_editor;
    std::function<void()> m_switchToEditor;

    wxButton* m_btnPlay     = nullptr;
    wxButton* m_btnPause    = nullptr;
    wxButton* m_btnRecord   = nullptr;
    wxSlider* m_speedSlider = nullptr;

    void OnRunSim(wxCommandEvent&);
    void OnTickClock(wxCommandEvent&);
    void OnSaveCanvas(wxCommandEvent&);
    void OnLoadCanvas(wxCommandEvent&);
    void OnClear(wxCommandEvent&);
    void OnExportVHDL(wxCommandEvent&);
    void OnExportVerilog(wxCommandEvent&);
    void OnExportPNG(wxCommandEvent&);
    void OnUndo(wxCommandEvent&);
    void OnRedo(wxCommandEvent&);
    void OnAnimPlay(wxCommandEvent&);
    void OnAnimPause(wxCommandEvent&);
    void OnAnimStep(wxCommandEvent&);
    void OnAnimSpeed(wxCommandEvent&);
    void OnRecord(wxCommandEvent&);
    void OnAlign(wxCommandEvent&);
    void OnCanvasZoom(wxCommandEvent&);
};
