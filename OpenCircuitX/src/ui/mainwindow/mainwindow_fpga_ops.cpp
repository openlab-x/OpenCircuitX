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

// FPGA Toolchain handlers
//--
// Helper: find the BoardProfile for the currently selected board id.
static const BoardProfile* FindBoard(const wxString& id)
{
    for (const BoardProfile& b : FPGAToolchain::GetBoards())
        if (b.id == id)
            return &b;
    return &FPGAToolchain::GetBoards()[0]; // fallback to first
}

void MainWindow::OnFPGASynthesize(wxCommandEvent&)
{
    wxString filePath = logicEditor->GetCurrentFilePath();
    if (filePath.IsEmpty())
    {
        outputPanel->LogMessage("No file open. Open a VHDL/Verilog file first.");
        return;
    }

    if (!fpgaToolchain->IsYosysAvailable())
    {
        outputPanel->LogError("yosys not found. Go to Tools > Settings > FPGA tab to set the path.");
        outputPanel->ShowErrorTab();
        return;
    }

    // Ask for top entity
    wxTextEntryDialog dlg(this,
        "Enter the top-level entity/module name for synthesis:",
        "Synthesize with Yosys",
        currentProject.simTopEntity);
    if (dlg.ShowModal() != wxID_OK) return;
    wxString topEntity = dlg.GetValue().Trim(true).Trim(false);
    if (topEntity.IsEmpty()) return;

    logicEditor->SaveCurrentFile();
    fpgaToolchain->SetWorkDir(currentProjectDirectory);

    // Pass project source files so GHDL can analyze all VHDL dependencies.
    // If the project has no listed files, fall back to the single open file.
    if (!currentProject.sourceFiles.IsEmpty())
        fpgaToolchain->SetProjectFiles(currentProject.sourceFiles);
    else
        fpgaToolchain->SetProjectFiles(wxArrayString());

    const BoardProfile* board = FindBoard(m_fpgaBoardId);

    wxString outDir = currentProjectDirectory.IsEmpty()
                    ? wxFileName(filePath).GetPath()
                    : currentProjectDirectory;

    outputPanel->ClearOutput();
    outputPanel->ClearErrors();
    outputPanel->ShowOutputTab();

    wxArrayString output, errors;
    SynthReport report;
    bool ok = fpgaToolchain->Synthesize(filePath, topEntity, outDir,
                                         *board, output, errors, report);

    for (size_t i = 0; i < output.GetCount(); ++i)
        outputPanel->LogMessage(output[i]);
    for (size_t i = 0; i < errors.GetCount(); ++i)
        outputPanel->LogError(errors[i]);

    if (ok)
    {
        outputPanel->LogMessage(FPGAToolchain::FormatReport(report, *board));
        OCXStatus("Synthesis complete: " + topEntity);
    }
    else
    {
        outputPanel->ShowErrorTab();
        OCXStatus("Synthesis failed.");
    }
}

void MainWindow::OnFPGAPlaceRoute(wxCommandEvent&)
{
    if (!fpgaToolchain->IsNextpnrAvailable())
    {
        outputPanel->LogError("nextpnr-ice40 not found. Go to Tools > Settings > FPGA tab.");
        outputPanel->ShowErrorTab();
        return;
    }

    wxString filePath = logicEditor->GetCurrentFilePath();
    wxString stem = wxFileName(filePath).GetName();
    wxString outDir = currentProjectDirectory.IsEmpty()
                    ? wxFileName(filePath).GetPath()
                    : currentProjectDirectory;
    wxString jsonFile = outDir + "/" + stem + ".json";

    if (!wxFileExists(jsonFile))
    {
        outputPanel->LogError("Netlist not found: " + jsonFile);
        outputPanel->LogError("Run Tools > FPGA > Synthesize first.");
        outputPanel->ShowErrorTab();
        return;
    }

    const BoardProfile* board = FindBoard(m_fpgaBoardId);

    // Constraint file dialog - filter adapts to the board family
    bool isECP5 = (board->family == "ecp5");
    wxString constraintTitle  = isECP5
        ? "Select LPF constraints file (optional - press Cancel to skip)"
        : "Select PCF constraints file (optional - press Cancel to skip)";
    wxString constraintFilter = isECP5
        ? "LPF files (*.lpf)|*.lpf|All files (*.*)|*.*"
        : "PCF files (*.pcf)|*.pcf|All files (*.*)|*.*";

    wxFileDialog pcfDlg(this, constraintTitle, outDir, "",
                        constraintFilter, wxFD_OPEN);
    wxString pcfFile;
    if (pcfDlg.ShowModal() == wxID_OK)
        pcfFile = pcfDlg.GetPath();
    fpgaToolchain->SetWorkDir(currentProjectDirectory);

    outputPanel->ClearOutput();
    outputPanel->ClearErrors();
    outputPanel->ShowOutputTab();

    wxArrayString output, errors;
    bool ok = fpgaToolchain->PlaceAndRoute(jsonFile, pcfFile, *board, outDir,
                                            output, errors);

    for (size_t i = 0; i < output.GetCount(); ++i)
        outputPanel->LogMessage(output[i]);
    for (size_t i = 0; i < errors.GetCount(); ++i)
        outputPanel->LogError(errors[i]);

    if (ok)
    {
        // Also run icepack to produce .bin
        wxString ascFile = outDir + "/" + stem + ".asc";
        wxString binFile = outDir + "/" + stem + ".bin";
        wxArrayString po, pe;
        if (fpgaToolchain->Pack(ascFile, binFile, po, pe))
        {
            for (size_t i = 0; i < po.GetCount(); ++i)
                outputPanel->LogMessage(po[i]);
            outputPanel->LogMessage("Bitstream ready: " + binFile);
            OCXStatus("P&R + Pack complete.");
        }
        else
        {
            for (size_t i = 0; i < pe.GetCount(); ++i)
                outputPanel->LogError(pe[i]);
            OCXStatus("Place & Route OK, icepack failed.");
        }
    }
    else
    {
        outputPanel->ShowErrorTab();
        OCXStatus("Place & route failed.");
    }
}

void MainWindow::OnFPGAProgram(wxCommandEvent&)
{
    if (!fpgaToolchain->IsOpenFPGALoaderAvailable())
    {
        outputPanel->LogError("openFPGALoader not found. Go to Tools > Settings > FPGA tab.");
        outputPanel->ShowErrorTab();
        return;
    }

    wxString filePath = logicEditor->GetCurrentFilePath();
    wxString outDir = currentProjectDirectory.IsEmpty()
                    ? wxFileName(filePath).GetPath()
                    : currentProjectDirectory;

    // Ask user to pick the bitstream file
    wxFileDialog binDlg(this, "Select bitstream file to program",
                         outDir, "",
                         "Bitstream files (*.bin;*.bit)|*.bin;*.bit|All files (*.*)|*.*",
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (binDlg.ShowModal() != wxID_OK) return;
    wxString binFile = binDlg.GetPath();

    const BoardProfile* board = FindBoard(m_fpgaBoardId);
    fpgaToolchain->SetWorkDir(currentProjectDirectory);

    outputPanel->ClearOutput();
    outputPanel->ClearErrors();
    outputPanel->ShowOutputTab();

    wxArrayString output, errors;
    bool ok = fpgaToolchain->Program(binFile, *board, output, errors);

    for (size_t i = 0; i < output.GetCount(); ++i)
        outputPanel->LogMessage(output[i]);
    for (size_t i = 0; i < errors.GetCount(); ++i)
        outputPanel->LogError(errors[i]);

    if (ok)
        OCXStatus("Board programmed: " + board->name);
    else
    {
        outputPanel->ShowErrorTab();
        OCXStatus("Programming failed.");
    }
}

//--