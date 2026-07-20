#include "open_project_dialog.h"

enum
{
    ID_Browse = 1,
    ID_Open
};

wxBEGIN_EVENT_TABLE(OpenProjectDialog, wxDialog)
EVT_BUTTON(ID_Browse, OpenProjectDialog::OnBrowse)
EVT_BUTTON(ID_Open, OpenProjectDialog::OnOpen)
wxEND_EVENT_TABLE()

OpenProjectDialog::OpenProjectDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Open Project", wxDefaultPosition, wxSize(400, 150))
{
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    // Text field to show selected project file
    wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* label = new wxStaticText(this, wxID_ANY, "Project File:");
    projectFileCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(250, -1));
    wxButton* browseButton = new wxButton(this, ID_Browse, "Browse");
    hbox1->Add(label, 0, wxRIGHT, 8);
    hbox1->Add(projectFileCtrl, 1);
    hbox1->Add(browseButton, 0, wxLEFT, 8);

    // Open and Cancel buttons
    wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
    wxButton* openButton = new wxButton(this, ID_Open, "Open");
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    hbox2->Add(openButton, 1);
    hbox2->Add(cancelButton, 1, wxLEFT, 8);

    vbox->Add(hbox1, 1, wxEXPAND | wxALL, 10);
    vbox->Add(hbox2, 0, wxALIGN_CENTER | wxBOTTOM | wxLEFT | wxRIGHT, 10);

    SetSizerAndFit(vbox);
}

void OpenProjectDialog::OnBrowse(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, "Open Project", "", "",
        "OpenCircuitX Project files (*.ocxproj)|*.ocxproj", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    // Set the selected file path in the text field
    projectFileCtrl->SetValue(openFileDialog.GetPath());
}

void OpenProjectDialog::OnOpen(wxCommandEvent& event)
{
    if (projectFileCtrl->GetValue().IsEmpty())
    {
        wxMessageBox("Please select a project file.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    EndModal(wxID_OK);
}

wxString OpenProjectDialog::GetProjectFilePath() const
{
    return projectFileCtrl->GetValue();
}
