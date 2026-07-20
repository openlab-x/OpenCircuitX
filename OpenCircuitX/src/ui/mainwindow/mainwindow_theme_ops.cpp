#include "mainwindow.h"
#include "ui/shell/app_theme.h"
#include "mdap/wxMaterialDesignArtProvider.hpp"
#include "ui/editor/logic_editor_panel.h"
#include "ui/panels/output_panel.h"
#include "ui/panels/watch_panel.h"
#include "ui/panels/outline_panel.h"
#include "ui/panels/welcome_panel.h"
#include <wx/config.h>
#include <wx/imaglist.h>

//--
// Theme switching - OCXStatus, ApplyThemeColors, OnThemeDark/Midnight/Light
//--

void MainWindow::OCXStatus(const wxString& text, int field)
{
    SetStatusText(text, field);
    if (wxStatusBar* sb = GetStatusBar()) sb->Refresh();
}

void MainWindow::ApplyThemeColors()
{
    // Frame and status bar
    SetBackgroundColour(OCXTheme::BgApp());
    if (wxStatusBar* sb = GetStatusBar())
    {
        sb->SetBackgroundColour(OCXTheme::BgPanel());
        sb->Refresh();
    }

    // Custom menu bar + toolbar
    if (m_ocxMenuBar)
    {
        m_ocxMenuBar->SetBackgroundColour(OCXTheme::BgApp());
        m_ocxMenuBar->Refresh();
    }
    if (m_ocxToolBar)
    {
        m_ocxToolBar->SetBackgroundColour(OCXTheme::BgPanel());
        wxSize bmpSize(OCXToolBar::BMP_SZ, OCXToolBar::BMP_SZ);
        wxColour clrN = OCXTheme::FgText();
        auto MI = [&](const wxArtID& id, const wxColour& clr) {
            return wxMaterialDesignArtProvider::GetBitmap(
                id, wxART_CLIENT_MATERIAL_FILLED, bmpSize, clr);
        };
        m_ocxToolBar->SetToolBitmap(wxID_NEW,           MI(wxART_NOTE_ADD,    wxColour(  0,150,220)));
        m_ocxToolBar->SetToolBitmap(ID_OpenProject,     MI(wxART_FOLDER_OPEN, wxColour(220,175, 50)));
        m_ocxToolBar->SetToolBitmap(wxID_SAVE,          MI(wxART_SAVE,        wxColour(  0,122,204)));
        m_ocxToolBar->SetToolBitmap(ID_CompileHDL,      MI(wxART_BUILD,       wxColour(200,150,  0)));
        m_ocxToolBar->SetToolBitmap(ID_RunSimulation,   MI(wxART_PLAY_ARROW,  wxColour( 60,180, 75)));
        m_ocxToolBar->SetToolBitmap(ID_DebugSimulation, MI(wxART_BUG_REPORT,  wxColour(230,115,  0)));
        m_ocxToolBar->SetToolBitmap(ID_UndoCanvas,      MI(wxART_UNDO,        clrN));
        m_ocxToolBar->SetToolBitmap(ID_RedoCanvas,      MI(wxART_REDO,        clrN));
        m_ocxToolBar->SetToolBitmap(ID_Find,            MI(wxART_SEARCH,      clrN));
        m_ocxToolBar->SetToolBitmap(ID_Settings,        MI(wxART_SETTINGS,    clrN));
    }

    // Project explorer
    projectExplorer->SetBackgroundColour(OCXTheme::BgPanel());
    projectExplorer->SetForegroundColour(OCXTheme::FgText());
    {
        wxSize iconSize(16, 16);
        wxImageList* imgList = new wxImageList(16, 16, true, 4);
        imgList->Add(wxMaterialDesignArtProvider::GetBitmap(wxART_FOLDER,          wxART_CLIENT_MATERIAL_FILLED, iconSize, wxColour(220,175, 50)));
        imgList->Add(wxMaterialDesignArtProvider::GetBitmap(wxART_CODE,            wxART_CLIENT_MATERIAL_FILLED, iconSize, wxColour( 86,156,214)));
        imgList->Add(wxMaterialDesignArtProvider::GetBitmap(wxART_DEVELOPER_BOARD, wxART_CLIENT_MATERIAL_FILLED, iconSize, wxColour( 78,201,176)));
        imgList->Add(wxMaterialDesignArtProvider::GetBitmap(wxART_ARTICLE,         wxART_CLIENT_MATERIAL_FILLED, iconSize, OCXTheme::FgText()));
        projectExplorer->AssignImageList(imgList);
    }
    projectExplorer->Refresh();

    // Workspace notebook background
    workspaceNotebook->SetBackgroundColour(OCXTheme::BgPanel());
    workspaceNotebook->Refresh();

    // Circuit canvas tab container
    if (wxWindow* canvasTab = workspaceNotebook->GetPage(1))
    {
        canvasTab->SetBackgroundColour(OCXTheme::BgPanel());
        canvasTab->Refresh();
    }

    // Output panel + inner tabs (logs, find results)
    outputPanel->ReapplyTheme();

    // Watch panel (signal debugger)
    watchPanel->ReapplyTheme();

    // Outline panel (symbol tree)
    outlinePanel->ReapplyTheme();

    // Run config bar (entity field, stop time, VHDL std choice + static labels)
    m_runConfigBar->SetBackgroundColour(OCXTheme::BgPanel());
    if (m_entityField)
    {
        m_entityField->SetBackgroundColour(OCXTheme::BgEditor());
        m_entityField->SetForegroundColour(OCXTheme::FgText());
    }
    if (m_stopTimeField)
    {
        m_stopTimeField->SetBackgroundColour(OCXTheme::BgEditor());
        m_stopTimeField->SetForegroundColour(OCXTheme::FgText());
    }
    if (m_vhdlStdChoice)
    {
        m_vhdlStdChoice->SetBackgroundColour(OCXTheme::BgPanel());
        m_vhdlStdChoice->SetForegroundColour(OCXTheme::FgText());
    }
    // Update the static text labels ("Entity:", "Stop Time:", shortcut hints).
    // wxStaticText on Windows uses system-default black text unless both its
    // background and foreground are explicitly set and it is refreshed.
    for (wxWindow* child : m_runConfigBar->GetChildren())
    {
        if (wxStaticText* lbl = wxDynamicCast(child, wxStaticText))
        {
            lbl->SetBackgroundColour(OCXTheme::BgPanel());
            lbl->SetForegroundColour(OCXTheme::FgDim());
            lbl->Refresh();
        }
    }
    m_runConfigBar->Refresh();

    // Welcome panel (shown when no project is open)
    if (m_welcomePanel)
        m_welcomePanel->ReapplyTheme();

    // Re-style the HDL editor (all open tabs, syntax colours, margins)
    logicEditor->ReapplyTheme();

    // Propagate to all children and redraw
    Refresh();
    Update();
}

void MainWindow::OnThemeDark(wxCommandEvent&)
{
    OCXTheme::Set(OCXThemeId::Dark);
    wxConfig("OpenCircuitX").Write("Theme", 0L);
    ApplyThemeColors();
}

void MainWindow::OnThemeMidnight(wxCommandEvent&)
{
    OCXTheme::Set(OCXThemeId::Midnight);
    wxConfig("OpenCircuitX").Write("Theme", 1L);
    ApplyThemeColors();
}

void MainWindow::OnThemeLight(wxCommandEvent&)
{
    OCXTheme::Set(OCXThemeId::Light);
    wxConfig("OpenCircuitX").Write("Theme", 2L);
    ApplyThemeColors();
}
