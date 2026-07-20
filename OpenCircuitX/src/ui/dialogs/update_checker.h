#pragma once

#include <wx/wx.h>
#include <wx/dialog.h>

// Performs a synchronous HTTP fetch of OCX_UPDATE_URL and compares
// the returned version string against OCX_VERSION_STRING.
// Shows a dialog with the result.
class UpdateCheckerDialog : public wxDialog
{
public:
    UpdateCheckerDialog(wxWindow* parent);

private:
    void CheckForUpdates();

    wxStaticText* m_statusLabel;
    wxButton*     m_downloadBtn;
    wxButton*     m_okBtn;
};
