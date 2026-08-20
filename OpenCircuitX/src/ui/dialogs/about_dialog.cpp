#include "about_dialog.h"
#include "ui/shell/app_theme.h"
#include "core/version.h"
#include <wx/statline.h>

AboutDialog::AboutDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "About OpenCircuitX",
               wxDefaultPosition, wxSize(420, 320),
               wxDEFAULT_DIALOG_STYLE)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);

    // App name
    wxStaticText* appName = new wxStaticText(this, wxID_ANY, "OpenCircuitX");
    appName->SetFont(wxFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                            wxFONTWEIGHT_BOLD, false, "Consolas"));
    appName->SetForegroundColour(OCXTheme::Accent());
    appName->SetBackgroundColour(OCXTheme::BgPanel());

    // Version
    wxStaticText* version = new wxStaticText(
        this, wxID_ANY, wxString("Version ") + OCX_VERSION_STRING);
    version->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                            wxFONTWEIGHT_NORMAL, false, "Consolas"));
    version->SetForegroundColour(OCXTheme::FgDim());
    version->SetBackgroundColour(OCXTheme::BgPanel());

    // Description
    wxStaticText* desc = new wxStaticText(
        this, wxID_ANY,
        "A professional EDA platform with a Circuit & HDL editor\n"
        "for VHDL, Verilog, and digital logic design.",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    desc->SetForegroundColour(OCXTheme::FgText());
    desc->SetBackgroundColour(OCXTheme::BgPanel());

    // Separator
    wxStaticLine* line1 = new wxStaticLine(this, wxID_ANY);

    // Powered by
    // Middle dot via wxString::FromUTF8 with explicit UTF-8 bytes, not a
    // \uXXXX escape in a narrow literal - the escape's byte encoding
    // depends on the compiler's execution charset (CP1252 here), and wx's
    // runtime decode assumes UTF-8, producing mojibake either way the two
    // disagree. List widened from "wxWidgets \u00B7 GHDL \u00B7 C++" to also credit
    // Icarus (Verilog simulation, equally core to GHDL/VHDL) and Yosys
    // (the whole FPGA synthesis feature) - both were real, load-bearing
    // dependencies missing from the credit line entirely.
    wxStaticText* poweredBy = new wxStaticText(
        this, wxID_ANY,
        wxString::FromUTF8("Powered by wxWidgets \xC2\xB7 GHDL \xC2\xB7 Icarus \xC2\xB7 Yosys \xC2\xB7 C++"),
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    poweredBy->SetForegroundColour(OCXTheme::FgDim());
    poweredBy->SetBackgroundColour(OCXTheme::BgPanel());

    // Made with <3 - three adjacent controls, not one string, so the heart
    // can have its own colour. The original used the orange heart emoji
    // (U+1F9E1), a supplementary-plane codepoint that needs a colour-emoji
    // font to render - GTK/fontconfig on Linux reliably falls back to one,
    // classic Windows static-text rendering doesn't, so it just showed
    // nothing there. Plain "hearts suit" (U+2665) needs no emoji font at
    // all. Coloured with a literal orange, not OCXTheme::Accent() - that's
    // actually blue in every theme variant (VS Code-style accent), not
    // orange, so reusing it would have quietly swapped the heart's colour
    // instead of fixing it. No orange exists anywhere in OCXTheme to reuse.
    wxBoxSizer* madeBySizer = new wxBoxSizer(wxHORIZONTAL);
    wxFont      madeByFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    const wxColour kHeartOrange(244, 144, 12);

    wxStaticText* madeByPre = new wxStaticText(this, wxID_ANY, "Made with ");
    madeByPre->SetFont(madeByFont);
    madeByPre->SetForegroundColour(OCXTheme::FgText());
    madeByPre->SetBackgroundColour(OCXTheme::BgPanel());

    wxStaticText* madeByHeart = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("\xE2\x99\xA5"));
    madeByHeart->SetFont(madeByFont);
    madeByHeart->SetForegroundColour(kHeartOrange);
    madeByHeart->SetBackgroundColour(OCXTheme::BgPanel());

    wxStaticText* madeByPost = new wxStaticText(this, wxID_ANY, " by OpenLabX");
    madeByPost->SetFont(madeByFont);
    madeByPost->SetForegroundColour(OCXTheme::FgText());
    madeByPost->SetBackgroundColour(OCXTheme::BgPanel());

    madeBySizer->Add(madeByPre,   0, wxALIGN_CENTER_VERTICAL);
    madeBySizer->Add(madeByHeart, 0, wxALIGN_CENTER_VERTICAL);
    madeBySizer->Add(madeByPost,  0, wxALIGN_CENTER_VERTICAL);

    // License
    wxStaticText* license = new wxStaticText(
        this, wxID_ANY,
        "Released under the MIT License.",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    license->SetForegroundColour(OCXTheme::FgDim());
    license->SetBackgroundColour(OCXTheme::BgPanel());

    // Separator
    wxStaticLine* line2 = new wxStaticLine(this, wxID_ANY);

    // OK button
    wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
    okBtn->SetBackgroundColour(OCXTheme::Accent());
    okBtn->SetForegroundColour(*wxWHITE);

    root->AddSpacer(20);
    root->Add(appName,   0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(4);
    root->Add(version,   0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(14);
    root->Add(desc,      0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(14);
    root->Add(line1,     0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(12);
    root->Add(poweredBy,  0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(8);
    root->Add(madeBySizer, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(8);
    root->Add(license,   0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(14);
    root->Add(line2,     0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(12);
    root->Add(okBtn,     0, wxALIGN_CENTER | wxBOTTOM, 16);

    SetSizerAndFit(root);
    Centre();
}
