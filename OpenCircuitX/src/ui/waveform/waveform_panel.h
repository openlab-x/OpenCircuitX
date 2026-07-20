#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/grid.h>
#include <wx/splitter.h>
#include "core/simulation/vcd_parser.h"
#include <functional>
#include <vector>

class WfNamesPanel;
class WfCanvas;

class WaveformPanel : public wxPanel
{
public:
    WaveformPanel(wxWindow* parent);

    // Load and display a .vcd file.
    void LoadVCD(const wxString& filePath);

    // Save/restore the waveform view state to/from a .ocxwave session file.
    void SaveSession(const wxString& path);
    void LoadSession(const wxString& path);

    // Register a callback that fires whenever the cursor position changes.
    // The callback receives the cursor time in VCD time units.
    void SetCursorCallback(std::function<void(long long)> cb);

    // Callback fired when user double-clicks or right-clicks→Add to Watch.
    // Receives the signal name. Connect to WatchPanel::AddSignal().
    void SetWatchCallback(std::function<void(const wxString&)> cb);

    // Access the parsed signal tracks (nullptr if no VCD loaded).
    const std::vector<WfTrack>* GetTracks() const;

    // Returns the last timestamp in the loaded VCD (0 if none / empty).
    long long GetEndTime() const { return m_data.endTime; }

private:
    VcdData       m_data;
    WfNamesPanel* m_names;
    WfCanvas*     m_canvas;
    wxStaticText* m_timeLabel;
    wxStaticText* m_fileLabel;

    // Signal value table (truth-table view)
    wxSplitterWindow*     m_splitter   = nullptr;
    wxPanel*              m_wavePane   = nullptr;   // top pane of splitter
    wxPanel*              m_tablePane  = nullptr;   // bottom pane of splitter
    wxGrid*               m_tableGrid  = nullptr;
    std::vector<long long> m_tableRowTimes;          // row index → timestamp

    void OnLoadVCD(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);
    void OnFit(wxCommandEvent& event);
    void OnStepBack(wxCommandEvent& event);
    void OnStepFwd(wxCommandEvent& event);
    void OnPrevTrans(wxCommandEvent& event);
    void OnNextTrans(wxCommandEvent& event);
    void OnExportCSV(wxCommandEvent& event);
    void OnExportPNG(wxCommandEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnUnitChanged(wxCommandEvent& event);
    void OnSaveSession(wxCommandEvent& event);
    void OnLoadSession(wxCommandEvent& event);
    void OnFindNext(wxCommandEvent& event);
    void OnFindPrev(wxCommandEvent& event);
    void OnToggleTable(wxCommandEvent& event);

    void RefreshTable();
    void HighlightTableRow(long long cursorTime);

    wxString m_currentVCDPath;

    std::function<void(long long)>       m_cursorCallback;
    std::function<void(const wxString&)> m_watchCallback;

    // Full unfiltered track list - kept for signal filter
    std::vector<WfTrack> m_allTracks;

    wxTextCtrl* m_filterField   = nullptr;
    wxChoice*   m_unitChoice    = nullptr;
    wxChoice*   m_findSigChoice = nullptr;
    wxTextCtrl* m_findValField  = nullptr;
    wxButton*   m_tableBtnRef   = nullptr;   // ptr to the Table button (for label update)

    void Reload();
    void ApplyFilter();
};
