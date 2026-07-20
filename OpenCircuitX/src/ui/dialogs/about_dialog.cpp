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
    wxStaticText* poweredBy = new wxStaticText(
        this, wxID_ANY,
        "Powered by wxWidgets \u00B7 GHDL \u00B7 C++",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    poweredBy->SetForegroundColour(OCXTheme::FgDim());
    poweredBy->SetBackgroundColour(OCXTheme::BgPanel());

    // Made with love
    wxStaticText* madeBy = new wxStaticText(
        this, wxID_ANY,
        wxString("Made with ") + wxString::FromUTF8("\xF0\x9F\xA7\xA1") + " by OpenLabX",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    madeBy->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                           wxFONTWEIGHT_BOLD));
    madeBy->SetForegroundColour(OCXTheme::FgText());
    madeBy->SetBackgroundColour(OCXTheme::BgPanel());

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
    root->Add(poweredBy, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(8);
    root->Add(madeBy,    0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(8);
    root->Add(license,   0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(14);
    root->Add(line2,     0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(12);
    root->Add(okBtn,     0, wxALIGN_CENTER | wxBOTTOM, 16);

    SetSizerAndFit(root);
    Centre();
}
