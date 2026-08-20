#include "settings_dialog.h"
#include "core/toolchain/fpga_toolchain.h"
#include <wx/filedlg.h>
#include <wx/config.h>
#include <wx/notebook.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/scrolwin.h>

enum
{
    ID_BrowseGHDL      = wxID_HIGHEST + 50,
    ID_BrowseIcarus,
    ID_BrowseVerilator,
    ID_BrowseYosys,
    ID_BrowseNextpnr,
    ID_BrowseNextpnrECP5,
    ID_BrowseIcepack,
    ID_BrowseEcppack,
    ID_BrowseLoader,
    ID_BrowseGHDLPlugin
};

wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(ID_BrowseGHDL,      SettingsDialog::OnBrowseGHDL)
    EVT_BUTTON(ID_BrowseIcarus,    SettingsDialog::OnBrowseIcarus)
    EVT_BUTTON(ID_BrowseVerilator, SettingsDialog::OnBrowseVerilator)
    EVT_BUTTON(ID_BrowseYosys,        SettingsDialog::OnBrowseYosys)
    EVT_BUTTON(ID_BrowseNextpnr,      SettingsDialog::OnBrowseNextpnr)
    EVT_BUTTON(ID_BrowseNextpnrECP5,  SettingsDialog::OnBrowseNextpnrECP5)
    EVT_BUTTON(ID_BrowseIcepack,      SettingsDialog::OnBrowseIcepack)
    EVT_BUTTON(ID_BrowseEcppack,      SettingsDialog::OnBrowseEcppack)
    EVT_BUTTON(ID_BrowseLoader,       SettingsDialog::OnBrowseLoader)
    EVT_BUTTON(ID_BrowseGHDLPlugin,   SettingsDialog::OnBrowseGHDLPlugin)
wxEND_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow* parent)
    // Height bumped from the original 580 to reduce how often the tab
    // content actually needs to scroll (see the tab panels below, both
    // wxScrolledWindow - that's the real fix for GTK rendering the same
    // content taller than Windows does; this is just a minor nicety).
    // Width bumped too - the vertical scrollbar the wxScrolledWindow tabs
    // can now show eats a slice of row width the original 580 didn't leave
    // room for, clipping the rightmost "Browse..." button on any row that
    // ends up scrolled (only tabs that actually need to scroll lose this
    // width, which is why Simulation looked fine while FPGA didn't). A
    // first +30 wasn't quite enough on the actual GTK theme in use;
    // widened further with real headroom instead of nudging by 10px at a
    // time - going too wide costs nothing but harmless empty space on
    // Windows or any tab that isn't scrolling, going too narrow clips a
    // button, so it's worth erring generous here.
    : wxDialog(parent, wxID_ANY, "Settings", wxDefaultPosition, wxSize(650, 680))
{
    wxConfig config("OpenCircuitX");

    // Load saved values
    wxString savedGHDL, savedIcarus, savedVerilator, savedStopTime;
    config.Read("GHDLPath",      &savedGHDL,      "ghdl");
    config.Read("IcarusPath",    &savedIcarus,    "iverilog");
    config.Read("VerilatorPath", &savedVerilator, "verilator");
    config.Read("StopTime",      &savedStopTime,  "");
    int savedAutoSave = 1;
    config.Read("AutoSaveInterval", &savedAutoSave, 1);

    wxString savedYosys, savedNextpnr, savedNextpnrECP5, savedIcepack, savedEcppack,
             savedLoader, savedGHDLPlugin, savedBoard;
    config.Read("YosysPath",         &savedYosys,         "yosys");
    config.Read("NextpnrPath",       &savedNextpnr,       "nextpnr-ice40");
    config.Read("NextpnrECP5Path",   &savedNextpnrECP5,   "nextpnr-ecp5");
    config.Read("IcepackPath",       &savedIcepack,       "icepack");
    config.Read("EcppackPath",       &savedEcppack,       "ecppack");
    config.Read("OpenFPGALoaderPath",&savedLoader,        "openFPGALoader");
    config.Read("GHDLYosysPlugin",   &savedGHDLPlugin,    "");
    config.Read("FPGABoard",         &savedBoard,         "ibreaker");

    // Helpers
    auto makeRow = [&](wxWindow* parent, const wxString& label,
                       wxTextCtrl*& ctrl, const wxString& value,
                       int browseId, wxBoxSizer* sizer)
    {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(parent, wxID_ANY, label),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        ctrl = new wxTextCtrl(parent, wxID_ANY, value,
                              wxDefaultPosition, wxSize(240, -1));
        row->Add(ctrl, 1, wxALIGN_CENTER_VERTICAL);
        if (browseId != wxID_NONE)
            row->Add(new wxButton(parent, browseId, "Browse..."), 0, wxLEFT, 8);
        sizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    };

    auto makeHint = [&](wxWindow* parent, const wxString& text,
                        wxBoxSizer* sizer)
    {
        wxStaticText* lbl = new wxStaticText(parent, wxID_ANY, text);
        lbl->SetForegroundColour(wxColour(120, 120, 120));
        sizer->Add(lbl, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    };

    // ---- Notebook ----
    wxNotebook* nb = new wxNotebook(this, wxID_ANY);

    // ---- Tab 1: Simulation ----
    // wxScrolledWindow, not wxPanel - the dialog is a fixed, non-resizable
    // size tuned for Windows' more compact widget metrics. GTK renders the
    // same content taller and had no way to grow the dialog to fit, so
    // rows overlapped instead. A scrollbar handles any height mismatch
    // properly regardless of platform, instead of guessing a fixed pixel
    // height that happens to work on whichever machine last tested it.
    wxScrolledWindow* simTab = new wxScrolledWindow(nb);
    simTab->SetScrollRate(0, 10);
    wxBoxSizer* simSizer = new wxBoxSizer(wxVERTICAL);
    simSizer->AddSpacer(8);

    makeRow(simTab, "GHDL executable:",           m_ghdlPathCtrl,    savedGHDL,      ID_BrowseGHDL,      simSizer);
    makeHint(simTab, "Enter 'ghdl' if on PATH, or browse to the executable.", simSizer);
    makeRow(simTab, "Icarus Verilog (iverilog):",  m_icarusPathCtrl,  savedIcarus,    ID_BrowseIcarus,    simSizer);
    makeHint(simTab, "Used for .v / .sv Compile + Run (F5/F6).", simSizer);
    makeRow(simTab, "Verilator:",                  m_verilatorPathCtrl, savedVerilator, ID_BrowseVerilator, simSizer);
    makeHint(simTab, "Used for Tools > Lint with Verilator (F8).", simSizer);
    makeRow(simTab, "Stop time:",                  m_stopTimeCtrl,    savedStopTime,  wxID_NONE,          simSizer);
    makeHint(simTab, "GHDL --stop-time (e.g. 100ns, 1us). Leave empty for default.", simSizer);

    // Auto-save interval
    {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(simTab, wxID_ANY, "Auto-save interval (minutes):"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        m_autoSaveSpinCtrl = new wxSpinCtrl(simTab, wxID_ANY,
                                             wxEmptyString, wxDefaultPosition, wxSize(70, -1),
                                             wxSP_ARROW_KEYS, 1, 30, savedAutoSave);
        row->Add(m_autoSaveSpinCtrl, 0, wxALIGN_CENTER_VERTICAL);
        simSizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    }
    makeHint(simTab, "How often unsaved editor files are saved automatically (1-30 min).", simSizer);

    simTab->SetSizer(simSizer);
    simTab->FitInside();   // computes the scrollable virtual size from simSizer
    nb->AddPage(simTab, "Simulation");

    // ---- Tab 2: FPGA ----
    wxScrolledWindow* fpgaTab = new wxScrolledWindow(nb);
    fpgaTab->SetScrollRate(0, 10);
    wxBoxSizer* fpgaSizer = new wxBoxSizer(wxVERTICAL);
    fpgaSizer->AddSpacer(8);

    makeRow(fpgaTab, "Yosys:",              m_yosysPathCtrl,       savedYosys,       ID_BrowseYosys,       fpgaSizer);
    makeHint(fpgaTab, "Open-source synthesis (iCE40 + ECP5). Enter 'yosys' if on PATH.", fpgaSizer);

    fpgaSizer->Add(new wxStaticText(fpgaTab, wxID_ANY, "-- iCE40 --"),
                   0, wxLEFT | wxBOTTOM, 10);
    makeRow(fpgaTab, "nextpnr-ice40:",      m_nextpnrPathCtrl,     savedNextpnr,     ID_BrowseNextpnr,     fpgaSizer);
    makeHint(fpgaTab, "Place & route for iCE40 boards (iCEBreaker, TinyFPGA BX, UPduino).", fpgaSizer);
    makeRow(fpgaTab, "icepack:",            m_icepackPathCtrl,     savedIcepack,     ID_BrowseIcepack,     fpgaSizer);
    makeHint(fpgaTab, "Packs .asc -> .bin bitstream (part of Project IceStorm).", fpgaSizer);

    fpgaSizer->Add(new wxStaticText(fpgaTab, wxID_ANY, "-- ECP5 --"),
                   0, wxLEFT | wxBOTTOM, 10);
    makeRow(fpgaTab, "nextpnr-ecp5:",       m_nextpnrECP5PathCtrl, savedNextpnrECP5, ID_BrowseNextpnrECP5, fpgaSizer);
    makeHint(fpgaTab, "Place & route for ECP5 boards (ColorLight, OrangeCrab, ULX3S).", fpgaSizer);
    makeRow(fpgaTab, "ecppack:",            m_ecppackPathCtrl,     savedEcppack,     ID_BrowseEcppack,     fpgaSizer);
    makeHint(fpgaTab, "Packs ECP5 .config -> .bit bitstream (part of Project Trellis).", fpgaSizer);

    fpgaSizer->Add(new wxStaticText(fpgaTab, wxID_ANY, "-- Common --"),
                   0, wxLEFT | wxBOTTOM, 10);
    makeRow(fpgaTab, "openFPGALoader:",     m_loaderPathCtrl,      savedLoader,      ID_BrowseLoader,      fpgaSizer);
    makeHint(fpgaTab, "Programs any supported FPGA board over USB.", fpgaSizer);
    makeRow(fpgaTab, "GHDL Yosys plugin:", m_ghdlPluginPathCtrl,  savedGHDLPlugin,  ID_BrowseGHDLPlugin,  fpgaSizer);
    makeHint(fpgaTab, "ghdl-yosys-plugin (.dll/.so/.dylib). Required for VHDL synthesis. Leave empty if on PATH.", fpgaSizer);

    // Board selector
    {
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(fpgaTab, wxID_ANY, "Target board:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        m_boardChoice = new wxChoice(fpgaTab, wxID_ANY,
                                      wxDefaultPosition, wxSize(260, -1));
        const auto& boards = FPGAToolchain::GetBoards();
        int selIdx = 0;
        for (int i = 0; i < (int)boards.size(); ++i)
        {
            m_boardChoice->Append(boards[i].name);
            if (boards[i].id == savedBoard)
                selIdx = i;
        }
        m_boardChoice->SetSelection(selIdx);
        row->Add(m_boardChoice, 1, wxALIGN_CENTER_VERTICAL);
        fpgaSizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    }

    fpgaTab->SetSizer(fpgaSizer);
    fpgaTab->FitInside();   // computes the scrollable virtual size from fpgaSizer
    nb->AddPage(fpgaTab, "FPGA");

    // ---- Root layout ----
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    root->Add(nb, 1, wxEXPAND | wxALL, 8);

    wxBoxSizer* btnRow = new wxBoxSizer(wxHORIZONTAL);
    btnRow->AddStretchSpacer();
    btnRow->Add(new wxButton(this, wxID_OK,     "OK"),     0, wxRIGHT, 8);
    btnRow->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    root->Add(btnRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizer(root);
}

//--
// Browse handlers
//--
static wxString BrowseExe(wxWindow* parent, const wxString& title)
{
    // Only Windows executables carry a .exe extension - on Linux/macOS every
    // toolchain binary (ghdl, iverilog, yosys, ...) has none, so defaulting
    // to a *.exe filter there hides every real match.
#ifdef __WXMSW__
    wxString wildcard = "Executables (*.exe)|*.exe|All files (*.*)|*.*";
#else
    wxString wildcard = "All files (*)|*";
#endif

    wxFileDialog dlg(parent, title, "", "", wildcard,
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
        return dlg.GetPath();
    return wxEmptyString;
}

void SettingsDialog::OnBrowseGHDL(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select GHDL executable");
    if (!p.IsEmpty()) m_ghdlPathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseIcarus(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select iverilog executable");
    if (!p.IsEmpty()) m_icarusPathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseVerilator(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select verilator executable");
    if (!p.IsEmpty()) m_verilatorPathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseYosys(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select yosys executable");
    if (!p.IsEmpty()) m_yosysPathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseNextpnr(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select nextpnr-ice40 executable");
    if (!p.IsEmpty()) m_nextpnrPathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseIcepack(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select icepack executable");
    if (!p.IsEmpty()) m_icepackPathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseEcppack(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select ecppack executable");
    if (!p.IsEmpty()) m_ecppackPathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseNextpnrECP5(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select nextpnr-ecp5 executable");
    if (!p.IsEmpty()) m_nextpnrECP5PathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseLoader(wxCommandEvent&)
{
    wxString p = BrowseExe(this, "Select openFPGALoader executable");
    if (!p.IsEmpty()) m_loaderPathCtrl->SetValue(p);
}

void SettingsDialog::OnBrowseGHDLPlugin(wxCommandEvent&)
{
    wxFileDialog dlg(this, "Select GHDL Yosys plugin", "", "",
                     "Plugin files (*.dll;*.so;*.dylib)|*.dll;*.so;*.dylib"
                     "|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
        m_ghdlPluginPathCtrl->SetValue(dlg.GetPath());
}

//--
// Accessors
//--
static wxString Trimmed(const wxString& s)
{
    wxString t = s; t.Trim(true); t.Trim(false); return t;
}

wxString SettingsDialog::GetGHDLPath()            const { return Trimmed(m_ghdlPathCtrl->GetValue());    }
wxString SettingsDialog::GetIcarusPath()           const { return Trimmed(m_icarusPathCtrl->GetValue());  }
wxString SettingsDialog::GetVerilatorPath()        const { return Trimmed(m_verilatorPathCtrl->GetValue());}
wxString SettingsDialog::GetStopTime()             const { return Trimmed(m_stopTimeCtrl->GetValue());    }
int      SettingsDialog::GetAutoSaveInterval()     const { return m_autoSaveSpinCtrl->GetValue();          }
wxString SettingsDialog::GetYosysPath()            const { return Trimmed(m_yosysPathCtrl->GetValue());   }
wxString SettingsDialog::GetNextpnrPath()          const { return Trimmed(m_nextpnrPathCtrl->GetValue()); }
wxString SettingsDialog::GetIcepackPath()          const { return Trimmed(m_icepackPathCtrl->GetValue()); }
wxString SettingsDialog::GetEcppackPath()          const { return Trimmed(m_ecppackPathCtrl->GetValue());       }
wxString SettingsDialog::GetNextpnrECP5Path()      const { return Trimmed(m_nextpnrECP5PathCtrl->GetValue());   }
wxString SettingsDialog::GetOpenFPGALoaderPath()   const { return Trimmed(m_loaderPathCtrl->GetValue());        }
wxString SettingsDialog::GetGHDLPluginPath()       const { return Trimmed(m_ghdlPluginPathCtrl->GetValue());    }

wxString SettingsDialog::GetBoardId() const
{
    int sel = m_boardChoice->GetSelection();
    if (sel < 0 || sel >= (int)FPGAToolchain::GetBoards().size())
        return "ibreaker";
    return FPGAToolchain::GetBoards()[sel].id;
}
