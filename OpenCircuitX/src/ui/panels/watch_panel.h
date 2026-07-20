#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "core/simulation/vcd_parser.h"
#include <vector>

// Signal watch panel - part of the debugger (Phase 10).
// Shows a list of user-chosen signal names and their value at the
// current waveform cursor position.

class WatchPanel : public wxPanel
{
public:
    WatchPanel(wxWindow* parent);

    void ReapplyTheme();

    // Append a signal name to the watch list.
    void AddSignal(const wxString& name);

    // Refresh all Value cells for the given cursor time.
    // tracks may be nullptr (all values shown as "--").
    void UpdateValues(long long cursorTime,
                      const std::vector<WfTrack>* tracks);

private:
    wxListCtrl* m_list;
    wxPanel*    m_bar = nullptr;

    void OnAdd(wxCommandEvent& event);
    void OnRemove(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);
};
