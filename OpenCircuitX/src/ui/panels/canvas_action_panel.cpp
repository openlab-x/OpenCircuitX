#include "canvas_action_panel.h"
#include "ui/canvas/circuit_canvas.h"
#include "ui/editor/logic_editor_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/filedlg.h>
#include <wx/scrolwin.h>

//--
// IDs  (local to this translation unit)
//--
enum
{
    // Simulation
    ID_AP_Run      = wxID_HIGHEST + 500,
    ID_AP_Tick,
    // Edit
    ID_AP_Undo,
    ID_AP_Redo,
    // Canvas ops
    ID_AP_Save,
    ID_AP_Load,
    ID_AP_Clear,
    // Export
    ID_AP_ExportVHDL,
    ID_AP_ExportVerilog,
    ID_AP_ExportPNG,
    // Animation
    ID_AP_AnimPlay,
    ID_AP_AnimPause,
    ID_AP_AnimStep,
    ID_AP_AnimSpeed,
    ID_AP_Record,
    // Alignment
    ID_AP_AlignLeft,
    ID_AP_AlignRight,
    ID_AP_AlignTop,
    ID_AP_AlignBottom,
    ID_AP_AlignCenterH,
    ID_AP_AlignCenterV,
    // Zoom
    ID_AP_ZoomIn,
    ID_AP_ZoomOut,
    ID_AP_ZoomFit,
    ID_AP_ZoomReset
};

//--
// Constructor
//--
CanvasActionPanel::CanvasActionPanel(wxWindow* parent,
                                     CircuitCanvas*        canvas,
                                     LogicEditorPanel*     editor,
                                     std::function<void()> switchToEditor)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(160, -1))
    , m_canvas(canvas)
    , m_editor(editor)
    , m_switchToEditor(switchToEditor)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    wxScrolledWindow* scroll = new wxScrolledWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxVSCROLL | wxBORDER_NONE);
    scroll->SetBackgroundColour(OCXTheme::BgPanel());
    scroll->SetScrollRate(0, 10);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // ---- Helpers -----------------------------------------------------------

    auto addSection = [&](const wxString& title)
    {
        sizer->AddSpacer(6);
        wxStaticLine* sep = new wxStaticLine(scroll, wxID_ANY,
                                              wxDefaultPosition, wxDefaultSize,
                                              wxLI_HORIZONTAL);
        sep->SetBackgroundColour(OCXTheme::BgSash());
        sizer->Add(sep, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);

        wxStaticText* lbl = new wxStaticText(scroll, wxID_ANY, title.Upper());
        lbl->SetForegroundColour(OCXTheme::FgDim());
        wxFont f = lbl->GetFont();
        f.MakeSmaller();
        f.SetWeight(wxFONTWEIGHT_BOLD);
        lbl->SetFont(f);
        sizer->Add(lbl, 0, wxLEFT | wxTOP | wxBOTTOM, 5);
    };

    auto addBtn = [&](int id, const wxString& label)
    {
        wxButton* btn = new wxButton(scroll, id, label,
                                     wxDefaultPosition, wxSize(144, 28));
        btn->SetBackgroundColour(OCXTheme::BgButton());
        btn->SetForegroundColour(OCXTheme::FgText());
        sizer->Add(btn, 0, wxLEFT | wxBOTTOM, 6);
    };

    auto addPair = [&](int idL, const wxString& lblL, int idR, const wxString& lblR)
    {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        auto makeBtn = [&](int id, const wxString& lbl) -> wxButton* {
            wxButton* b = new wxButton(scroll, id, lbl,
                                       wxDefaultPosition, wxSize(68, 26));
            b->SetBackgroundColour(OCXTheme::BgButton());
            b->SetForegroundColour(OCXTheme::FgText());
            return b;
        };
        row->Add(makeBtn(idL, lblL), 0, wxRIGHT, 4);
        row->Add(makeBtn(idR, lblR));
        sizer->Add(row, 0, wxLEFT | wxBOTTOM, 6);
    };

    // ---- Simulation --------------------------------------------------------
    addSection("Simulation");
    addBtn(ID_AP_Run,  "Run Sim");
    addBtn(ID_AP_Tick, "Tick Clock");

    // ---- Animation ---------------------------------------------------------
    addSection("Animate");

    wxBoxSizer* ppRow = new wxBoxSizer(wxHORIZONTAL);
    m_btnPlay  = new wxButton(scroll, ID_AP_AnimPlay,  "Play",
                               wxDefaultPosition, wxSize(68, 28));
    m_btnPause = new wxButton(scroll, ID_AP_AnimPause, "Pause",
                               wxDefaultPosition, wxSize(68, 28));
    m_btnPlay ->SetBackgroundColour(wxColour(40, 110, 50));
    m_btnPause->SetBackgroundColour(wxColour(90, 60, 20));
    m_btnPlay ->SetForegroundColour(OCXTheme::FgText());
    m_btnPause->SetForegroundColour(OCXTheme::FgText());
    ppRow->Add(m_btnPlay,  0, wxRIGHT, 4);
    ppRow->Add(m_btnPause);
    sizer->Add(ppRow, 0, wxLEFT | wxBOTTOM, 6);

    addBtn(ID_AP_AnimStep, "Step");

    wxStaticText* speedLbl = new wxStaticText(scroll, wxID_ANY, "SPEED");
    speedLbl->SetForegroundColour(OCXTheme::FgDim());
    wxFont sf = speedLbl->GetFont();
    sf.MakeSmaller();
    sf.SetWeight(wxFONTWEIGHT_BOLD);
    speedLbl->SetFont(sf);
    sizer->Add(speedLbl, 0, wxLEFT | wxTOP, 6);

    m_speedSlider = new wxSlider(scroll, ID_AP_AnimSpeed, 5, 1, 10,
                                  wxDefaultPosition, wxSize(144, -1),
                                  wxSL_HORIZONTAL | wxSL_AUTOTICKS);
    m_speedSlider->SetBackgroundColour(OCXTheme::BgPanel());
    sizer->Add(m_speedSlider, 0, wxLEFT | wxTOP | wxBOTTOM, 6);

    wxBoxSizer* slRow = new wxBoxSizer(wxHORIZONTAL);
    auto makeTiny = [&](const wxString& text) {
        wxStaticText* t = new wxStaticText(scroll, wxID_ANY, text);
        t->SetForegroundColour(OCXTheme::FgDim());
        wxFont ff = t->GetFont();
        ff.MakeSmaller();
        t->SetFont(ff);
        return t;
    };
    slRow->Add(makeTiny("Slow"), 0, wxLEFT, 6);
    slRow->AddStretchSpacer();
    slRow->Add(makeTiny("Fast"), 0, wxRIGHT, 10);
    sizer->Add(slRow, 0, wxEXPAND);
    sizer->AddSpacer(4);

    // Record VCD button
    m_btnRecord = new wxButton(scroll, ID_AP_Record, "Record VCD",
                               wxDefaultPosition, wxSize(144, 28));
    m_btnRecord->SetBackgroundColour(wxColour(140, 30, 30));
    m_btnRecord->SetForegroundColour(OCXTheme::FgText());
    m_btnRecord->SetToolTip("Record animation output signals as a .vcd waveform (max 200 ticks).\nAuto-loads in the Waveform tab when done.");
    sizer->Add(m_btnRecord, 0, wxLEFT | wxBOTTOM, 6);

    // ---- Edit --------------------------------------------------------------
    addSection("Edit");
    addPair(ID_AP_Undo, "Undo", ID_AP_Redo, "Redo");

    // ---- Zoom --------------------------------------------------------------
    addSection("Zoom");
    addPair(ID_AP_ZoomIn,  "Z+",  ID_AP_ZoomOut,   "Z-");
    addPair(ID_AP_ZoomFit, "Fit", ID_AP_ZoomReset, "100%");

    // ---- Align -------------------------------------------------------------
    addSection("Align");
    addPair(ID_AP_AlignLeft,    "Left",   ID_AP_AlignRight,   "Right");
    addPair(ID_AP_AlignTop,     "Top",    ID_AP_AlignBottom,  "Bottom");
    addPair(ID_AP_AlignCenterH, "Ctr H",  ID_AP_AlignCenterV, "Ctr V");

    // ---- Canvas ------------------------------------------------------------
    addSection("Canvas");
    addBtn(ID_AP_Save,  "Save Canvas");
    addBtn(ID_AP_Load,  "Load Canvas");
    addBtn(ID_AP_Clear, "Clear All");

    // ---- Export ------------------------------------------------------------
    addSection("Export");
    addBtn(ID_AP_ExportVHDL,    "Export VHDL");
    addBtn(ID_AP_ExportVerilog, "Export Verilog");
    addBtn(ID_AP_ExportPNG,     "Export PNG");

    sizer->AddSpacer(10);

    scroll->SetSizer(sizer);
    scroll->FitInside();

    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(scroll, 1, wxEXPAND);
    SetSizer(outer);

    // ---- Event bindings ----------------------------------------------------
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnRunSim,       this, ID_AP_Run);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnTickClock,    this, ID_AP_Tick);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnUndo,         this, ID_AP_Undo);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnRedo,         this, ID_AP_Redo);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnSaveCanvas,   this, ID_AP_Save);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnLoadCanvas,   this, ID_AP_Load);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnClear,        this, ID_AP_Clear);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnExportVHDL,   this, ID_AP_ExportVHDL);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnExportVerilog,this, ID_AP_ExportVerilog);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnExportPNG,    this, ID_AP_ExportPNG);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnAnimPlay,     this, ID_AP_AnimPlay);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnAnimPause,    this, ID_AP_AnimPause);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnAnimStep,     this, ID_AP_AnimStep);
    Bind(wxEVT_SLIDER, &CanvasActionPanel::OnAnimSpeed,    this, ID_AP_AnimSpeed);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnRecord,       this, ID_AP_Record);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnAlign,        this,
         ID_AP_AlignLeft, ID_AP_AlignCenterV);
    Bind(wxEVT_BUTTON, &CanvasActionPanel::OnCanvasZoom,   this,
         ID_AP_ZoomIn, ID_AP_ZoomReset);
}

//--
// Handlers
//--
void CanvasActionPanel::OnRunSim(wxCommandEvent&)    { m_canvas->RunSimulation(); }
void CanvasActionPanel::OnTickClock(wxCommandEvent&) { m_canvas->TickClocks();    }
void CanvasActionPanel::OnUndo(wxCommandEvent&)      { m_canvas->UndoCanvas();    }
void CanvasActionPanel::OnRedo(wxCommandEvent&)      { m_canvas->RedoCanvas();    }

void CanvasActionPanel::OnSaveCanvas(wxCommandEvent&)
{
    wxFileDialog dlg(this, "Save Canvas", "", "",
                     "OCX Schematic (*.ocxschem)|*.ocxschem|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    if (!m_canvas->SaveCanvas(dlg.GetPath()))
        wxMessageBox("Could not save canvas.", "Error", wxOK | wxICON_ERROR);
}

void CanvasActionPanel::OnLoadCanvas(wxCommandEvent&)
{
    wxFileDialog dlg(this, "Load Canvas", "", "",
                     "OCX Schematic (*.ocxschem)|*.ocxschem|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;
    if (!m_canvas->LoadCanvas(dlg.GetPath()))
        wxMessageBox("Could not load canvas.", "Error", wxOK | wxICON_ERROR);
}

void CanvasActionPanel::OnClear(wxCommandEvent&)
{
    if (wxMessageBox("Clear all gates and wires?", "Confirm",
                     wxYES_NO | wxICON_QUESTION) == wxYES)
        m_canvas->ClearAll();
}

static void ShowExportDialog(wxWindow* parent,
                              LogicEditorPanel* editor,
                              std::function<void()> switchToEditor,
                              const wxString& content,
                              const wxString& tabName,
                              const wxString& dialogTitle)
{
    if (editor)
    {
        editor->NewTabWithContent(tabName, content);
        if (switchToEditor) switchToEditor();
    }
    else
    {
        wxDialog dlg(parent, wxID_ANY, dialogTitle,
                     wxDefaultPosition, wxSize(600, 420));
        dlg.SetBackgroundColour(OCXTheme::BgPanel());
        wxTextCtrl* tc = new wxTextCtrl(&dlg, wxID_ANY, content,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxTE_MULTILINE | wxTE_READONLY |
                                         wxHSCROLL | wxBORDER_NONE);
        tc->SetBackgroundColour(OCXTheme::BgOutput());
        tc->SetForegroundColour(OCXTheme::FgText());
        wxFont mono(OCXTheme::FontSize() - 1, wxFONTFAMILY_TELETYPE,
                    wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                    OCXTheme::FontFace());
        tc->SetFont(mono);
        wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
        s->Add(tc, 1, wxEXPAND | wxALL, 8);
        s->Add(dlg.CreateButtonSizer(wxOK), 0,
               wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
        dlg.SetSizer(s);
        dlg.ShowModal();
    }
}

void CanvasActionPanel::OnExportVHDL(wxCommandEvent&)
{
    ShowExportDialog(this, m_editor, m_switchToEditor,
                     m_canvas->ExportToVHDL(),
                     "circuit_export.vhd", "VHDL Export");
}

void CanvasActionPanel::OnExportVerilog(wxCommandEvent&)
{
    ShowExportDialog(this, m_editor, m_switchToEditor,
                     m_canvas->ExportToVerilog(),
                     "circuit_export.v", "Verilog Export");
}

void CanvasActionPanel::OnExportPNG(wxCommandEvent&)
{
    wxFileDialog dlg(this, "Export Canvas as PNG", "", "circuit.png",
                     "PNG Image (*.png)|*.png|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    if (!m_canvas->ExportToPNG(dlg.GetPath()))
        wxMessageBox("Could not export PNG.\nEnsure there are gates on the canvas.",
                     "Export PNG", wxOK | wxICON_ERROR);
}

void CanvasActionPanel::OnCanvasZoom(wxCommandEvent& event)
{
    switch (event.GetId())
    {
    case ID_AP_ZoomIn:    m_canvas->ZoomIn();    break;
    case ID_AP_ZoomOut:   m_canvas->ZoomOut();   break;
    case ID_AP_ZoomReset: m_canvas->ZoomReset(); break;
    case ID_AP_ZoomFit:   m_canvas->ZoomToFit(); break;
    }
}

void CanvasActionPanel::OnAlign(wxCommandEvent& event)
{
    switch (event.GetId())
    {
    case ID_AP_AlignLeft:    m_canvas->AlignLeft();    break;
    case ID_AP_AlignRight:   m_canvas->AlignRight();   break;
    case ID_AP_AlignTop:     m_canvas->AlignTop();     break;
    case ID_AP_AlignBottom:  m_canvas->AlignBottom();  break;
    case ID_AP_AlignCenterH: m_canvas->AlignCenterH(); break;
    case ID_AP_AlignCenterV: m_canvas->AlignCenterV(); break;
    }
}

void CanvasActionPanel::UpdateAnimButtons()
{
    bool anim = m_canvas->IsAnimating();
    if (m_btnPlay)
        m_btnPlay->SetBackgroundColour(anim ? wxColour(20, 60, 25) : wxColour(40, 110, 50));
    if (m_btnPause)
        m_btnPause->SetBackgroundColour(anim ? wxColour(140, 90, 20) : wxColour(60, 45, 20));
    if (m_btnPlay)  m_btnPlay->Refresh();
    if (m_btnPause) m_btnPause->Refresh();

    // Reset record button if recording has stopped
    if (m_btnRecord && !m_canvas->IsRecording())
    {
        m_btnRecord->SetLabel("Record VCD");
        m_btnRecord->SetBackgroundColour(wxColour(140, 30, 30));
        m_btnRecord->Refresh();
    }
}

void CanvasActionPanel::OnAnimPlay(wxCommandEvent&)
{
    m_canvas->StartAnimation();
    UpdateAnimButtons();
}

void CanvasActionPanel::OnAnimPause(wxCommandEvent&)
{
    m_canvas->StopAnimation();
    UpdateAnimButtons();
}

void CanvasActionPanel::OnAnimStep(wxCommandEvent&)
{
    if (m_canvas->IsAnimating())
    {
        m_canvas->StopAnimation();
        UpdateAnimButtons();
    }
    m_canvas->StepAnimation();
}

void CanvasActionPanel::OnAnimSpeed(wxCommandEvent&)
{
    if (!m_speedSlider) return;
    int val = m_speedSlider->GetValue(); // 1=slow … 10=fast
    int ms  = 2100 - val * 200;          // 1→1900ms, 10→100ms
    m_canvas->SetAnimSpeed(ms);
}

void CanvasActionPanel::OnRecord(wxCommandEvent&)
{
    if (m_canvas->IsRecording())
    {
        // Stop an in-progress recording manually
        m_canvas->StopRecording();
        if (m_btnRecord)
        {
            m_btnRecord->SetLabel("Record VCD");
            m_btnRecord->SetBackgroundColour(wxColour(140, 30, 30));
        }
        return;
    }

    // Ask where to save the VCD
    wxFileDialog dlg(this, "Save Simulation VCD", "", "canvas_sim.vcd",
                     "VCD files (*.vcd)|*.vcd",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    m_canvas->StartRecording(dlg.GetPath());

    // Start animation automatically if not already running
    if (!m_canvas->IsAnimating())
    {
        m_canvas->StartAnimation();
        UpdateAnimButtons();
    }

    if (m_btnRecord)
    {
        m_btnRecord->SetLabel("Stop Recording");
        m_btnRecord->SetBackgroundColour(wxColour(200, 40, 40));
    }
}
