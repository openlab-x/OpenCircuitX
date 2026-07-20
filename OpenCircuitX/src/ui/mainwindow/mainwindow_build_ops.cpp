#include "mainwindow.h"
#ifdef __WXMSW__
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif
#include "ui/dialogs/new_project_dialog.h"
#include "ui/dialogs/open_project_dialog.h"
#include "ui/editor/logic_editor_panel.h"
#include "ui/panels/output_panel.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/dialogs/about_dialog.h"
#include "ui/dialogs/update_checker.h"
#include "ui/canvas/circuit_canvas.h"
#include "ui/panels/component_palette.h"
#include "ui/panels/canvas_action_panel.h"
#include "ui/waveform/waveform_panel.h"
#include "ui/panels/watch_panel.h"
#include "ui/panels/welcome_panel.h"
#include "ui/panels/outline_panel.h"
#include "ui/dialogs/symbol_search_dialog.h"
#include "ui/dialogs/quick_open_dialog.h"
#include "ui/panels/find_results_panel.h"
#include "core/simulation/circuit_simulator.h"
#include "core/toolchain/fpga_toolchain.h"
#include "core/plugin/plugin_manager.h"
#include "utils/project_file.h"
#include "ui/shell/app_theme.h"
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/textfile.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/artprov.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/imaglist.h>
#include <wx/tokenzr.h>
#include <wx/numdlg.h>
#include <wx/dcbuffer.h>
#include <wx/renderer.h>
#include <wx/file.h>
#include <vector>

// Compile / Run / Debug
//--
void MainWindow::OnCompileHDL(wxCommandEvent& event)
{
    wxString filePath = logicEditor->GetCurrentFilePath();
    if (filePath.IsEmpty())
    {
        outputPanel->LogMessage("No file is open. Open a VHDL file first.");
        return;
    }

    wxString ext = wxFileName(filePath).GetExt().Lower();
    bool isVerilog = (ext == "v" || ext == "sv");

    if (isVerilog)
    {
        if (!circuitSimulator->IsIcarusAvailable())
        {
            outputPanel->LogError("iverilog not found. Go to Tools > Settings to set the Icarus Verilog path.");
            outputPanel->ShowErrorTab();
            return;
        }
    }
    else
    {
        if (!circuitSimulator->IsGHDLAvailable())
        {
            outputPanel->LogError("GHDL not found. Go to Tools > Settings to set the GHDL path.");
            outputPanel->ShowErrorTab();
            return;
        }
    }

    logicEditor->SaveCurrentFile();
    circuitSimulator->SetWorkDir(currentProjectDirectory);

    outputPanel->ClearOutput();
    outputPanel->ClearErrors();
    outputPanel->ShowOutputTab();

    wxProgressDialog busyDlg("Compiling", "Analyzing HDL files...",
                              100, this, wxPD_APP_MODAL | wxPD_SMOOTH);
    busyDlg.Pulse();
    OCXStatus("Compiling... please wait");
    wxYield();

    wxArrayString output, errors;
    bool success;

    if (isVerilog)
    {
        // Collect all .v/.sv files in the project; fall back to the open file
        wxArrayString vFiles = CollectProjectVerilogFiles();
        if (vFiles.IsEmpty())
            vFiles.Add(filePath);
        circuitSimulator->SetProjectFiles(vFiles);
        outputPanel->LogMessage(wxString::Format(
            "Compiling %d Verilog file(s)...", (int)vFiles.GetCount()));
        success = circuitSimulator->CompileVerilog(filePath, output, errors);
    }
    else
    {
        // Analyze all VHDL files in the project together (design files first)
        wxArrayString vhdlFiles = CollectProjectVHDLFiles();
        if (vhdlFiles.IsEmpty())
            vhdlFiles.Add(filePath); // no project open - just the current file
        circuitSimulator->SetProjectFiles(vhdlFiles);
        outputPanel->LogMessage(wxString::Format(
            "Analyzing %d VHDL file(s)...", (int)vhdlFiles.GetCount()));
        success = circuitSimulator->CompileHDL(filePath, output, errors);
    }

    for (size_t i = 0; i < output.GetCount(); ++i)
        outputPanel->LogMessage(output[i]);
    for (size_t i = 0; i < errors.GetCount(); ++i)
        outputPanel->LogError(errors[i]);

    if (!success)
    {
        outputPanel->ShowErrorTab();
        OCXStatus("Build failed.");

        // Apply inline squiggle decorations for each error location
        auto parsed = outputPanel->GetParsedErrors();
        std::vector<LogicEditorPanel::ErrorMark> marks;
        marks.reserve(parsed.size());
        for (const auto& pe : parsed)
        {
            LogicEditorPanel::ErrorMark m;
            m.filePath = pe.file;
            m.line     = pe.line;
            marks.push_back(m);
        }
        logicEditor->SetErrorDecorations(marks);
    }
    else
    {
        logicEditor->ClearErrorDecorations();
        OCXStatus("Build succeeded.");
        SaveProjectState();
    }
}

void MainWindow::OnRunSimulation(wxCommandEvent& event)
{
    wxString filePath = logicEditor->GetCurrentFilePath();
    if (filePath.IsEmpty())
    {
        outputPanel->LogMessage("No file is open. Open a VHDL file first.");
        return;
    }

    wxString ext2 = wxFileName(filePath).GetExt().Lower();
    bool isVerilog2 = (ext2 == "v" || ext2 == "sv");

    if (isVerilog2)
    {
        // Icarus Verilog path - no top entity needed
        if (!circuitSimulator->IsIcarusAvailable())
        {
            outputPanel->LogError("iverilog not found. Go to Tools > Settings to set the Icarus Verilog path.");
            outputPanel->ShowErrorTab();
            return;
        }

        logicEditor->SaveCurrentFile();
        circuitSimulator->SetWorkDir(currentProjectDirectory);

        outputPanel->ClearOutput();
        outputPanel->ClearErrors();
        outputPanel->ShowOutputTab();
        outputPanel->LogMessage("Running Verilog simulation: " + filePath);

        wxProgressDialog busyDlg("Simulating",
                                  "Step 1/2  Compiling Verilog files...",
                                  100, this, wxPD_APP_MODAL | wxPD_SMOOTH);
        busyDlg.Update(0, "Step 1/2  Compiling Verilog files...");
        OCXStatus("Simulating... please wait");
        wxYield();

        wxArrayString output, errors;
        wxString stem   = wxFileName(filePath).GetName();
        wxString vvpPath = (!currentProjectDirectory.IsEmpty()
                             ? currentProjectDirectory + "/"
                             : "") + stem + ".vvp";

        bool ok = circuitSimulator->CompileVerilog(filePath, output, errors);
        if (ok)
        {
            busyDlg.Update(50, "Step 2/2  Running simulation...");
            ok = circuitSimulator->RunVVP(vvpPath, output, errors);
        }
        busyDlg.Update(100, "Done.");

        for (size_t i = 0; i < output.GetCount(); ++i) outputPanel->LogMessage(output[i]);
        for (size_t i = 0; i < errors.GetCount(); ++i) outputPanel->LogError(errors[i]);
        if (!ok) { outputPanel->ShowErrorTab(); OCXStatus("Simulation failed."); }
        else        OCXStatus("Verilog simulation complete.");
        return;
    }

    if (!circuitSimulator->IsGHDLAvailable())
    {
        outputPanel->LogError("GHDL not found. Go to Tools > Settings to set the GHDL path.");
        outputPanel->ShowErrorTab();
        return;
    }

    // Read entity from the Run Config Bar (always visible - no dialog needed)
    if (m_entityField)
    {
        wxString entered = m_entityField->GetValue().Trim(true).Trim(false);
        if (!entered.IsEmpty())
            currentProject.simTopEntity = entered;
    }
    if (m_stopTimeField)
    {
        wxString st = m_stopTimeField->GetValue().Trim(true).Trim(false);
        currentProject.runTime = st;
        circuitSimulator->SetRunTime(st);
    }
    if (m_vhdlStdChoice)
    {
        const wxString stds[] = { "08", "93", "19" };
        int sel = m_vhdlStdChoice->GetSelection();
        if (sel >= 0 && sel < 3)
        {
            currentProject.vhdlStandard = stds[sel];
            circuitSimulator->SetVHDLStd(stds[sel]);
        }
    }
    if (currentProject.simTopEntity.IsEmpty())
    {
        wxTextEntryDialog dlg(this,
            "Enter the top-level entity to simulate\n"
            "(e.g. tb_counter). Set it in the Run Config Bar to skip this dialog:",
            "Run Simulation", "");
        if (dlg.ShowModal() != wxID_OK) return;
        currentProject.simTopEntity = dlg.GetValue().Trim(true).Trim(false);
        if (m_entityField) m_entityField->SetValue(currentProject.simTopEntity);
        if (currentProject.simTopEntity.IsEmpty()) return;
    }
    SaveProjectState();
    wxString topEntity = currentProject.simTopEntity;

    logicEditor->SaveCurrentFile();
    circuitSimulator->SetWorkDir(currentProjectDirectory);

    // Supply all project VHDL files so CompileHDL analyzes them together
    wxArrayString vhdlFiles = CollectProjectVHDLFiles();
    if (vhdlFiles.IsEmpty())
        vhdlFiles.Add(filePath);
    circuitSimulator->SetProjectFiles(vhdlFiles);

    outputPanel->ClearOutput();
    outputPanel->ClearErrors();
    outputPanel->ShowOutputTab();
    outputPanel->LogMessage(wxString::Format(
        "Running simulation: %s  (%d file(s))", topEntity, (int)vhdlFiles.GetCount()));
    for (size_t i = 0; i < vhdlFiles.GetCount(); ++i)
        outputPanel->LogMessage("  [" + wxString::Format("%zu", i + 1) + "] "
                                + wxFileName(vhdlFiles[i]).GetFullName());
    if (!currentProject.runTime.IsEmpty())
        outputPanel->LogMessage("Run time: " + currentProject.runTime);

    // Generate a .vcd waveform file alongside the project
    wxString vcdPath;
    if (!currentProjectDirectory.IsEmpty())
        vcdPath = currentProjectDirectory + "/" + topEntity + ".vcd";

    // Each GHDL step gets its own progress update so the bar visibly advances.
    bool success = false;
    {
        wxProgressDialog busyDlg("Simulating",
                                  "Step 1/3  Analyzing VHDL files...",
                                  100, this, wxPD_APP_MODAL | wxPD_SMOOTH);
        busyDlg.Update(0, "Step 1/3  Analyzing VHDL files...");
        OCXStatus("Simulating " + topEntity + "... please wait");
        wxYield();

        wxArrayString output, errors;

        // ghdl -a  (analyze all project files)
        success = circuitSimulator->CompileHDL(filePath, output, errors);

        if (success)
        {
            // ghdl -e  (elaborate top entity)
            busyDlg.Update(33, "Step 2/3  Elaborating " + topEntity + "...");
            success = circuitSimulator->ElaborateHDL(topEntity, output, errors);
        }

        if (success)
        {
            // ghdl -r  (run with optional VCD output)
            busyDlg.Update(66, "Step 3/3  Running simulation...");
            success = circuitSimulator->RunHDL(topEntity, vcdPath, output, errors);
        }

        for (size_t i = 0; i < output.GetCount(); ++i)
            outputPanel->LogMessage(output[i]);
        for (size_t i = 0; i < errors.GetCount(); ++i)
            outputPanel->LogError(errors[i]);

        if (success && !vcdPath.IsEmpty() && wxFileExists(vcdPath))
        {
            busyDlg.Update(90, "Loading waveform...");
            waveformPanel->LoadVCD(vcdPath);
            workspaceNotebook->SetSelection(1); // switch tab while dialog still open
            wxYield(); // let Reload()'s CallAfter(FitToWindow) fire - canvas paints
        }
        busyDlg.Update(100, "Done.");
    }   // dialog closes after waveform is already rendered

    if (!success)
    {
        outputPanel->ShowErrorTab();
        OCXStatus("Simulation failed.");
    }
    else
    {
        OCXStatus("Simulation complete: " + topEntity);

        if (!vcdPath.IsEmpty() && wxFileExists(vcdPath))
        {
            // Warn if no time-advancing transitions were captured.
            // Most common cause: simulating the design entity directly instead
            // of its testbench (e.g. 'and_gate' instead of 'tb_and_gate').
            if (waveformPanel->GetEndTime() == 0)
            {
                outputPanel->LogMessage(
                    "Warning: waveform is empty (no signal transitions recorded).");
                outputPanel->LogMessage(
                    "  -> Make sure the top entity is your testbench, not the design unit.");
                outputPanel->LogMessage(
                    "  -> Example: use 'tb_and_gate' instead of 'and_gate'.");
            }
        }
    }
}

void MainWindow::OnDebugSimulation(wxCommandEvent& event)
{
    wxArrayString output, errors;
    circuitSimulator->DebugSimulation(logicEditor->GetCurrentFilePath(), output, errors);
    for (size_t i = 0; i < errors.GetCount(); ++i)
        outputPanel->LogError(errors[i]);
    outputPanel->ShowErrorTab();
}

//--