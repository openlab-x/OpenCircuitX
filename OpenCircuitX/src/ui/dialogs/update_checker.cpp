#include "update_checker.h"
#include "ui/shell/app_theme.h"
#include "core/version.h"
#include "core/update_check.h"
#include <wx/utils.h>

UpdateCheckerDialog::UpdateCheckerDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Check for Updates",
               wxDefaultPosition, wxSize(420, 220),
               wxDEFAULT_DIALOG_STYLE)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);

    wxStaticText* title = new wxStaticText(this, wxID_ANY, "OpenCircuitX Update Check");
    title->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                          wxFONTWEIGHT_BOLD, false, "Consolas"));
    title->SetForegroundColour(OCXTheme::Accent());
    title->SetBackgroundColour(OCXTheme::BgPanel());

    wxStaticText* currentVer = new wxStaticText(
        this, wxID_ANY,
        wxString("Current version: ") + OCX_VERSION_STRING);
    currentVer->SetForegroundColour(OCXTheme::FgDim());
    currentVer->SetBackgroundColour(OCXTheme::BgPanel());

    m_statusLabel = new wxStaticText(
        this, wxID_ANY, "Checking...",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    m_statusLabel->SetForegroundColour(OCXTheme::FgText());
    m_statusLabel->SetBackgroundColour(OCXTheme::BgPanel());

    // Download button - hidden until an update is found
    m_downloadBtn = new wxButton(this, wxID_ANY, "  Download Latest Version  ");
    m_downloadBtn->SetBackgroundColour(wxColour(60, 180, 90));
    m_downloadBtn->SetForegroundColour(*wxWHITE);
    m_downloadBtn->SetToolTip("Downloads the installer for the newest release directly");
    m_downloadBtn->Hide();
    m_downloadBtn->Bind(wxEVT_BUTTON, [](wxCommandEvent&) {
        wxLaunchDefaultBrowser(OCX_LATEST_DOWNLOAD_URL);
    });

    m_okBtn = new wxButton(this, wxID_OK, "OK");
    m_okBtn->SetBackgroundColour(OCXTheme::Accent());
    m_okBtn->SetForegroundColour(*wxWHITE);
    m_okBtn->Enable(false);

    wxBoxSizer* btnRow = new wxBoxSizer(wxHORIZONTAL);
    btnRow->Add(m_downloadBtn, 0, wxRIGHT, 10);
    btnRow->Add(m_okBtn,       0);

    root->AddSpacer(20);
    root->Add(title,        0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(6);
    root->Add(currentVer,   0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(16);
    root->Add(m_statusLabel, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, 20);
    root->AddSpacer(16);
    root->Add(btnRow,        0, wxALIGN_CENTER | wxBOTTOM, 16);

    SetSizerAndFit(root);
    Centre();

    CallAfter(&UpdateCheckerDialog::CheckForUpdates);
}

void UpdateCheckerDialog::CheckForUpdates()
{
    wxString remote = OCXFetchRemoteVersion();

    if (remote.IsEmpty())
    {
        m_statusLabel->SetForegroundColour(wxColour(244, 135, 113));
        m_statusLabel->SetLabel(
            "Could not reach the update server.\n"
            "Check your internet connection.");
    }
    else
    {
        int cmp = OCXCompareVersions(OCX_VERSION_STRING, remote);
        if (cmp < 0)
        {
            m_statusLabel->SetForegroundColour(wxColour(100, 220, 100));
            m_statusLabel->SetLabel(
                wxString::Format("Version %s is available!", remote));
            m_downloadBtn->Show();
        }
        else if (cmp == 0)
        {
            m_statusLabel->SetForegroundColour(OCXTheme::FgText());
            m_statusLabel->SetLabel("You are up to date.");
        }
        else
        {
            m_statusLabel->SetForegroundColour(OCXTheme::FgDim());
            m_statusLabel->SetLabel("You are running a pre-release build.");
        }
    }

    m_statusLabel->Wrap(360);
    GetSizer()->Layout();
    Fit();
    m_okBtn->Enable(true);
}
