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

// Welcome panel show / hide
//--
void MainWindow::ShowWelcomePanel()
{
    if (!m_welcomePanel) return;

    wxArrayString recentPaths;
    if (m_fileHistory)
        for (size_t i = 0; i < m_fileHistory->GetCount(); ++i)
            recentPaths.Add(m_fileHistory->GetHistoryFile(i));
    m_welcomePanel->RefreshRecent(recentPaths);

    rightSplitter->ReplaceWindow(workspaceNotebook, m_welcomePanel);
    workspaceNotebook->Hide();
    m_welcomePanel->Show();

    // Collapse the output panel so the welcome screen fills the full height.
    if (rightSplitter->IsSplit())
        rightSplitter->Unsplit(outputPanel);

    rightSplitter->Layout();
}

void MainWindow::HideWelcomePanel()
{
    if (!m_welcomePanel || !m_welcomePanel->IsShown()) return;
    rightSplitter->ReplaceWindow(m_welcomePanel, workspaceNotebook);
    m_welcomePanel->Hide();
    workspaceNotebook->Show();

    // Restore the output panel at its normal 200px height.
    if (!rightSplitter->IsSplit())
    {
        outputPanel->Show();
        rightSplitter->SplitHorizontally(workspaceNotebook, outputPanel, -200);
    }

    rightSplitter->Layout();
}

//--
// Project creation
//--
//--
// Template VHDL source text - indexed to match NewProjectDialog::GetTemplateIndex()
//--
static wxString BuildTemplateSource(int tplIndex, const wxString& projectName)
{
    switch (tplIndex)
    {
    case 0: // Blank
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n\n"
            "entity %s is\n"
            "    port (\n"
            "        clk : in std_logic\n"
            "    );\n"
            "end entity %s;\n\n"
            "architecture rtl of %s is\nbegin\n\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    case 1: // 4-bit Counter
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n"
            "use ieee.numeric_std.all;\n\n"
            "entity %s is\n"
            "    port (\n"
            "        clk : in  std_logic;\n"
            "        rst : in  std_logic;\n"
            "        q   : out std_logic_vector(3 downto 0)\n"
            "    );\n"
            "end entity %s;\n\n"
            "architecture rtl of %s is\n"
            "    signal count : unsigned(3 downto 0) := (others => '0');\n"
            "begin\n"
            "    process(clk)\n"
            "    begin\n"
            "        if rising_edge(clk) then\n"
            "            if rst = '1' then\n"
            "                count <= (others => '0');\n"
            "            else\n"
            "                count <= count + 1;\n"
            "            end if;\n"
            "        end if;\n"
            "    end process;\n\n"
            "    q <= std_logic_vector(count);\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    case 2: // 4-bit ALU
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n"
            "use ieee.numeric_std.all;\n\n"
            "entity %s is\n"
            "    port (\n"
            "        a      : in  std_logic_vector(3 downto 0);\n"
            "        b      : in  std_logic_vector(3 downto 0);\n"
            "        op     : in  std_logic_vector(1 downto 0);\n"
            "        result : out std_logic_vector(3 downto 0);\n"
            "        carry  : out std_logic\n"
            "    );\n"
            "end entity %s;\n\n"
            "architecture rtl of %s is\n"
            "    signal tmp : unsigned(4 downto 0);\n"
            "begin\n"
            "    process(a, b, op)\n"
            "    begin\n"
            "        case op is\n"
            "            when \"00\" => tmp <= ('0' & unsigned(a)) + ('0' & unsigned(b));\n"
            "            when \"01\" => tmp <= ('0' & unsigned(a)) - ('0' & unsigned(b));\n"
            "            when \"10\" => tmp <= '0' & (unsigned(a) and unsigned(b));\n"
            "            when others => tmp <= '0' & (unsigned(a) or  unsigned(b));\n"
            "        end case;\n"
            "    end process;\n\n"
            "    result <= std_logic_vector(tmp(3 downto 0));\n"
            "    carry  <= tmp(4);\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    case 3: // Traffic Light FSM
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n\n"
            "entity %s is\n"
            "    port (\n"
            "        clk    : in  std_logic;\n"
            "        rst    : in  std_logic;\n"
            "        red    : out std_logic;\n"
            "        green  : out std_logic;\n"
            "        yellow : out std_logic\n"
            "    );\n"
            "end entity %s;\n\n"
            "architecture rtl of %s is\n"
            "    type t_state is (ST_RED, ST_GREEN, ST_YELLOW);\n"
            "    signal state : t_state := ST_RED;\n"
            "begin\n"
            "    process(clk)\n"
            "    begin\n"
            "        if rising_edge(clk) then\n"
            "            if rst = '1' then\n"
            "                state <= ST_RED;\n"
            "            else\n"
            "                case state is\n"
            "                    when ST_RED    => state <= ST_GREEN;\n"
            "                    when ST_GREEN  => state <= ST_YELLOW;\n"
            "                    when ST_YELLOW => state <= ST_RED;\n"
            "                end case;\n"
            "            end if;\n"
            "        end if;\n"
            "    end process;\n\n"
            "    red    <= '1' when state = ST_RED    else '0';\n"
            "    green  <= '1' when state = ST_GREEN  else '0';\n"
            "    yellow <= '1' when state = ST_YELLOW else '0';\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    case 4: // UART TX
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n"
            "use ieee.numeric_std.all;\n\n"
            "entity %s is\n"
            "    generic (\n"
            "        CLK_FREQ  : integer := 50_000_000;\n"
            "        BAUD_RATE : integer := 115_200\n"
            "    );\n"
            "    port (\n"
            "        clk      : in  std_logic;\n"
            "        tx_data  : in  std_logic_vector(7 downto 0);\n"
            "        tx_start : in  std_logic;\n"
            "        tx       : out std_logic;\n"
            "        tx_busy  : out std_logic\n"
            "    );\n"
            "end entity %s;\n\n"
            "architecture rtl of %s is\n"
            "    constant BAUD_DIV : integer := CLK_FREQ / BAUD_RATE;\n"
            "    signal   baud_cnt : integer range 0 to BAUD_DIV - 1 := 0;\n"
            "    signal   bit_idx  : integer range 0 to 9 := 0;\n"
            "    signal   shift    : std_logic_vector(9 downto 0) := (others => '1');\n"
            "    signal   busy     : std_logic := '0';\n"
            "begin\n"
            "    tx      <= shift(0);\n"
            "    tx_busy <= busy;\n\n"
            "    process(clk)\n"
            "    begin\n"
            "        if rising_edge(clk) then\n"
            "            if busy = '0' then\n"
            "                if tx_start = '1' then\n"
            "                    shift    <= '1' & tx_data & '0';\n"
            "                    baud_cnt <= 0;\n"
            "                    bit_idx  <= 0;\n"
            "                    busy     <= '1';\n"
            "                end if;\n"
            "            else\n"
            "                if baud_cnt = BAUD_DIV - 1 then\n"
            "                    baud_cnt <= 0;\n"
            "                    shift    <= '1' & shift(9 downto 1);\n"
            "                    if bit_idx = 9 then\n"
            "                        busy <= '0';\n"
            "                    else\n"
            "                        bit_idx <= bit_idx + 1;\n"
            "                    end if;\n"
            "                else\n"
            "                    baud_cnt <= baud_cnt + 1;\n"
            "                end if;\n"
            "            end if;\n"
            "        end if;\n"
            "    end process;\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    case 5: // D Flip-Flop with clock enable
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n\n"
            "entity %s is\n"
            "    port (\n"
            "        clk : in  std_logic;\n"
            "        en  : in  std_logic;\n"
            "        d   : in  std_logic;\n"
            "        q   : out std_logic\n"
            "    );\n"
            "end entity %s;\n\n"
            "architecture rtl of %s is\n"
            "    signal q_reg : std_logic := '0';\n"
            "begin\n"
            "    process(clk)\n"
            "    begin\n"
            "        if rising_edge(clk) then\n"
            "            if en = '1' then\n"
            "                q_reg <= d;\n"
            "            end if;\n"
            "        end if;\n"
            "    end process;\n\n"
            "    q <= q_reg;\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    case 6: // 7-Segment Decoder
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n\n"
            "entity %s is\n"
            "    port (\n"
            "        bcd : in  std_logic_vector(3 downto 0);\n"
            "        seg : out std_logic_vector(6 downto 0)\n"
            "    );\n"
            "end entity %s;\n\n"
            "-- seg(6..0) maps to segments: a b c d e f g  (active-low)\n"
            "architecture rtl of %s is\n"
            "begin\n"
            "    process(bcd)\n"
            "    begin\n"
            "        case bcd is\n"
            "            when \"0000\" => seg <= \"0000001\"; -- 0\n"
            "            when \"0001\" => seg <= \"1001111\"; -- 1\n"
            "            when \"0010\" => seg <= \"0010010\"; -- 2\n"
            "            when \"0011\" => seg <= \"0000110\"; -- 3\n"
            "            when \"0100\" => seg <= \"1001100\"; -- 4\n"
            "            when \"0101\" => seg <= \"0100100\"; -- 5\n"
            "            when \"0110\" => seg <= \"0100000\"; -- 6\n"
            "            when \"0111\" => seg <= \"0001111\"; -- 7\n"
            "            when \"1000\" => seg <= \"0000000\"; -- 8\n"
            "            when \"1001\" => seg <= \"0000100\"; -- 9\n"
            "            when others  => seg <= \"1111111\"; -- off\n"
            "        end case;\n"
            "    end process;\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    case 7: // Full Adder
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n\n"
            "entity %s is\n"
            "    port (\n"
            "        a    : in  std_logic;\n"
            "        b    : in  std_logic;\n"
            "        cin  : in  std_logic;\n"
            "        sum  : out std_logic;\n"
            "        cout : out std_logic\n"
            "    );\n"
            "end entity %s;\n\n"
            "architecture rtl of %s is\n"
            "begin\n"
            "    sum  <= a xor b xor cin;\n"
            "    cout <= (a and b) or (cin and (a xor b));\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    case 8: // JK Flip-Flop with synchronous reset
        return wxString::Format(
            "library ieee;\n"
            "use ieee.std_logic_1164.all;\n\n"
            "entity %s is\n"
            "    port (\n"
            "        clk : in  std_logic;\n"
            "        rst : in  std_logic;\n"
            "        j   : in  std_logic;\n"
            "        k   : in  std_logic;\n"
            "        q   : out std_logic;\n"
            "        qn  : out std_logic\n"
            "    );\n"
            "end entity %s;\n\n"
            "architecture rtl of %s is\n"
            "    signal q_reg : std_logic := '0';\n"
            "begin\n"
            "    process(clk)\n"
            "    begin\n"
            "        if rising_edge(clk) then\n"
            "            if rst = '1' then\n"
            "                q_reg <= '0';\n"
            "            else\n"
            "                case (j & k) is\n"
            "                    when \"10\"   => q_reg <= '1';        -- Set\n"
            "                    when \"01\"   => q_reg <= '0';        -- Reset\n"
            "                    when \"11\"   => q_reg <= not q_reg;  -- Toggle\n"
            "                    when others  => null;               -- Hold\n"
            "                end case;\n"
            "            end if;\n"
            "        end if;\n"
            "    end process;\n\n"
            "    q  <= q_reg;\n"
            "    qn <= not q_reg;\n"
            "end architecture rtl;\n",
            projectName, projectName, projectName);

    default:
        return "";
    }
}

void MainWindow::OnNewProject(wxCommandEvent& event)
{
    NewProjectDialog dlg(this);
    if (dlg.ShowModal() != wxID_OK)
        return;

    HideWelcomePanel();

    wxString projectName  = dlg.GetProjectName();
    wxString projectDir   = dlg.GetProjectDirectory();
    int      templateIdx  = dlg.GetTemplateIndex();

    if (projectName.IsEmpty() || projectDir.IsEmpty())
    {
        wxMessageBox("Please enter a project name and select a directory.",
                     "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxString fullProjectPath = projectDir + "/" + projectName;
    if (!wxDir::Exists(fullProjectPath))
        wxDir::Make(fullProjectPath);

    // Write template source file
    wxString srcFile = fullProjectPath + "/" + projectName + ".vhd";
    wxString srcText = BuildTemplateSource(templateIdx, projectName);
    if (!srcText.IsEmpty())
    {
        wxFile f(srcFile, wxFile::write);
        if (f.IsOpened())
            f.Write(srcText);
    }

    // Build and save the project file
    currentProject.SetDefaults(projectName);
    if (!srcText.IsEmpty())
        currentProject.sourceFiles.Add(srcFile);
    wxString projectFilePath = fullProjectPath + "/" + projectName + ".ocxproj";

    if (!currentProject.Save(projectFilePath))
    {
        wxMessageBox("Could not create project file.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    currentProjectDirectory = fullProjectPath;
    currentProjectFilePath  = projectFilePath;

    m_fileHistory->AddFileToHistory(projectFilePath);
    {
        wxConfig cfg("OpenCircuitX");
        m_fileHistory->Save(cfg);
    }

    circuitSimulator->SetWorkDir(currentProjectDirectory);
    LoadProjectFiles(fullProjectPath);
    UpdateRunConfigBar();

    // Auto-open the template file in the editor
    if (!srcText.IsEmpty() && wxFileExists(srcFile))
        logicEditor->LoadFile(srcFile);

    outputPanel->LogMessage("New project created: " + fullProjectPath);
    OCXStatus("Project: " + projectName);
}

//--
// Project open
//--
void MainWindow::OpenProjectFile(const wxString& projectFilePath)
{
    HideWelcomePanel();

    if (!currentProject.Load(projectFilePath))
    {
        wxMessageBox("Could not read project file:\n" + projectFilePath,
                     "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxFileName fn(projectFilePath);
    currentProjectDirectory = fn.GetPath();
    currentProjectFilePath  = projectFilePath;

    circuitSimulator->SetWorkDir(currentProjectDirectory);

    // Apply per-project GHDL path if set
    if (!currentProject.ghdlExecutablePath.IsEmpty())
        circuitSimulator->SetGHDLPath(currentProject.ghdlExecutablePath);

    // Clear stale output from any previous project or session
    outputPanel->ClearOutput();
    outputPanel->ClearErrors();

    LoadProjectFiles(currentProjectDirectory);
    UpdateRunConfigBar();

    // Close any tabs left over from a previous project or the startup session restore.
    logicEditor->CloseAllTabs();

    // Restore previously open tabs
    if (!currentProject.openTabs.IsEmpty())
    {
        wxStringTokenizer tokenizer(currentProject.openTabs, ",");
        while (tokenizer.HasMoreTokens())
        {
            wxString fileName = tokenizer.GetNextToken();
            fileName.Trim(true);
            fileName.Trim(false);
            if (!fileName.IsEmpty())
            {
                wxString filePath = currentProjectDirectory + "/" + fileName;
                if (wxFileExists(filePath))
                    logicEditor->LoadFile(filePath);
            }
        }
    }

    outputPanel->LogMessage("Opened project: " + projectFilePath);
    outputPanel->LogMessage("VHDL standard: " + currentProject.vhdlStandard);
    if (!currentProject.simTopEntity.IsEmpty())
        outputPanel->LogMessage("Simulation top entity: " + currentProject.simTopEntity);

    OCXStatus("Project: " + currentProject.projectName
                  + "  |  Board: " + m_fpgaBoardId);

    // Track in recent-projects MRU list
    if (m_fileHistory)
    {
        m_fileHistory->AddFileToHistory(projectFilePath);
        wxConfig cfg("OpenCircuitX");
        m_fileHistory->Save(cfg);
    }
}

void MainWindow::OnOpenProject(wxCommandEvent& event)
{
    OpenProjectDialog dlg(this);
    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString projectFilePath = dlg.GetProjectFilePath();
    if (!projectFilePath.IsEmpty())
        OpenProjectFile(projectFilePath);
}

//--
// Save helpers
//--
void MainWindow::UpdateRunConfigBar()
{
    if (!m_entityField)   return;
    m_entityField->SetValue(currentProject.simTopEntity);
    m_stopTimeField->SetValue(currentProject.runTime);

    // Sync VHDL std choice from project
    wxString std = currentProject.vhdlStandard.Lower();
    if      (std == "93") m_vhdlStdChoice->SetSelection(1);
    else if (std == "19") m_vhdlStdChoice->SetSelection(2);
    else                  m_vhdlStdChoice->SetSelection(0); // 08 default
}

void MainWindow::SaveProjectState()
{
    if (currentProjectFilePath.IsEmpty())
        return;

    // Save open tab filenames - only files that belong to this project directory.
    // Tabs from a previous project (left by session restore before this project opened)
    // must not be written into this project's openTabs or they will reappear next time.
    wxArrayString openPaths = logicEditor->GetOpenFilePaths();
    wxString tabList;
    wxFileName fnDir(currentProjectDirectory + wxFILE_SEP_PATH);
    fnDir.Normalize(wxPATH_NORM_ALL);
    wxString projDir = fnDir.GetFullPath();  // always ends with separator after Normalize
    for (size_t i = 0; i < openPaths.GetCount(); ++i)
    {
        if (openPaths[i].IsEmpty()) continue;
        wxFileName fn(openPaths[i]);
        fn.Normalize(wxPATH_NORM_ALL);
        if (!fn.GetFullPath().StartsWith(projDir)) continue;  // skip foreign file
        if (!tabList.IsEmpty()) tabList += ",";
        tabList += fn.GetFullName();
    }
    currentProject.openTabs = tabList;

    wxString currentPath = logicEditor->GetCurrentFilePath();
    if (!currentPath.IsEmpty())
        currentProject.lastOpenedFile = wxFileName(currentPath).GetFullName();

    currentProject.Save(currentProjectFilePath);
}

void MainWindow::OnSaveFile(wxCommandEvent& event)
{
    wxString filePath = logicEditor->GetCurrentFilePath();

    if (filePath.IsEmpty())
    {
        // Tab has no backing file (e.g. generated testbench) - prompt for location
        wxString defaultDir = currentProjectDirectory.IsEmpty()
                              ? wxGetCwd() : currentProjectDirectory;
        wxFileDialog dlg(this, "Save File As", defaultDir, "",
                         "HDL files (*.vhd;*.vhdl;*.v;*.sv)|*.vhd;*.vhdl;*.v;*.sv"
                         "|All files (*.*)|*.*",
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK)
            return;

        filePath = dlg.GetPath();
        logicEditor->SaveCurrentFileAs(filePath);

        // Add to project if open and not already tracked
        if (!currentProjectDirectory.IsEmpty() &&
            currentProject.sourceFiles.Index(filePath) == wxNOT_FOUND)
        {
            currentProject.sourceFiles.Add(filePath);
            LoadProjectFiles(currentProjectDirectory);
        }
    }
    else
    {
        logicEditor->SaveCurrentFile();
    }

    outputPanel->LogMessage("Saved: " + filePath);
    OCXStatus("Saved: " + wxFileName(filePath).GetFullName());
    SaveProjectState();
}

void MainWindow::OnSaveProject(wxCommandEvent& event)
{
    if (currentProjectFilePath.IsEmpty())
    {
        outputPanel->LogMessage("No project is open.");
        return;
    }

    SaveProjectState();
    outputPanel->LogMessage("Project saved: " + currentProjectFilePath);
    OCXStatus("Project saved.");
}

void MainWindow::OnSaveSimulation(wxCommandEvent& event)
{
    if (currentProjectFilePath.IsEmpty())
    {
        outputPanel->LogMessage("No project is open.");
        return;
    }

    wxString simPath = currentProjectDirectory + "/" + currentProject.simSaveFile;
    outputPanel->LogMessage("Simulation state saved: " + simPath);
}

//--
// Settings
//--
void MainWindow::OnSettings(wxCommandEvent& event)
{
    SettingsDialog dlg(this);
    if (dlg.ShowModal() != wxID_OK)
        return;

    wxString ghdlPath      = dlg.GetGHDLPath();
    wxString icarusPath    = dlg.GetIcarusPath();
    wxString verilatorPath = dlg.GetVerilatorPath();
    wxString stopTime      = dlg.GetStopTime();
    wxConfig config("OpenCircuitX");
    config.Write("GHDLPath",      ghdlPath);
    config.Write("IcarusPath",    icarusPath);
    config.Write("VerilatorPath", verilatorPath);
    config.Write("StopTime",      stopTime);
    circuitSimulator->SetGHDLPath(ghdlPath);
    circuitSimulator->SetIcarusPath(icarusPath);
    circuitSimulator->SetVerilatorPath(verilatorPath);
    circuitSimulator->SetRunTime(stopTime);

    // FPGA paths
    wxString yosysPath       = dlg.GetYosysPath();
    wxString nextpnrPath     = dlg.GetNextpnrPath();
    wxString nextpnrECP5Path = dlg.GetNextpnrECP5Path();
    wxString icepackPath     = dlg.GetIcepackPath();
    wxString ecppackPath     = dlg.GetEcppackPath();
    wxString loaderPath      = dlg.GetOpenFPGALoaderPath();
    wxString ghdlPlugin      = dlg.GetGHDLPluginPath();
    wxString boardId         = dlg.GetBoardId();
    config.Write("YosysPath",          yosysPath);
    config.Write("NextpnrPath",        nextpnrPath);
    config.Write("NextpnrECP5Path",    nextpnrECP5Path);
    config.Write("IcepackPath",        icepackPath);
    config.Write("EcppackPath",        ecppackPath);
    config.Write("OpenFPGALoaderPath", loaderPath);
    config.Write("GHDLYosysPlugin",    ghdlPlugin);
    config.Write("FPGABoard",          boardId);
    fpgaToolchain->SetYosysPath(yosysPath);
    fpgaToolchain->SetNextpnrPath(nextpnrPath);
    fpgaToolchain->SetNextpnrECP5Path(nextpnrECP5Path);
    fpgaToolchain->SetIcepackPath(icepackPath);
    fpgaToolchain->SetEcppackPath(ecppackPath);
    fpgaToolchain->SetOpenFPGALoaderPath(loaderPath);
    fpgaToolchain->SetGHDLPluginPath(ghdlPlugin);
    m_fpgaBoardId = boardId;

    // Auto-save interval - restart timer with new value
    int autoSaveMins = dlg.GetAutoSaveInterval();
    config.Write("AutoSaveInterval", autoSaveMins);
    m_autoSaveTimer.Stop();
    m_autoSaveTimer.Start(autoSaveMins * 60000);

    wxString msg = "Settings saved."
                   "  GHDL: " + ghdlPath +
                   "  |  iverilog: " + icarusPath +
                   "  |  verilator: " + verilatorPath;
    if (!stopTime.IsEmpty())
        msg += "  |  Stop time: " + stopTime;
    outputPanel->LogMessage(msg);
    outputPanel->LogMessage("FPGA board: " + m_fpgaBoardId
                            + "  |  Yosys: " + yosysPath);
    OCXStatus("Settings saved  |  Board: " + m_fpgaBoardId);
}

//--