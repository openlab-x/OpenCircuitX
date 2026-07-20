#pragma once
#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/filepicker.h>
#include <wx/choice.h>
#include <wx/stattext.h>

class NewProjectDialog : public wxDialog
{
public:
    NewProjectDialog(wxWindow* parent);

    wxString GetProjectName()      const;
    wxString GetProjectDirectory() const;
    // 0=Blank, 1=4-bit Counter, 2=4-bit ALU, 3=Traffic Light FSM,
    // 4=UART TX, 5=D Flip-Flop, 6=7-Seg Decoder, 7=Full Adder, 8=JK Flip-Flop
    int      GetTemplateIndex()    const;

private:
    void OnTemplateChanged(wxCommandEvent& event);

    wxTextCtrl*      m_projectNameCtrl;
    wxDirPickerCtrl* m_dirPicker;
    wxChoice*        m_templateChoice;
    wxStaticText*    m_templateDesc;
    wxTextCtrl*      m_previewCtrl;

    static const wxString s_descriptions[];
    static const wxString s_previews[];

    wxDECLARE_EVENT_TABLE();
};
