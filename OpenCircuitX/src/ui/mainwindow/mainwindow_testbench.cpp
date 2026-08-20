#include "mainwindow.h"
#include "ui/editor/logic_editor_panel.h"
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/tokenzr.h>
#include <vector>

//--
// Testbench generator
//** Verilog/SV port parser - ANSI-style only **//
//** VHDL entity/port parser - line-by-line **//
//--
struct VerilogPort { wxString dir; wxString type; wxString name; };

static bool ParseVerilogModule(const wxString& src,
                                wxString& modName,
                                std::vector<VerilogPort>& ports)
{
    wxArrayString lines = wxStringTokenize(src, "\n");
    bool inModule  = false;
    bool inPorts   = false;
    bool portsDone = false;
    int  depth     = 0;
    wxString portBlock;

    for (const wxString& rawLine : lines)
    {
        wxString line = wxString(rawLine).Trim(true).Trim(false);
        // Strip // comments
        int cmt = line.Find("//");
        if (cmt != wxNOT_FOUND) line = line.Left(cmt).Trim(true);
        if (line.IsEmpty()) continue;

        wxString lo = line.Lower();

        if (!inModule)
        {
            // Match: module <name> (  or module <name>
            int mpos = lo.Find("module ");
            if (mpos != wxNOT_FOUND)
            {
                wxString rest = line.Mid(mpos + 7).Trim(false);
                // Remove any #(...) parameter list
                int hashPos = rest.Find('#');
                if (hashPos != wxNOT_FOUND) rest = rest.Left(hashPos).Trim(true);
                int parenPos = rest.Find('(');
                wxString name = parenPos != wxNOT_FOUND ?
                    rest.Left(parenPos).Trim(true) : rest.Trim(true);
                // Remove trailing semicolons/spaces
                name.Replace(";", "");
                modName  = name.Trim(true).Trim(false);
                inModule = true;
                // Count depth from this line onward
                for (wxChar ch : line)
                {
                    if (ch == '(') { ++depth; inPorts = true; }
                    else if (ch == ')') --depth;
                }
                if (inPorts)
                {
                    int op = line.Find('(');
                    if (depth > 0)
                    {
                        // Port list continues on following lines
                        portBlock = line.Mid(op + 1);
                    }
                    else
                    {
                        // Whole port list opens and closes on the module line,
                        // e.g.  module and_gate(input a, input b, output y);
                        int cp = line.Find(')', true);
                        if (cp > op)
                            portBlock = line.Mid(op + 1, cp - op - 1);
                        portsDone = true;
                    }
                }
            }
            if (portsDone) break;
            continue;
        }

        // Accumulate port block until closing paren
        for (wxChar ch : line)
        {
            if (ch == '(') ++depth;
            else if (ch == ')') --depth;
        }
        if (depth > 0)
            portBlock += "\n" + line;
        else
        {
            // depth hit 0 - the closing paren line may still contain ports before ')'
            int closeParen = line.Find(')');
            if (closeParen > 0)
                portBlock += "\n" + line.Left(closeParen);
            break;
        }
    }

    if (modName.IsEmpty()) return false;

    // Parse individual port declarations from portBlock
    // Split by commas that are not inside brackets
    wxArrayString tokens;
    wxString cur;
    int br = 0;
    for (wxChar ch : portBlock)
    {
        if (ch == '[') ++br;
        else if (ch == ']') --br;
        else if (ch == ',' && br == 0)
        {
            cur.Trim(true).Trim(false);
            if (!cur.IsEmpty()) tokens.Add(cur);
            cur.Clear();
            continue;
        }
        cur += ch;
    }
    cur.Trim(true).Trim(false);
    if (!cur.IsEmpty()) tokens.Add(cur);

    for (const wxString& tok : tokens)
    {
        wxString t = wxString(tok).Trim(true).Trim(false);
        if (t.IsEmpty()) continue;

        VerilogPort p;
        wxString lo = t.Lower();

        // Detect direction
        if      (lo.StartsWith("input ") || lo.StartsWith("input\t"))  { p.dir = "input";  t = t.Mid(5).Trim(false); }
        else if (lo.StartsWith("output ") || lo.StartsWith("output\t")){ p.dir = "output"; t = t.Mid(6).Trim(false); }
        else if (lo.StartsWith("inout ") || lo.StartsWith("inout\t"))  { p.dir = "inout";  t = t.Mid(5).Trim(false); }
        else { p.dir = "input"; /* bare name - old style */ }

        // Strip wire/reg/logic keywords
        lo = t.Lower();
        for (const wxString& kw : { wxString("wire "), wxString("reg "), wxString("logic ") })
            if (lo.StartsWith(kw)) { t = t.Mid(kw.length()).Trim(false); lo = t.Lower(); break; }

        // Detect range [x:y]
        if (t.StartsWith("["))
        {
            int rb = t.Find(']');
            if (rb != wxNOT_FOUND)
            {
                p.type = t.Left(rb + 1);
                t = t.Mid(rb + 1).Trim(false);
            }
        }
        p.name = t.Trim(true).Trim(false);
        p.name.Replace(";", "");
        if (!p.name.IsEmpty())
            ports.push_back(p);
    }

    return !modName.IsEmpty();
}

static bool ParseVHDLEntity(const wxString& src,
                             wxString& entityName,
                             wxArrayString& portLines)
{
    wxArrayString lines = wxStringTokenize(src, "\n");
    bool inEntity = false;
    bool inPort   = false;
    int  depth    = 0;

    for (const wxString& rawLine : lines)
    {
        wxString line = wxString(rawLine).Trim(true).Trim(false).Lower();

        // Strip inline comment
        int cmtPos = line.Find("--");
        if (cmtPos != wxNOT_FOUND)
            line = line.Left(cmtPos).Trim(true);

        if (!inEntity)
        {
            if (line.StartsWith("entity ") && line.Contains(" is"))
            {
                // entity <name> is
                wxString rest = line.Mid(7); // after "entity "
                int isPos = rest.Find(" is");
                if (isPos != wxNOT_FOUND)
                {
                    entityName = wxString(rawLine).Trim(false).Mid(7).Left(isPos).Trim(true);
                    inEntity = true;
                }
            }
            continue;
        }

        if (!inPort)
        {
            if (line.Contains("port") && line.Contains("("))
            {
                inPort = true;
                depth  = 1;
            }
            else if (line.StartsWith("end "))
                break;
            continue;
        }

        // Inside port block
        for (wxChar ch : line)
        {
            if (ch == '(') ++depth;
            else if (ch == ')') --depth;
        }

        if (depth <= 0)
            break;

        // Only keep lines that look like port declarations
        if (line.Contains(":") &&
            (line.Contains(" in ") || line.Contains(" out ") ||
             line.Contains(" inout ") || line.Contains(" buffer ")))
        {
            // Restore original casing from rawLine
            portLines.Add(wxString(rawLine).Trim(true).Trim(false));
        }
    }
    return !entityName.IsEmpty();
}

void MainWindow::OnGenTestbench(wxCommandEvent&)
{
    workspaceNotebook->SetSelection(0); // ensure HDL Editor is active

    wxString src = logicEditor->GetCode();
    if (src.IsEmpty())
    {
        wxMessageBox("No file open in the editor.", "Generate Testbench",
                     wxOK | wxICON_INFORMATION);
        return;
    }

    wxString ext = logicEditor->GetCurrentFileExt().Lower();
    const bool isVerilog = (ext == "v" || ext == "sv");

    //** Verilog / SystemVerilog testbench **//
    if (isVerilog)
    {
        wxString modName;
        std::vector<VerilogPort> ports;
        if (!ParseVerilogModule(src, modName, ports))
        {
            wxMessageBox("Could not find a Verilog module declaration in the current file.",
                         "Generate Testbench", wxOK | wxICON_WARNING);
            return;
        }

        // Detect clk and rst ports
        wxString clkPort, rstPort;
        for (const VerilogPort& p : ports)
        {
            wxString lo = p.name.Lower();
            if (clkPort.IsEmpty() && (lo == "clk" || lo == "clock" || lo.Contains("clk")))
                clkPort = p.name;
            if (rstPort.IsEmpty() && (lo == "rst" || lo == "reset" || lo == "rstn" || lo.Contains("rst")))
                rstPort = p.name;
        }

        wxString tb;
        tb += "`timescale 1ns / 1ps\n\n";
        tb += "module tb_" + modName + ";\n\n";

        // Signal declarations
        for (const VerilogPort& p : ports)
        {
            wxString decl = (p.dir == "output") ? "wire" : "reg";
            if (!p.type.IsEmpty())
                tb += "    " + decl + " " + p.type + " " + p.name + ";\n";
            else
                tb += "    " + decl + " " + p.name + ";\n";
        }
        tb += "\n";

        // Initialise regs
        tb += "    initial begin\n";
        for (const VerilogPort& p : ports)
        {
            if (p.dir != "output")
                tb += "        " + p.name + " = 0;\n";
        }
        tb += "    end\n\n";

        // Waveform dump - Icarus has no --vcd flag, so the testbench has to ask
        // for the waveform itself. The name matches what Run Simulation looks for.
        tb += "    // Waveform output - required for the Waveform tab to load anything\n";
        tb += "    initial begin\n";
        tb += "        $dumpfile(\"tb_" + modName + ".vcd\");\n";
        tb += "        $dumpvars(0, tb_" + modName + ");\n";
        tb += "    end\n\n";

        // DUT instantiation
        tb += "    " + modName + " DUT (\n";
        for (int i = 0; i < (int)ports.size(); ++i)
        {
            wxString comma = (i < (int)ports.size() - 1) ? "," : "";
            tb += "        ." + ports[i].name + "(" + ports[i].name + ")" + comma + "\n";
        }
        tb += "    );\n\n";

        // Clock generator
        if (!clkPort.IsEmpty())
            tb += "    // 100 MHz clock (period = 10 ns)\n"
                  "    always #5 " + clkPort + " = ~" + clkPort + ";\n\n";

        // Stimulus
        tb += "    initial begin\n";
        if (!rstPort.IsEmpty())
        {
            tb += "        " + rstPort + " = 1;\n";
            tb += "        #20;\n";
            tb += "        " + rstPort + " = 0;\n";
            tb += "        #10;\n";
        }
        tb += "        // TODO: add test stimulus here\n";
        tb += "        #100;\n";
        tb += "        $finish;\n";
        tb += "    end\n\n";

        tb += "endmodule\n";

        wxString tbName = "tb_" + modName + ".v";
        if (!currentProjectDirectory.IsEmpty())
        {
            wxString tbPath = currentProjectDirectory + "/" + tbName;
            if (wxFileExists(tbPath) &&
                wxMessageBox("\"" + tbName + "\" already exists. Overwrite?",
                             "Generate Testbench", wxYES_NO | wxICON_QUESTION, this) != wxYES)
                return;
            wxFile f(tbPath, wxFile::write);
            if (!f.IsOpened())
            {
                wxMessageBox("Could not write \"" + tbName + "\".\n"
                             "Check that the project directory is writable.",
                             "Generate Testbench", wxOK | wxICON_ERROR, this);
                return;
            }
            f.Write(tb.ToUTF8(), tb.ToUTF8().length());
            f.Close();
            if (currentProject.sourceFiles.Index(tbPath) == wxNOT_FOUND)
                currentProject.sourceFiles.Add(tbPath);
            SaveProjectState();
            LoadProjectFiles(currentProjectDirectory);
            logicEditor->LoadFile(tbPath);
        }
        else
        {
            logicEditor->NewTabWithContent(tbName, tb);
        }
        OCXStatus("Verilog testbench generated for: " + modName);
        return;
    }

    //** VHDL testbench (smart - detects clk + rst, generates clock process) **//
    wxString entityName;
    wxArrayString portLines;
    if (!ParseVHDLEntity(src, entityName, portLines))
    {
        wxMessageBox("Could not find a VHDL entity declaration in the current file.\n"
                     "Make sure the file contains: entity <name> is",
                     "Generate Testbench", wxOK | wxICON_WARNING);
        return;
    }

    // Helper: extract signal name and type from a raw port line
    // portLines entries look like: "clk : in std_logic"
    struct PortInfo { wxString name; wxString dir; wxString type; };
    std::vector<PortInfo> pinfo;
    wxString clkSig, rstSig;

    for (const wxString& pl : portLines)
    {
        int colon = pl.Find(':');
        if (colon == wxNOT_FOUND) continue;
        PortInfo pi;
        pi.name = pl.Left(colon).Trim(true).Trim(false);
        wxString rest = pl.Mid(colon + 1).Trim(false);
        wxString lo   = rest.Lower();
        for (const wxString& dir : { wxString("in "), wxString("out "),
                                      wxString("inout "), wxString("buffer ") })
        {
            if (lo.StartsWith(dir)) { pi.dir = wxString(dir).Trim(); rest = rest.Mid(dir.length()).Trim(false); lo = rest.Lower(); break; }
        }
        rest.Replace(";", "");
        pi.type = rest.Trim(true);
        pinfo.push_back(pi);

        wxString nameLo = pi.name.Lower();
        if (clkSig.IsEmpty() && (nameLo == "clk" || nameLo == "clock" || nameLo.Contains("clk")))
            clkSig = pi.name;
        if (rstSig.IsEmpty() && (nameLo == "rst" || nameLo == "reset" || nameLo == "rstn" || nameLo.Contains("rst")))
            rstSig = pi.name;
    }

    wxString tb;
    tb += "library IEEE;\n";
    tb += "use IEEE.STD_LOGIC_1164.ALL;\n\n";
    tb += "entity tb_" + entityName + " is\n";
    tb += "end tb_" + entityName + ";\n\n";
    tb += "architecture sim of tb_" + entityName + " is\n\n";

    // Component declaration
    tb += "    component " + entityName + "\n";
    tb += "        port (\n";
    for (int i = 0; i < (int)portLines.GetCount(); ++i)
    {
        wxString pl = portLines[i];
        if (!pl.EndsWith(";") && i < (int)portLines.GetCount() - 1) pl += ";";
        if (pl.EndsWith(";") && i == (int)portLines.GetCount() - 1) pl = pl.Left(pl.Length() - 1);
        tb += "            " + pl + "\n";
    }
    tb += "        );\n";
    tb += "    end component;\n\n";

    // Clock period constant
    if (!clkSig.IsEmpty())
        tb += "    constant CLK_PERIOD : time := 10 ns;  -- 100 MHz\n\n";

    // Signal declarations
    for (const PortInfo& pi : pinfo)
        tb += "    signal " + pi.name + " : " + pi.type + ";\n";

    tb += "\nbegin\n\n";

    // DUT instantiation
    tb += "    DUT: " + entityName + "\n";
    tb += "        port map (\n";
    for (int i = 0; i < (int)pinfo.size(); ++i)
    {
        wxString entry = "            " + pinfo[i].name + " => " + pinfo[i].name;
        if (i < (int)pinfo.size() - 1) entry += ",";
        tb += entry + "\n";
    }
    tb += "        );\n\n";

    // Clock process
    if (!clkSig.IsEmpty())
    {
        tb += "    -- Clock generator: " + clkSig + " toggles every CLK_PERIOD/2\n";
        tb += "    clk_gen: process\n";
        tb += "    begin\n";
        tb += "        " + clkSig + " <= '0'; wait for CLK_PERIOD / 2;\n";
        tb += "        " + clkSig + " <= '1'; wait for CLK_PERIOD / 2;\n";
        tb += "    end process;\n\n";
    }

    // Stimulus process
    tb += "    stim_proc: process\n";
    tb += "    begin\n";
    if (!rstSig.IsEmpty())
    {
        tb += "        -- Assert reset for 2 clock cycles\n";
        tb += "        " + rstSig + " <= '1';\n";
        if (!clkSig.IsEmpty())
            tb += "        wait for CLK_PERIOD * 2;\n";
        else
            tb += "        wait for 20 ns;\n";
        tb += "        " + rstSig + " <= '0';\n";
        tb += "        wait for CLK_PERIOD;\n\n";
    }
    tb += "        -- TODO: add test stimulus here\n\n";
    if (!clkSig.IsEmpty())
        tb += "        wait for CLK_PERIOD * 10;\n";
    else
        tb += "        wait for 100 ns;\n";
    tb += "        wait;\n";
    tb += "    end process;\n\n";
    tb += "end sim;\n";

    wxString tbName = "tb_" + entityName + ".vhd";
    if (!currentProjectDirectory.IsEmpty())
    {
        wxString tbPath = currentProjectDirectory + "/" + tbName;
        if (wxFileExists(tbPath) &&
            wxMessageBox("\"" + tbName + "\" already exists. Overwrite?",
                         "Generate Testbench", wxYES_NO | wxICON_QUESTION, this) != wxYES)
            return;
        wxFile f(tbPath, wxFile::write);
        if (!f.IsOpened())
        {
            wxMessageBox("Could not write \"" + tbName + "\".\n"
                         "Check that the project directory is writable.",
                         "Generate Testbench", wxOK | wxICON_ERROR, this);
            return;
        }
        f.Write(tb.ToUTF8(), tb.ToUTF8().length());
        f.Close();
        if (currentProject.sourceFiles.Index(tbPath) == wxNOT_FOUND)
            currentProject.sourceFiles.Add(tbPath);
        SaveProjectState();
        LoadProjectFiles(currentProjectDirectory);
        logicEditor->LoadFile(tbPath);
    }
    else
    {
        logicEditor->NewTabWithContent(tbName, tb);
    }
    OCXStatus("Testbench generated for: " + entityName);
}
