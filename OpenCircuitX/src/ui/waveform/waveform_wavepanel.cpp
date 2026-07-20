#include "waveform_panel.h"
#include "ui/waveform/waveform_internal.h"
#include "ui/shell/app_theme.h"
#include <wx/dcbuffer.h>
#include <wx/filedlg.h>
#include <wx/file.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/grid.h>
#include <wx/menu.h>
#include <wx/splitter.h>
#include <functional>
#include <climits>
#include <set>
#include <map>

//--
// WaveformPanel
//--
enum
{
    ID_WF_LoadVCD = wxID_HIGHEST + 200,
    ID_WF_ZoomIn,
    ID_WF_ZoomOut,
    ID_WF_Fit,
    ID_WF_StepBack,
    ID_WF_StepFwd,
    ID_WF_PrevTrans,
    ID_WF_NextTrans,
    ID_WF_ExportCSV,
    ID_WF_ExportPNG,
    ID_WF_Filter,
    ID_WF_UnitChoice,
    ID_WF_SaveSession,
    ID_WF_LoadSession,
    ID_WF_FindNext,
    ID_WF_FindPrev,
    ID_WF_FindSig,
    ID_WF_FindVal,
    ID_WF_ToggleTable
};

// Returns picoseconds per one raw VCD time unit.
// "1 ns" -> 1000.0,  "1 ps" -> 1.0,  "10 ps" -> 10.0,  "1 fs" -> 0.001
static double ParseTimescaleToPs(const wxString& ts)
{
    wxString s = ts;
    s.Trim(true).Trim(false).MakeLower();
    if (s.IsEmpty()) return 1.0;

    double mult = 1.0;
    size_t i = 0;
    while (i < s.Length() && (wxIsdigit(s[i]) || s[i] == '.')) ++i;
    if (i > 0) { double v; if (s.Left(i).ToDouble(&v)) mult = v; }
    while (i < s.Length() && s[i] == ' ') ++i;
    wxString unit = s.Mid(i).Trim();

    double psPer = 1.0;
    if      (unit == "fs")  psPer = 0.001;
    else if (unit == "ps")  psPer = 1.0;
    else if (unit == "ns")  psPer = 1000.0;
    else if (unit == "us")  psPer = 1.0e6;
    else if (unit == "ms")  psPer = 1.0e9;
    else if (unit == "s")   psPer = 1.0e12;

    return mult * psPer;
}

WaveformPanel::WaveformPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    //--
    // Two-row toolbar
    //
    //  Row 1 (primary - always fully visible):
    //    [Load VCD] | [Zoom-][Fit][Zoom+] | [|<][<<][>>][>|] | [CSV][PNG]
    //    ... stretch ... | T=cursor | hover
    //
    //  Row 2 (secondary):
    //    [Save Session][Load Session] | file label (stretch)
    //    | Filter:[field] | Find:[sig][val][<][>] | Unit:[choice]
    //--
    wxPanel* toolbar = new wxPanel(this, wxID_ANY);
    toolbar->SetBackgroundColour(OCXTheme::BgPanel());
    toolbar->SetMinSize(wxSize(-1, 66));   // two rows of 26px + 7px padding each

    // Shared button factory (parent passed explicitly so we can use it in both rows)
    auto MkBtn = [&](wxWindow* par, int id, const wxString& lbl, int w) -> wxButton* {
        wxButton* b = new wxButton(par, id, lbl,
                                   wxDefaultPosition, wxSize(w, 26));
        b->SetBackgroundColour(OCXTheme::BgSash());
        b->SetForegroundColour(OCXTheme::FgText());
        return b;
    };
    // Thin vertical separator (1px × 22px coloured panel)
    auto Sep = [&](wxWindow* par) -> wxPanel* {
        wxPanel* p = new wxPanel(par, wxID_ANY,
                                 wxDefaultPosition, wxSize(1, 22));
        p->SetBackgroundColour(OCXTheme::BgSash());
        return p;
    };

    //** Row 1 **//
    wxPanel* row1 = new wxPanel(toolbar, wxID_ANY);
    row1->SetBackgroundColour(OCXTheme::BgPanel());

    // File
    wxButton* btnLoad = MkBtn(row1, ID_WF_LoadVCD, "Load VCD", 80);

    // Zoom - three prominent buttons, always leftmost after Load VCD
    wxButton* btnZoomOut = MkBtn(row1, ID_WF_ZoomOut, "Zoom -", 60);
    wxButton* btnFit     = MkBtn(row1, ID_WF_Fit,     "Fit",    46);
    wxButton* btnZoomIn  = MkBtn(row1, ID_WF_ZoomIn,  "Zoom +", 60);

    // Navigation
    wxButton* btnPrevT = MkBtn(row1, ID_WF_PrevTrans, "|< Prev",  64);
    wxButton* btnStepB = MkBtn(row1, ID_WF_StepBack,  "<< Step",  64);
    wxButton* btnStepF = MkBtn(row1, ID_WF_StepFwd,   "Step >>",  64);
    wxButton* btnNextT = MkBtn(row1, ID_WF_NextTrans, "Next >|",  64);

    // Export
    wxButton* btnCSV = MkBtn(row1, ID_WF_ExportCSV, "CSV", 44);
    wxButton* btnPNG = MkBtn(row1, ID_WF_ExportPNG, "PNG", 44);

    // Signal table toggle
    wxButton* btnTable = MkBtn(row1, ID_WF_ToggleTable, "Table", 56);
    m_tableBtnRef = btnTable;

    // Cursor time label (bold, accent)
    m_timeLabel = new wxStaticText(row1, wxID_ANY, "T = --");
    {
        wxFont tf = m_timeLabel->GetFont();
        tf.SetWeight(wxFONTWEIGHT_BOLD);
        m_timeLabel->SetFont(tf);
    }
    m_timeLabel->SetForegroundColour(OCXTheme::Accent());
    m_timeLabel->SetMinSize(wxSize(170, -1));

    // Hover label (signal/value under mouse)
    wxStaticText* hoverLbl = new wxStaticText(row1, wxID_ANY, "");
    hoverLbl->SetForegroundColour(OCXTheme::FgDim());
    hoverLbl->SetMinSize(wxSize(200, -1));

    wxBoxSizer* row1Sz = new wxBoxSizer(wxHORIZONTAL);
    row1Sz->AddSpacer(6);
    // File
    row1Sz->Add(btnLoad,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row1Sz->Add(Sep(row1),  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // Zoom
    row1Sz->Add(btnZoomOut, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row1Sz->Add(btnFit,     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row1Sz->Add(btnZoomIn,  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row1Sz->Add(Sep(row1),  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // Navigation
    row1Sz->Add(btnPrevT,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row1Sz->Add(btnStepB,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row1Sz->Add(btnStepF,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row1Sz->Add(btnNextT,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row1Sz->Add(Sep(row1),  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // Export
    row1Sz->Add(btnCSV,     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row1Sz->Add(btnPNG,     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row1Sz->Add(btnTable,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // Stretch - pushes time label + hover to the far right
    row1Sz->AddStretchSpacer(1);
    row1Sz->Add(m_timeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    row1Sz->Add(hoverLbl,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row1->SetSizer(row1Sz);

    //** Row 2 **//
    wxPanel* row2 = new wxPanel(toolbar, wxID_ANY);
    row2->SetBackgroundColour(OCXTheme::BgPanel());

    // Session buttons
    wxButton* btnSaveSes = MkBtn(row2, ID_WF_SaveSession, "Save Session", 90);
    wxButton* btnLoadSes = MkBtn(row2, ID_WF_LoadSession, "Load Session", 90);

    // File label (stretches to fill available space)
    m_fileLabel = new wxStaticText(row2, wxID_ANY, "No file loaded");
    m_fileLabel->SetForegroundColour(OCXTheme::FgDim());

    // Filter
    wxStaticText* filterLbl = new wxStaticText(row2, wxID_ANY, "Filter:");
    filterLbl->SetForegroundColour(OCXTheme::FgDim());
    m_filterField = new wxTextCtrl(row2, ID_WF_Filter, "",
                                   wxDefaultPosition, wxSize(100, 22),
                                   wxBORDER_SIMPLE);
    m_filterField->SetBackgroundColour(OCXTheme::BgEditor());
    m_filterField->SetForegroundColour(OCXTheme::FgText());

    // Find
    wxStaticText* findLbl = new wxStaticText(row2, wxID_ANY, "Find:");
    findLbl->SetForegroundColour(OCXTheme::FgDim());
    m_findSigChoice = new wxChoice(row2, ID_WF_FindSig,
                                   wxDefaultPosition, wxSize(110, 24));
    m_findSigChoice->Append("-- signal --");
    m_findSigChoice->SetSelection(0);
    m_findSigChoice->SetBackgroundColour(OCXTheme::BgEditor());
    m_findSigChoice->SetForegroundColour(OCXTheme::FgText());
    m_findValField = new wxTextCtrl(row2, ID_WF_FindVal, "",
                                    wxDefaultPosition, wxSize(56, 22),
                                    wxBORDER_SIMPLE | wxTE_PROCESS_ENTER);
    m_findValField->SetBackgroundColour(OCXTheme::BgEditor());
    m_findValField->SetForegroundColour(OCXTheme::FgText());
    m_findValField->SetHint("value");
    wxButton* btnFindPrev = MkBtn(row2, ID_WF_FindPrev, "< Prev", 52);
    wxButton* btnFindNext = MkBtn(row2, ID_WF_FindNext, "Next >", 52);

    // Time unit picker
    wxStaticText* unitLbl = new wxStaticText(row2, wxID_ANY, "Unit:");
    unitLbl->SetForegroundColour(OCXTheme::FgDim());
    m_unitChoice = new wxChoice(row2, ID_WF_UnitChoice,
                                wxDefaultPosition, wxSize(58, 24));
    m_unitChoice->Append("raw");
    m_unitChoice->Append("ps");
    m_unitChoice->Append("ns");
    m_unitChoice->Append("us");
    m_unitChoice->Append("ms");
    m_unitChoice->SetSelection(0);
    m_unitChoice->SetBackgroundColour(OCXTheme::BgEditor());
    m_unitChoice->SetForegroundColour(OCXTheme::FgText());

    wxBoxSizer* row2Sz = new wxBoxSizer(wxHORIZONTAL);
    row2Sz->AddSpacer(6);
    // Session
    row2Sz->Add(btnSaveSes,  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row2Sz->Add(btnLoadSes,  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row2Sz->Add(Sep(row2),   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // File label stretches
    row2Sz->Add(m_fileLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row2Sz->Add(Sep(row2),   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // Filter
    row2Sz->Add(filterLbl,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row2Sz->Add(m_filterField, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row2Sz->Add(Sep(row2),    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // Find
    row2Sz->Add(findLbl,        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row2Sz->Add(m_findSigChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row2Sz->Add(m_findValField,  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row2Sz->Add(btnFindPrev,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row2Sz->Add(btnFindNext,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row2Sz->Add(Sep(row2),      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    // Unit
    row2Sz->Add(unitLbl,     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    row2Sz->Add(m_unitChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    row2->SetSizer(row2Sz);

    //** Stack rows vertically **//
    wxBoxSizer* tbSizer = new wxBoxSizer(wxVERTICAL);
    tbSizer->Add(row1, 0, wxEXPAND | wxTOP | wxBOTTOM, 3);
    tbSizer->Add(row2, 0, wxEXPAND | wxBOTTOM, 3);
    toolbar->SetSizer(tbSizer);

    //** Splitter: waveform on top, signal table on bottom (toggleable) **//
    m_splitter = new wxSplitterWindow(this, wxID_ANY,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE | wxBORDER_NONE);
    m_splitter->SetSashGravity(1.0);   // top pane grows when window resizes
    m_splitter->SetMinimumPaneSize(60);

    // Top pane - canvas area (names + waveform side by side)
    m_wavePane = new wxPanel(m_splitter, wxID_ANY);
    m_wavePane->SetBackgroundColour(OCXTheme::BgOutput());

    // Alias for backward compat with code below that uses `content`
    wxPanel* content = m_wavePane;

    // WfCanvas must be created before WfNamesPanel (names holds canvas ptr)
    m_canvas = new WfCanvas(content, nullptr);
    m_names  = new WfNamesPanel(content, m_canvas);
    m_canvas->SetTimeLabelCtrl(m_timeLabel);
    m_canvas->SetHoverLabelCtrl(hoverLbl);

    // Names panel: remove-track callback
    m_names->SetRemoveCallback([this](int row)
    {
        if (row < 0 || row >= (int)m_data.tracks.size()) return;
        m_data.tracks.erase(m_data.tracks.begin() + row);
        Reload();
    });

    // Names panel: add-to-watch callback (forwarded to mainwindow via m_watchCallback)
    m_names->SetWatchCallback([this](const wxString& name)
    {
        if (m_watchCallback) m_watchCallback(name);
    });

    // Names panel: drag-to-reorder callback
    // `from` and `to` are visible row indices in m_data.tracks.
    // We reorder in m_allTracks (canonical list) by signal name lookup, then
    // re-apply the filter so both lists stay consistent.
    m_names->SetReorderCallback([this](int from, int to)
    {
        if (from < 0 || to < 0 || from == to) return;
        int n = (int)m_data.tracks.size();
        if (from >= n || to >= n) return;

        // Find each visible row's position in m_allTracks by signal name
        wxString nameFrom = m_data.tracks[from].signal.name;
        wxString nameTo   = m_data.tracks[to].signal.name;

        int posFrom = -1, posTo = -1;
        for (int i = 0; i < (int)m_allTracks.size(); ++i)
        {
            if (posFrom < 0 && m_allTracks[i].signal.name == nameFrom) posFrom = i;
            if (posTo   < 0 && m_allTracks[i].signal.name == nameTo)   posTo   = i;
        }
        if (posFrom < 0 || posTo < 0) return;

        // Move the dragged element to just before the drop target
        WfTrack moved = m_allTracks[posFrom];
        m_allTracks.erase(m_allTracks.begin() + posFrom);
        int insertAt = posTo;
        if (posFrom < posTo) --insertAt;   // adjust for the removed element
        m_allTracks.insert(m_allTracks.begin() + insertAt, moved);

        ApplyFilter();
    });

    // Scroll sync is handled by WfCanvas::ScrollWindow override.
    m_canvas->Bind(wxEVT_SIZE, [this](wxSizeEvent& e){
        e.Skip();
        m_names->Refresh();
    });

    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    contentSizer->Add(m_names,  0, wxEXPAND);
    contentSizer->Add(m_canvas, 1, wxEXPAND);
    content->SetSizer(contentSizer);

    //** Signal table pane (bottom half of splitter, initially hidden) **//
    m_tablePane = new wxPanel(m_splitter, wxID_ANY);
    m_tablePane->SetBackgroundColour(OCXTheme::BgPanel());
    {
        // Header bar above the grid
        wxPanel* hdr = new wxPanel(m_tablePane, wxID_ANY);
        hdr->SetBackgroundColour(OCXTheme::BgSash());
        hdr->SetMinSize(wxSize(-1, 22));
        wxStaticText* hdrLbl = new wxStaticText(hdr, wxID_ANY,
            "  Signal Value Table - each row is a timestamp where at least one signal changes");
        hdrLbl->SetForegroundColour(OCXTheme::FgDim());
        wxBoxSizer* hdrSz = new wxBoxSizer(wxHORIZONTAL);
        hdrSz->Add(hdrLbl, 0, wxALIGN_CENTER_VERTICAL);
        hdr->SetSizer(hdrSz);

        // Grid
        m_tableGrid = new wxGrid(m_tablePane, wxID_ANY);
        m_tableGrid->CreateGrid(0, 0);
        m_tableGrid->EnableEditing(false);
        m_tableGrid->SetSelectionMode(wxGrid::wxGridSelectRows);
        m_tableGrid->DisableDragGridSize();
        m_tableGrid->SetDefaultCellFont(wxFont(8, wxFONTFAMILY_TELETYPE,
                                               wxFONTSTYLE_NORMAL,
                                               wxFONTWEIGHT_NORMAL, false, "Consolas"));
        m_tableGrid->SetDefaultCellBackgroundColour(OCXTheme::BgOutput());
        m_tableGrid->SetDefaultCellTextColour(OCXTheme::FgText());
        m_tableGrid->SetLabelBackgroundColour(OCXTheme::BgPanel());
        m_tableGrid->SetLabelTextColour(OCXTheme::FgDim());
        m_tableGrid->SetGridLineColour(OCXTheme::BgSash());
        m_tableGrid->SetDefaultRowSize(20, true);
        m_tableGrid->HideRowLabels();
        m_tableGrid->SetBackgroundColour(OCXTheme::BgOutput());

        // Clicking a row jumps the waveform cursor to that timestamp
        m_tableGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK,
            [this](wxGridEvent& e)
            {
                int row = e.GetRow();
                if (row >= 0 && row < (int)m_tableRowTimes.size())
                    m_canvas->SetCursorTimeDirect(m_tableRowTimes[row]);
                e.Skip();
            });

        wxBoxSizer* tableSz = new wxBoxSizer(wxVERTICAL);
        tableSz->Add(hdr,          0, wxEXPAND);
        tableSz->Add(m_tableGrid,  1, wxEXPAND);
        m_tablePane->SetSizer(tableSz);
    }

    // Start with only the waveform pane visible (no split)
    m_splitter->Initialize(m_wavePane);

    //** Root sizer **//
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    root->Add(toolbar,     0, wxEXPAND | wxBOTTOM, 1);
    root->Add(m_splitter,  1, wxEXPAND);
    SetSizer(root);

    // Toolbar events
    Bind(wxEVT_BUTTON, &WaveformPanel::OnLoadVCD,    this, ID_WF_LoadVCD);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnZoomIn,     this, ID_WF_ZoomIn);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnZoomOut,    this, ID_WF_ZoomOut);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnFit,        this, ID_WF_Fit);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnStepBack,   this, ID_WF_StepBack);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnStepFwd,    this, ID_WF_StepFwd);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnPrevTrans,  this, ID_WF_PrevTrans);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnNextTrans,  this, ID_WF_NextTrans);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnExportCSV,    this, ID_WF_ExportCSV);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnExportPNG,    this, ID_WF_ExportPNG);
    Bind(wxEVT_TEXT,   &WaveformPanel::OnFilterChanged, this, ID_WF_Filter);
    Bind(wxEVT_CHOICE,  &WaveformPanel::OnUnitChanged,   this, ID_WF_UnitChoice);
    Bind(wxEVT_BUTTON,  &WaveformPanel::OnSaveSession,  this, ID_WF_SaveSession);
    Bind(wxEVT_BUTTON,  &WaveformPanel::OnLoadSession,  this, ID_WF_LoadSession);
    Bind(wxEVT_BUTTON,  &WaveformPanel::OnFindNext,     this, ID_WF_FindNext);
    Bind(wxEVT_BUTTON,  &WaveformPanel::OnFindPrev,     this, ID_WF_FindPrev);
    Bind(wxEVT_TEXT_ENTER, &WaveformPanel::OnFindNext,  this, ID_WF_FindVal);
    Bind(wxEVT_BUTTON, &WaveformPanel::OnToggleTable,   this, ID_WF_ToggleTable);

    // Internal cursor callback - forwards to external and syncs table highlight
    m_canvas->SetCursorCallback([this](long long t)
    {
        if (m_cursorCallback) m_cursorCallback(t);
        HighlightTableRow(t);
    });
}

void WaveformPanel::LoadVCD(const wxString& filePath)
{
    // Show a loading indicator before parsing - large VCDs can take several seconds
    m_fileLabel->SetLabel("Loading " + wxFileName(filePath).GetFullName() + "...");
    m_fileLabel->Update();
    wxYield();

    VcdParser parser;
    if (!parser.Parse(filePath))
    {
        m_fileLabel->SetLabel("Error reading: " + wxFileName(filePath).GetFullName());
        return;
    }

    m_currentVCDPath = filePath;
    m_data = parser.GetData();
    m_allTracks = m_data.tracks;   // keep full unfiltered copy
    if (m_filterField) m_filterField->SetValue("");  // reset filter on new load

    // Auto-detect display unit from VCD $timescale so times show as ns/us/ms
    // rather than raw simulator ticks.
    {
        double psPerUnit = ParseTimescaleToPs(m_data.timescale);
        int      autoSel   = 2;       // default: ns
        double   autoDivPs = 1000.0;  // 1 ns in ps
        wxString autoUnit  = "ns";
        if      (psPerUnit >= 1.0e9) { autoSel = 4; autoDivPs = 1.0e9; autoUnit = "ms"; }
        else if (psPerUnit >= 1.0e6) { autoSel = 3; autoDivPs = 1.0e6; autoUnit = "us"; }
        double autoDivisor = autoDivPs / psPerUnit; // raw units per 1 display unit
        if (m_unitChoice) m_unitChoice->SetSelection(autoSel);
        m_canvas->SetDisplayUnit(autoDivisor, autoUnit);

        long long dispEnd = (autoDivisor > 0.0)
            ? (long long)((double)m_data.endTime / autoDivisor + 0.5)
            : m_data.endTime;
        m_fileLabel->SetLabel(wxFileName(filePath).GetFullName()
            + wxString::Format("  |  %d signals  |  end: %lld %s",
                               (int)m_data.tracks.size(), dispEnd, autoUnit));
    }

    // Repopulate signal picker for Find Value
    if (m_findSigChoice)
    {
        m_findSigChoice->Clear();
        m_findSigChoice->Append("-- signal --");
        for (const auto& t : m_allTracks)
            m_findSigChoice->Append(t.signal.name);
        m_findSigChoice->SetSelection(0);
    }

    Reload();
}

void WaveformPanel::Reload()
{
    m_names->SetTracks(&m_data.tracks);
    m_canvas->SetData(&m_data.tracks, m_data.endTime, m_data.timescale);
    m_names->Refresh();
    RefreshTable();
    // Defer FitToWindow until after the sizer layout pass has settled so that
    // GetClientSize() returns the real canvas width, not a stale/zero value.
    CallAfter([this]()
    {
        m_canvas->FitToWindow();
        m_names->Refresh();
    });
}

//--
// Signal value table
//--
void WaveformPanel::RefreshTable()
{
    if (!m_tableGrid) return;

    const auto& tracks = m_data.tracks;
    int numSig = (int)tracks.size();

    // Collect all unique timestamps across all tracks
    std::set<long long> timeSet;
    for (const auto& tr : tracks)
        for (const auto& c : tr.changes)
            timeSet.insert(c.time);

    m_tableRowTimes.assign(timeSet.begin(), timeSet.end()); // already sorted
    int numRows = (int)m_tableRowTimes.size();
    int numCols = 1 + numSig; // Time + one per signal

    m_tableGrid->BeginBatch();

    // Resize grid
    if (m_tableGrid->GetNumberRows() > 0)
        m_tableGrid->DeleteRows(0, m_tableGrid->GetNumberRows());
    if (m_tableGrid->GetNumberCols() > 0)
        m_tableGrid->DeleteCols(0, m_tableGrid->GetNumberCols());

    if (numRows == 0 || numSig == 0)
    {
        m_tableGrid->EndBatch();
        return;
    }

    m_tableGrid->AppendCols(numCols);
    m_tableGrid->AppendRows(numRows);

    // Column headers
    wxString timeUnit = m_canvas->GetDisplayUnit();
    wxString timeHeader = timeUnit.IsEmpty()
        ? "Time (" + m_data.timescale + ")"
        : "Time (" + timeUnit + ")";
    m_tableGrid->SetColLabelValue(0, timeHeader);
    m_tableGrid->SetColSize(0, 90);

    for (int s = 0; s < numSig; ++s)
    {
        wxString hdr = tracks[s].signal.name;
        if (tracks[s].signal.width > 1)
            hdr += wxString::Format("[%d:0]", tracks[s].signal.width - 1);
        m_tableGrid->SetColLabelValue(s + 1, hdr);
        m_tableGrid->SetColSize(s + 1, 55);
    }

    // Populate rows
    double divisor = m_canvas->GetDisplayDivisor();
    bool   useUnit = !timeUnit.IsEmpty();

    for (int r = 0; r < numRows; ++r)
    {
        long long t = m_tableRowTimes[r];

        // Alternating row background
        wxColour rowBg = (r % 2 == 0) ? OCXTheme::BgOutput() : OCXTheme::BgLineCur();

        // Time cell
        wxString timeStr;
        if (useUnit)
        {
            double v = (double)t / divisor;
            timeStr = (v == (long long)v)
                ? wxString::Format("%lld", (long long)v)
                : wxString::Format("%.3f", v);
        }
        else
        {
            timeStr = wxString::Format("%lld", t);
        }
        m_tableGrid->SetCellValue(r, 0, "  " + timeStr);
        m_tableGrid->SetCellAlignment(r, 0, wxALIGN_LEFT, wxALIGN_CENTER);
        m_tableGrid->SetCellBackgroundColour(r, 0, rowBg);
        m_tableGrid->SetCellTextColour(r, 0, OCXTheme::FgDim());

        // Signal value cells
        for (int s = 0; s < numSig; ++s)
        {
            wxString val = tracks[s].ValueAt(t);
            wxString display;
            if (tracks[s].signal.width == 1)
            {
                display = val; // "0", "1", "x"
            }
            else
            {
                display = (val.StartsWith("b") || val.StartsWith("B"))
                    ? BinToHex(val, tracks[s].signal.width)
                    : val;
            }

            m_tableGrid->SetCellValue(r, s + 1, display);
            m_tableGrid->SetCellAlignment(r, s + 1, wxALIGN_CENTER, wxALIGN_CENTER);
            m_tableGrid->SetCellBackgroundColour(r, s + 1, rowBg);

            // Color: green=1/high, red=0/low, dim=x/z
            wxColour fg;
            if (display == "1" || display == "h" || display == "H")
                fg = wxColour(80, 200, 80);
            else if (display == "0")
                fg = wxColour(200, 80, 80);
            else
                fg = OCXTheme::FgDim();
            m_tableGrid->SetCellTextColour(r, s + 1, fg);
        }
    }

    m_tableGrid->EndBatch();
    m_tableGrid->Refresh();
}

void WaveformPanel::HighlightTableRow(long long cursorTime)
{
    if (!m_tableGrid || m_tableRowTimes.empty()) return;
    if (!m_splitter->IsSplit()) return;

    // Find the last row whose timestamp <= cursorTime
    int target = 0;
    for (int r = 0; r < (int)m_tableRowTimes.size(); ++r)
    {
        if (m_tableRowTimes[r] <= cursorTime)
            target = r;
        else
            break;
    }

    m_tableGrid->SelectRow(target, false);
    m_tableGrid->MakeCellVisible(target, 0);
}

void WaveformPanel::OnToggleTable(wxCommandEvent&)
{
    if (m_splitter->IsSplit())
    {
        m_splitter->Unsplit(m_tablePane);
        if (m_tableBtnRef) m_tableBtnRef->SetLabel("Table");
    }
    else
    {
        m_splitter->SplitHorizontally(m_wavePane, m_tablePane, -180);
        if (m_tableBtnRef) m_tableBtnRef->SetLabel("Table");
        HighlightTableRow(m_canvas->GetCursorTime());
    }
}

void WaveformPanel::OnLoadVCD(wxCommandEvent&)
{
    wxFileDialog dlg(this, "Open VCD Waveform File", "", "",
                     "VCD files (*.vcd)|*.vcd|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK)
        return;
    LoadVCD(dlg.GetPath());
}

void WaveformPanel::OnZoomIn(wxCommandEvent&)
{
    m_canvas->ZoomIn();
    m_names->Refresh();
}

void WaveformPanel::OnZoomOut(wxCommandEvent&)
{
    m_canvas->ZoomOut();
    m_names->Refresh();
}

void WaveformPanel::OnFit(wxCommandEvent&)
{
    m_canvas->FitToWindow();
    m_names->Refresh();
}

void WaveformPanel::OnStepBack(wxCommandEvent&)
{
    // Step by 1% of total simulation time (minimum 1 unit)
    long long delta = wxMax(1LL, m_canvas->GetEndTime() / 100);
    m_canvas->StepCursor(-delta);
    m_names->Refresh();
}

void WaveformPanel::OnStepFwd(wxCommandEvent&)
{
    long long delta = wxMax(1LL, m_canvas->GetEndTime() / 100);
    m_canvas->StepCursor(delta);
    m_names->Refresh();
}

void WaveformPanel::OnPrevTrans(wxCommandEvent&)
{
    m_canvas->StepToPrevTransition();
    m_names->Refresh();
}

void WaveformPanel::OnNextTrans(wxCommandEvent&)
{
    m_canvas->StepToNextTransition();
    m_names->Refresh();
}

void WaveformPanel::OnExportCSV(wxCommandEvent&)
{
    if (m_data.tracks.empty())
    {
        wxMessageBox("No waveform data loaded.", "Export CSV", wxOK | wxICON_INFORMATION);
        return;
    }

    wxFileDialog dlg(this, "Export Waveform as CSV", "", "waveform.csv",
                     "CSV files (*.csv)|*.csv|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    // Collect every unique timestamp across all tracks (always include t=0)
    std::set<long long> times;
    times.insert(0);
    for (const WfTrack& t : m_data.tracks)
        for (const WfChange& c : t.changes)
            times.insert(c.time);

    wxFile f(dlg.GetPath(), wxFile::write);
    if (!f.IsOpened())
    {
        wxMessageBox("Could not open file for writing.", "Export CSV", wxOK | wxICON_ERROR);
        return;
    }

    // Header row
    wxString header = "Time";
    for (const WfTrack& t : m_data.tracks)
        header += "," + t.signal.name;
    header += "\n";
    f.Write(header);

    // One row per timestamp
    for (long long ts : times)
    {
        wxString row = wxString::Format("%lld", ts);
        for (const WfTrack& t : m_data.tracks)
            row += "," + t.ValueAt(ts);
        row += "\n";
        f.Write(row);
    }
}

void WaveformPanel::SetCursorCallback(std::function<void(long long)> cb)
{
    // Store externally; the canvas callback (set in constructor) calls both
    // this external callback and HighlightTableRow.
    m_cursorCallback = cb;
}

void WaveformPanel::SetWatchCallback(std::function<void(const wxString&)> cb)
{
    m_watchCallback = cb;
    m_names->SetWatchCallback([this](const wxString& name){
        if (m_watchCallback) m_watchCallback(name);
    });
}

const std::vector<WfTrack>* WaveformPanel::GetTracks() const
{
    return m_data.tracks.empty() ? nullptr : &m_data.tracks;
}

void WaveformPanel::OnExportPNG(wxCommandEvent&)
{
    if (m_data.tracks.empty())
    {
        wxMessageBox("No waveform data loaded.", "Export PNG", wxOK | wxICON_INFORMATION);
        return;
    }

    wxFileDialog dlg(this, "Export Waveform as PNG", "", "waveform.png",
                     "PNG files (*.png)|*.png|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    if (!m_canvas->ExportPNG(dlg.GetPath(), m_names))
        wxMessageBox("Could not save PNG file.", "Export PNG", wxOK | wxICON_ERROR);
}

void WaveformPanel::OnFilterChanged(wxCommandEvent&)
{
    ApplyFilter();
}

void WaveformPanel::OnUnitChanged(wxCommandEvent&)
{
    // ps per one unit of each display choice
    static const double   psPerDisplay[] = { 0.0, 1.0, 1000.0, 1.0e6, 1.0e9 };
    static const wxString unitLabels[]   = { "",  "ps", "ns",   "us",  "ms"  };

    int sel = m_unitChoice->GetSelection();
    if (sel < 0 || sel >= 5) return;

    if (sel == 0)   // raw - no conversion
    {
        m_canvas->SetDisplayUnit(1.0, "");
        RefreshTable();
        return;
    }

    // Compute divisor from the actual VCD timescale (raw units per 1 display unit)
    double psPerUnit = ParseTimescaleToPs(m_data.timescale);
    double divisor   = psPerDisplay[sel] / psPerUnit;
    m_canvas->SetDisplayUnit(divisor, unitLabels[sel]);
    RefreshTable();
}

void WaveformPanel::ApplyFilter()
{
    if (m_allTracks.empty()) return;

    wxString filter = m_filterField ? m_filterField->GetValue().Lower().Trim() : "";

    if (filter.IsEmpty())
    {
        m_data.tracks = m_allTracks;
    }
    else
    {
        m_data.tracks.clear();
        for (const WfTrack& t : m_allTracks)
            if (t.signal.name.Lower().Contains(filter))
                m_data.tracks.push_back(t);
    }

    m_names->SetTracks(&m_data.tracks);
    m_canvas->SetData(&m_data.tracks, m_data.endTime, m_data.timescale);
    m_names->Refresh();
}

//--
// Waveform session save / load (.ocxwave)
//--
void WaveformPanel::SaveSession(const wxString& path)
{
    wxFile f(path, wxFile::write);
    if (!f.IsOpened()) return;

    wxString out;
    out += "[Session]\n";
    out += "VCDPath="     + m_currentVCDPath + "\n";
    out += wxString::Format("PixelsPerUnit=%.6f\n", m_canvas->GetPixelsPerUnit());
    out += wxString::Format("CursorTime=%lld\n",    m_canvas->GetCursorTime());
    out += "FilterText="  + (m_filterField ? m_filterField->GetValue() : "") + "\n";
    out += wxString::Format("UnitChoice=%d\n", m_unitChoice ? m_unitChoice->GetSelection() : 0);
    out += "\n[Markers]\n";

    const auto& markers = m_canvas->GetMarkers();
    out += wxString::Format("Count=%d\n", (int)markers.size());
    int idx = 0;
    for (const auto& kv : markers)
    {
        out += wxString::Format("Marker%dTime=%lld\n", idx, kv.first);
        out += wxString::Format("Marker%dLabel=%s\n",  idx, kv.second);
        ++idx;
    }

    f.Write(out);
}

void WaveformPanel::LoadSession(const wxString& path)
{
    wxFileConfig cfg("", "", path, "", wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH);

    wxString vcdPath;
    cfg.Read("/Session/VCDPath", &vcdPath, "");
    if (vcdPath.IsEmpty() || !wxFileExists(vcdPath))
    {
        wxMessageBox("Session VCD file not found:\n" + vcdPath,
                     "Load Session", wxOK | wxICON_WARNING);
        return;
    }

    // Load the VCD
    LoadVCD(vcdPath);
    Reload();

    // Restore zoom
    double ppu = 1.0;
    cfg.Read("/Session/PixelsPerUnit", &ppu, 1.0);
    m_canvas->SetPixelsPerUnit(ppu);

    // Restore cursor
    long long cursor = 0;
    wxString cursorStr;
    cfg.Read("/Session/CursorTime", &cursorStr, "0");
    cursorStr.ToLongLong(&cursor);
    if (cursor >= 0)
        m_canvas->SetCursorTimeDirect(cursor);

    // Restore filter
    wxString filter;
    cfg.Read("/Session/FilterText", &filter, "");
    if (m_filterField) { m_filterField->SetValue(filter); ApplyFilter(); }

    // Restore unit choice
    long unitSel = 0;
    cfg.Read("/Session/UnitChoice", &unitSel, 0L);
    if (m_unitChoice && unitSel >= 0 && unitSel < (int)m_unitChoice->GetCount())
    {
        m_unitChoice->SetSelection((int)unitSel);
        wxCommandEvent dummy;
        OnUnitChanged(dummy);
    }

    // Restore markers
    m_canvas->ClearMarkers();
    long markerCount = 0;
    cfg.Read("/Markers/Count", &markerCount, 0L);
    for (int i = 0; i < (int)markerCount; ++i)
    {
        wxString timeStr, label;
        cfg.Read(wxString::Format("/Markers/Marker%dTime", i), &timeStr, "");
        cfg.Read(wxString::Format("/Markers/Marker%dLabel", i), &label, "");
        long long t = 0;
        timeStr.ToLongLong(&t);
        m_canvas->AddMarker(t, label);
    }

    m_names->Refresh();
    m_canvas->Refresh();
}

void WaveformPanel::OnSaveSession(wxCommandEvent&)
{
    if (m_currentVCDPath.IsEmpty())
    {
        wxMessageBox("No waveform loaded - nothing to save.",
                     "Save Session", wxOK | wxICON_INFORMATION);
        return;
    }

    wxFileDialog dlg(this, "Save Waveform Session", "", "session.ocxwave",
                     "OpenCircuitX Waveform Session (*.ocxwave)|*.ocxwave|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    SaveSession(dlg.GetPath());
}

void WaveformPanel::OnLoadSession(wxCommandEvent&)
{
    wxFileDialog dlg(this, "Load Waveform Session", "", "",
                     "OpenCircuitX Waveform Session (*.ocxwave)|*.ocxwave|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    LoadSession(dlg.GetPath());
}

//--
// Find Value - jump cursor to next/prev occurrence of a signal value
//--
void WaveformPanel::OnFindNext(wxCommandEvent&)
{
    if (!m_findSigChoice || !m_findValField) return;
    int sel = m_findSigChoice->GetSelection();
    if (sel <= 0) return;                       // 0 = "-- signal --"
    int trackIdx = sel - 1;                     // offset by placeholder

    // Search in m_allTracks (unfiltered) but by index in the visible track order
    // The choice is populated from m_allTracks, so index matches.
    if (trackIdx >= (int)m_allTracks.size()) return;

    wxString value = m_findValField->GetValue().Trim(true).Trim(false);
    if (value.IsEmpty()) return;

    long long cursorTime = m_canvas ? m_canvas->GetCursorTime() : -1;
    long long found = m_canvas
        ? m_canvas->FindNextValue(trackIdx, value, cursorTime)
        : -1;

    if (found < 0)
    {
        // Wrap around from beginning
        found = m_canvas ? m_canvas->FindNextValue(trackIdx, value, -1) : -1;
        if (found < 0) { wxBell(); return; }
    }

    if (m_canvas)
    {
        m_canvas->SetCursorTimeDirect(found);
        // Scroll canvas to show the found time
        int x = (int)(found * m_canvas->GetPixelsPerUnit()) - 40;
        if (x < 0) x = 0;
        m_canvas->Scroll(x, -1);
    }
}

void WaveformPanel::OnFindPrev(wxCommandEvent&)
{
    if (!m_findSigChoice || !m_findValField) return;
    int sel = m_findSigChoice->GetSelection();
    if (sel <= 0) return;
    int trackIdx = sel - 1;

    if (trackIdx >= (int)m_allTracks.size()) return;

    wxString value = m_findValField->GetValue().Trim(true).Trim(false);
    if (value.IsEmpty()) return;

    long long cursorTime = m_canvas ? m_canvas->GetCursorTime() : -1;
    if (cursorTime < 0) cursorTime = m_data.endTime;

    long long found = m_canvas
        ? m_canvas->FindPrevValue(trackIdx, value, cursorTime)
        : -1;

    if (found < 0)
    {
        // Wrap around from end
        found = m_canvas ? m_canvas->FindPrevValue(trackIdx, value, m_data.endTime + 1) : -1;
        if (found < 0) { wxBell(); return; }
    }

    if (m_canvas)
    {
        m_canvas->SetCursorTimeDirect(found);
        int x = (int)(found * m_canvas->GetPixelsPerUnit()) - 40;
        if (x < 0) x = 0;
        m_canvas->Scroll(x, -1);
    }
}
