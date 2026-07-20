#include "component_palette.h"
#include "ui/canvas/circuit_canvas.h"
#include "ui/editor/logic_editor_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/filedlg.h>
#include <wx/scrolwin.h>

enum
{
    // Gates
    ID_PlaceAND    = wxID_HIGHEST + 200,
    ID_PlaceOR,
    ID_PlaceNOT,
    ID_PlaceNAND,
    ID_PlaceNOR,
    ID_PlaceXOR,
    ID_PlaceXNOR,
    ID_PlaceDFF,
    // Combinational
    ID_PlaceMUX,
    ID_PlaceHA,
    ID_PlaceFA,
    // Sequential / Sources
    ID_PlaceSRLatch,
    ID_PlaceJKFF,
    ID_PlaceTFF,
    ID_PlaceClock,
    // Ports
    ID_PlaceInput,
    ID_PlaceOutput,
    // Routing
    ID_PlaceTristate,
    ID_PlaceDemux
};

ComponentPalette::ComponentPalette(wxWindow* parent, CircuitCanvas* canvasPtr,
                                   LogicEditorPanel* /*editor*/,
                                   std::function<void()> /*switchToEditor*/)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(160, -1))
    , canvas(canvasPtr)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    // ---- Scrollable inner container ----------------------------------------
    wxScrolledWindow* scroll = new wxScrolledWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxVSCROLL | wxBORDER_NONE);
    scroll->SetBackgroundColour(OCXTheme::BgPanel());
    scroll->SetScrollRate(0, 10);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // ---- Helpers -----------------------------------------------------------

    // Section header: a thin separator line + dim uppercase label
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

    // Full-width button
    auto addBtn = [&](int id, const wxString& label)
    {
        wxButton* btn = new wxButton(scroll, id, label,
                                     wxDefaultPosition, wxSize(144, 28));
        btn->SetBackgroundColour(OCXTheme::BgButton());
        btn->SetForegroundColour(OCXTheme::FgText());
        sizer->Add(btn, 0, wxLEFT | wxBOTTOM, 6);
    };

    // Paired 2-column button row
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

    // ---- Gates -------------------------------------------------------------
    addSection("Gates");
    addBtn(ID_PlaceAND,  "AND");
    addBtn(ID_PlaceOR,   "OR");
    addBtn(ID_PlaceNOT,  "NOT");
    addBtn(ID_PlaceNAND, "NAND");
    addBtn(ID_PlaceNOR,  "NOR");
    addBtn(ID_PlaceXOR,  "XOR");
    addBtn(ID_PlaceXNOR, "XNOR");
    addBtn(ID_PlaceDFF,  "D Flip-Flop");

    // ---- Combinational -----------------------------------------------------
    addSection("Combinational");
    addBtn(ID_PlaceMUX, "MUX 2:1");
    addBtn(ID_PlaceHA,  "Half Adder");
    addBtn(ID_PlaceFA,  "Full Adder");

    // ---- Sequential --------------------------------------------------------
    addSection("Sequential");
    addBtn(ID_PlaceSRLatch, "SR Latch");
    addBtn(ID_PlaceJKFF,    "JK Flip-Flop");
    addBtn(ID_PlaceTFF,     "T Flip-Flop");
    addBtn(ID_PlaceClock,   "Clock");

    // ---- Routing -----------------------------------------------------------
    addSection("Routing");
    addBtn(ID_PlaceTristate, "Tri-state");
    addBtn(ID_PlaceDemux,    "DEMUX 1:2");

    // ---- Ports -------------------------------------------------------------
    addSection("Ports");
    addBtn(ID_PlaceInput,  "Input Port");
    addBtn(ID_PlaceOutput, "Output Port");

    sizer->AddSpacer(10);

    // ---- Wire up scroll window sizer and virtual size ----------------------
    scroll->SetSizer(sizer);
    scroll->FitInside();  // sets virtual height from sizer content

    // Outer panel sizer - scroll fills the full palette area
    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(scroll, 1, wxEXPAND);
    SetSizer(outerSizer);

    // ---- Event bindings (on outer panel - events bubble up from scroll) ----
    Bind(wxEVT_BUTTON, &ComponentPalette::OnPlaceGate, this, ID_PlaceAND, ID_PlaceDemux);
}

void ComponentPalette::OnPlaceGate(wxCommandEvent& event)
{
    GateType type;
    switch (event.GetId())
    {
    case ID_PlaceAND:      type = GateType::AND;        break;
    case ID_PlaceOR:       type = GateType::OR;         break;
    case ID_PlaceNOT:      type = GateType::NOT;        break;
    case ID_PlaceNAND:     type = GateType::NAND;       break;
    case ID_PlaceNOR:      type = GateType::NOR;        break;
    case ID_PlaceXOR:      type = GateType::XOR;        break;
    case ID_PlaceXNOR:     type = GateType::XNOR;       break;
    case ID_PlaceDFF:      type = GateType::DFLIPFLOP;  break;
    case ID_PlaceSRLatch:  type = GateType::SRLATCH;    break;
    case ID_PlaceJKFF:     type = GateType::JKFLIPFLOP; break;
    case ID_PlaceTFF:      type = GateType::TFLIPFLOP;  break;
    case ID_PlaceMUX:      type = GateType::MUX;        break;
    case ID_PlaceHA:       type = GateType::HALFADDER;  break;
    case ID_PlaceFA:       type = GateType::FULLADDER;  break;
    case ID_PlaceClock:    type = GateType::CLOCK;      break;
    case ID_PlaceInput:    type = GateType::INPUT;      break;
    case ID_PlaceOutput:    type = GateType::OUTPUT;    break;
    case ID_PlaceTristate:  type = GateType::TRISTATE;  break;
    case ID_PlaceDemux:     type = GateType::DEMUX;     break;
    default: return;
    }
    canvas->SetPlacementMode(type);
    canvas->SetFocus();
}

