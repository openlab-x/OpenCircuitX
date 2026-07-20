#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>

class SettingsDialog : public wxDialog
{
public:
    SettingsDialog(wxWindow* parent);

    // Simulation tools
    wxString GetGHDLPath() const;
    wxString GetIcarusPath() const;
    wxString GetVerilatorPath() const;
    wxString GetStopTime() const;
    int      GetAutoSaveInterval() const;  // returns value in minutes (1-30)

    // FPGA tools
    wxString GetYosysPath() const;
    wxString GetNextpnrPath() const;
    wxString GetIcepackPath() const;
    wxString GetEcppackPath() const;
    wxString GetNextpnrECP5Path() const;
    wxString GetOpenFPGALoaderPath() const;
    wxString GetGHDLPluginPath() const;
    wxString GetBoardId() const;

private:
    void OnBrowseGHDL(wxCommandEvent& event);
    void OnBrowseIcarus(wxCommandEvent& event);
    void OnBrowseVerilator(wxCommandEvent& event);
    void OnBrowseYosys(wxCommandEvent& event);
    void OnBrowseNextpnr(wxCommandEvent& event);
    void OnBrowseIcepack(wxCommandEvent& event);
    void OnBrowseEcppack(wxCommandEvent& event);
    void OnBrowseNextpnrECP5(wxCommandEvent& event);
    void OnBrowseLoader(wxCommandEvent& event);
    void OnBrowseGHDLPlugin(wxCommandEvent& event);

    // Simulation tab
    wxSpinCtrl* m_autoSaveSpinCtrl;
    wxTextCtrl* m_ghdlPathCtrl;
    wxTextCtrl* m_icarusPathCtrl;
    wxTextCtrl* m_verilatorPathCtrl;
    wxTextCtrl* m_stopTimeCtrl;

    // FPGA tab
    wxTextCtrl* m_yosysPathCtrl;
    wxTextCtrl* m_nextpnrPathCtrl;
    wxTextCtrl* m_icepackPathCtrl;
    wxTextCtrl* m_ecppackPathCtrl;
    wxTextCtrl* m_nextpnrECP5PathCtrl;
    wxTextCtrl* m_loaderPathCtrl;
    wxTextCtrl* m_ghdlPluginPathCtrl;
    wxChoice*   m_boardChoice;

    wxDECLARE_EVENT_TABLE();
};
