#include "fpga_toolchain.h"
#include <wx/utils.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/tokenzr.h>

//--
// Static board list
//--
// BoardProfile field order:
//   id, name, family,
//   nextpnrDevice, nextpnrPkg, synthTarget, constraintFlag,
//   pnrOutExt, bitExt,
//   lutTotal, ffTotal, bramTotal

static const std::vector<BoardProfile> s_boards =
{
    // ---- iCE40 family ----
    {
        "ibreaker", "iCEBreaker (UP5K, sg48)", "ice40",
        "--up5k", "--package sg48", "synth_ice40", "--pcf",
        ".asc", ".bin",
        5280, 5120, 30
    },
    {
        "ice40hx8k", "iCE40-HX8K (ct256)", "ice40",
        "--hx8k", "--package ct256", "synth_ice40", "--pcf",
        ".asc", ".bin",
        7680, 7680, 32
    },
    {
        "tinyfpga", "TinyFPGA BX (LP8K, cm81)", "ice40",
        "--lp8k", "--package cm81", "synth_ice40", "--pcf",
        ".asc", ".bin",
        7680, 7680, 0
    },
    {
        "upduino3", "UPduino v3 (UP5K, sg48)", "ice40",
        "--up5k", "--package sg48", "synth_ice40", "--pcf",
        ".asc", ".bin",
        5280, 5120, 30
    },

    // ---- ECP5 family ----
    {
        "colorlight_i5", "ColorLight i5 (ECP5-25F, CABGA256)", "ecp5",
        "--25k", "--package CABGA256", "synth_ecp5", "--lpf",
        ".config", ".bit",
        24288, 24288, 56
    },
    {
        "orangecrab", "OrangeCrab r0.2 (ECP5-25F, CSFBGA285)", "ecp5",
        "--25k", "--package CSFBGA285", "synth_ecp5", "--lpf",
        ".config", ".bit",
        24288, 24288, 56
    },
    {
        "ulx3s_85", "ULX3S 85F (ECP5-85F, CABGA381)", "ecp5",
        "--85k", "--package CABGA381", "synth_ecp5", "--lpf",
        ".config", ".bit",
        84000, 84000, 208
    },
};

const std::vector<BoardProfile>& FPGAToolchain::GetBoards()
{
    return s_boards;
}

//--
// Constructor / setters
//--
FPGAToolchain::FPGAToolchain()
    : m_yosysPath("yosys")
    , m_nextpnrPath("nextpnr-ice40")
    , m_nextpnrECP5Path("nextpnr-ecp5")
    , m_icepackPath("icepack")
    , m_ecppackPath("ecppack")
    , m_loaderPath("openFPGALoader")
{
}

void FPGAToolchain::SetYosysPath(const wxString& p)
{
    m_yosysPath = p.IsEmpty() ? "yosys" : p;
}

void FPGAToolchain::SetNextpnrPath(const wxString& p)
{
    m_nextpnrPath = p.IsEmpty() ? "nextpnr-ice40" : p;
}

void FPGAToolchain::SetIcepackPath(const wxString& p)
{
    m_icepackPath = p.IsEmpty() ? "icepack" : p;
}

void FPGAToolchain::SetEcppackPath(const wxString& p)
{
    m_ecppackPath = p.IsEmpty() ? "ecppack" : p;
}

void FPGAToolchain::SetNextpnrECP5Path(const wxString& p)
{
    m_nextpnrECP5Path = p.IsEmpty() ? "nextpnr-ecp5" : p;
}

void FPGAToolchain::SetOpenFPGALoaderPath(const wxString& p)
{
    m_loaderPath = p.IsEmpty() ? "openFPGALoader" : p;
}

void FPGAToolchain::SetWorkDir(const wxString& dir)
{
    m_workDir = dir;
}

void FPGAToolchain::SetGHDLPluginPath(const wxString& path)
{
    m_ghdlPluginPath = path;
}

void FPGAToolchain::SetProjectFiles(const wxArrayString& files)
{
    m_projectFiles = files;
}

//--
// Availability checks
//--
bool FPGAToolchain::IsYosysAvailable() const
{
    wxArrayString o, e;
    return Execute("\"" + m_yosysPath + "\" --version", o, e, m_yosysPath) == 0;
}

bool FPGAToolchain::IsNextpnrAvailable() const
{
    wxArrayString o, e;
    return Execute("\"" + m_nextpnrPath + "\" --version", o, e, m_nextpnrPath) == 0;
}

bool FPGAToolchain::IsIcepackAvailable() const
{
    wxArrayString o, e;
    // icepack with no args exits non-zero but prints help - check stderr not empty
    Execute("\"" + m_icepackPath + "\"", o, e, m_icepackPath);
    return !e.IsEmpty() || !o.IsEmpty();
}

bool FPGAToolchain::IsOpenFPGALoaderAvailable() const
{
    wxArrayString o, e;
    return Execute("\"" + m_loaderPath + "\" --Version", o, e, m_loaderPath) == 0;
}

//--
// Execute helper
//--
int FPGAToolchain::Execute(const wxString& cmd,
                            wxArrayString& output,
                            wxArrayString& errors,
                            const wxString& toolPath) const
{
    wxExecuteEnv env;
    bool needEnv = false;

    if (!m_workDir.IsEmpty())
    {
        env.cwd  = m_workDir;
        needEnv  = true;
    }

    // OSS CAD Suite (and similar bundles) ship each tool's runtime DLLs in a
    // lib/ directory next to bin/, and only add both to PATH via their own
    // launcher script. Pointing Settings straight at bin/yosys.exe skips that
    // script, so the tool fails to even start (e.g. "libffi-8.dll not found")
    // unless we add lib/ back onto PATH ourselves.
    if (!toolPath.IsEmpty())
    {
        wxString binDir = wxFileName(toolPath).GetPath();
        wxFileName libFn = wxFileName::DirName(binDir);
        libFn.RemoveLastDir();
        libFn.AppendDir("lib");
        wxString libDir = libFn.GetPath();

        if (!libDir.IsEmpty() && wxDirExists(libDir))
        {
            wxGetEnvMap(&env.env);

            // wxGetEnvMap() preserves whatever casing Windows stored the
            // variable under - almost always "Path", not "PATH". Find and
            // erase any case-insensitive match by key first (wx's hash map
            // erase(iterator) returns void, doesn't invalidate-and-advance
            // like std::map, so collect keys before erasing) so we replace
            // it rather than add a second, conflicting entry alongside it.
            wxString existingPath;
            wxArrayString staleKeys;
            for (const auto& kv : env.env)
                if (kv.first.CmpNoCase("PATH") == 0)
                {
                    existingPath = kv.second;
                    staleKeys.Add(kv.first);
                }
            for (const wxString& k : staleKeys)
                env.env.erase(k);

            env.env["PATH"] = libDir + wxPATH_SEP + binDir
                             + (existingPath.IsEmpty() ? wxString()
                                                        : wxPATH_SEP + existingPath);
            needEnv = true;
        }
    }

    return (int)wxExecute(cmd, output, errors, wxEXEC_SYNC,
                          needEnv ? &env : nullptr);
}

//--
// SynthReport parsing
// Yosys prints the cell breakdown as "<count>   <CellName>", count first, e.g.:
//   "        1   SB_LUT4"
//   "        8   SB_DFF"
//   "        2   SB_RAM40_4K"
// under a "=== <module> ===" header ("Local Count" - just this module) and
// again under "=== design hierarchy ===" (count including submodules) - only
// the first block is parsed, otherwise every count would be doubled for any
// design with submodules.
//--
SynthReport FPGAToolchain::ParseYosysReport(const wxArrayString& lines)
{
    SynthReport r;
    int dffs = 0;
    int brams = 0;

    // Pulls the first whitespace-separated integer token out of the line,
    // wherever it falls - Yosys puts it before the cell name, not after.
    auto firstIntToken = [](const wxString& ln) -> long {
        wxStringTokenizer tok(ln, " \t");
        while (tok.HasMoreTokens())
        {
            long v;
            if (tok.GetNextToken().ToLong(&v))
                return v;
        }
        return 0;
    };

    for (size_t i = 0; i < lines.GetCount(); ++i)
    {
        wxString line = lines[i];

        // Stop before the submodule-inclusive block repeats the same names.
        // Matches only the "=== design hierarchy ===" stats header, not the
        // unrelated "Executing HIERARCHY pass (managing design hierarchy)"
        // logged near the very start of every run.
        if (line.Contains("=== design hierarchy ==="))
            break;

        // iCE40 LUT
        if (line.Contains("SB_LUT4"))
            r.luts = (int)firstIntToken(line);

        // ECP5 LUT
        if (line.Contains("LUT4") && !line.Contains("SB_LUT4"))
            r.luts = (r.luts < 0 ? 0 : r.luts) + (int)firstIntToken(line);

        // iCE40 DFFs (all variants: SB_DFF, SB_DFFE, SB_DFFSR, ...)
        if (line.Contains("SB_DFF"))
            dffs += (int)firstIntToken(line);

        // ECP5 DFFs (FD1S3AX, FD1S3IX, FD1S3BX, TRELLIS_FF, ...)
        if (line.Contains("FD1S3") || line.Contains("TRELLIS_FF"))
            dffs += (int)firstIntToken(line);

        // iCE40 BRAM
        if (line.Contains("SB_RAM40_4K"))
            brams += (int)firstIntToken(line);

        // ECP5 BRAM (DP16KD = 16Kbit; report in same unit as lutTotal for consistency)
        if (line.Contains("DP16KD"))
            brams += (int)firstIntToken(line);
    }

    if (r.luts >= 0)
    {
        r.ffs   = dffs;
        r.brams = brams;
        r.valid = true;
    }

    return r;
}

//--
// FormatReport - builds the resource-usage table shown in Output tab
//--
wxString FPGAToolchain::FormatReport(const SynthReport& r, const BoardProfile& board)
{
    if (!r.valid)
        return "No synthesis statistics available.\n";

    wxString out;
    out += "=== Synthesis Report - " + board.name + " ===\n";
    out += wxString::Format("%-14s  %6s  %6s  %6s\n", "Resource", "Used", "Total", "Usage");
    out += wxString(45, '-') + "\n";

    auto fmtRow = [&](const wxString& name, int used, int total)
    {
        if (used < 0) return;
        double pct = total > 0 ? (used * 100.0 / total) : 0.0;
        out += wxString::Format("%-14s  %6d  %6d  %5.1f%%\n",
                                name, used, total, pct);
    };

    fmtRow("LUT4",        r.luts,  board.lutTotal);
    fmtRow("DFF",         r.ffs,   board.ffTotal);
    fmtRow("BRAM (4Kb)",  r.brams, board.bramTotal);
    out += "\n";

    return out;
}

//--
// Synthesize
//--
bool FPGAToolchain::Synthesize(const wxString& filePath,
                                const wxString& topEntity,
                                const wxString& outDir,
                                const BoardProfile& board,
                                wxArrayString& output,
                                wxArrayString& errors,
                                SynthReport& report)
{
    if (filePath.IsEmpty())
    {
        errors.Add("No file selected for synthesis.");
        return false;
    }

    wxString stem    = wxFileName(filePath).GetName();
    wxString jsonOut = outDir + "/" + stem + ".json";
    wxString ysFile  = outDir + "/" + stem + "_synth.ys";

    wxString ext   = wxFileName(filePath).GetExt().Lower();
    bool     isVHDL = (ext == "vhd" || ext == "vhdl");

    //--
    // Build the yosys script file.
    //
    // Writing a .ys file instead of passing -p "..." on the command line avoids
    // all shell-quoting and semicolon-escaping issues on Windows, Linux, macOS.
    // Paths go into the script with escaped backslashes; forward slashes also
    // work on all three platforms inside yosys itself.
    //--
    wxString script;

    if (isVHDL)
    {
        // GHDL frontend: one command that analyzes all VHDL source files and
        // elaborates the top entity.  If the project has listed source files
        // use them all (so dependencies are analyzed together); otherwise fall
        // back to the single file that was passed in.
        wxArrayString sources = m_projectFiles.IsEmpty()
                              ? wxArrayString(1, &filePath)
                              : m_projectFiles;

        // ghdl --std=08 "file1.vhd" "file2.vhd" -e <top>
        script += "ghdl --std=08";
        for (size_t i = 0; i < sources.GetCount(); ++i)
        {
            wxString p = sources[i];
            p.Replace("\\", "/");           // yosys prefers forward slashes
            script += " \"" + p + "\"";
        }
        script += " -e " + topEntity + "\n";
    }
    else
    {
        // Verilog / SystemVerilog
        wxString fp = filePath;
        fp.Replace("\\", "/");
        script += "read_verilog \"" + fp + "\"\n";
    }

    // Common synthesis step
    wxString jp = jsonOut;
    jp.Replace("\\", "/");
    script += board.synthTarget + " -top " + topEntity + " -json \"" + jp + "\"\n";

    // Write the script file
    {
        wxFile ysf(ysFile, wxFile::write);
        if (!ysf.IsOpened())
        {
            errors.Add("Cannot write synthesis script: " + ysFile);
            return false;
        }
        ysf.Write(script, wxConvUTF8);
    }

    //--
    // Build the yosys command.
    // No inline -p "..." needed - just pass the script file as a positional arg.
    //--
    wxString cmd = "\"" + m_yosysPath + "\"";

    if (isVHDL)
    {
        // Load the GHDL plugin.  If m_ghdlPluginPath is empty, yosys will
        // look for a plugin named "ghdl" on its plugin search path.
        wxString plugin = m_ghdlPluginPath.IsEmpty() ? "ghdl"
                                                      : "\"" + m_ghdlPluginPath + "\"";
        cmd += " -m " + plugin;
    }

    wxString yp = ysFile;
    yp.Replace("\\", "/");
    cmd += " \"" + yp + "\"";

    output.Add(wxString(isVHDL ? "Synthesizing (VHDL via GHDL plugin): "
                               : "Synthesizing: ")
               + wxFileName(filePath).GetFullName()
               + "  (top: " + topEntity + ")");
    output.Add("Script: " + ysFile);

    int exitCode = Execute(cmd, output, errors, m_yosysPath);

    if (exitCode == 0)
    {
        output.Add("Synthesis OK -> " + jsonOut);
        report = ParseYosysReport(output);
    }
    else
    {
        errors.Add("Synthesis failed. Check errors above.");
    }

    return exitCode == 0;
}

//--
// Place & Route
//--
bool FPGAToolchain::PlaceAndRoute(const wxString& jsonFile,
                                   const wxString& pcfFile,
                                   const BoardProfile& board,
                                   const wxString& outDir,
                                   wxArrayString& output,
                                   wxArrayString& errors)
{
    if (!wxFileExists(jsonFile))
    {
        errors.Add("JSON netlist not found: " + jsonFile + "\nRun Synthesize first.");
        return false;
    }

    wxString stem    = wxFileName(jsonFile).GetName();
    wxString pnrOut  = outDir + "/" + stem + board.pnrOutExt;

    // Pick the right nextpnr binary and output flag based on family
    bool isECP5 = (board.family == "ecp5");
    wxString pnrPath = isECP5 ? m_nextpnrECP5Path : m_nextpnrPath;
    wxString pnrBin  = "\"" + pnrPath + "\"";
    wxString outFlag = isECP5 ? "--textcfg" : "--asc";

    wxString cmd = pnrBin
                 + " " + board.nextpnrDevice
                 + " " + board.nextpnrPkg
                 + " --json \"" + jsonFile + "\""
                 + " " + outFlag + " \"" + pnrOut + "\"";

    if (!pcfFile.IsEmpty() && wxFileExists(pcfFile))
        cmd += " " + board.constraintFlag + " \"" + pcfFile + "\"";

    output.Add("Place & Route: " + wxFileName(jsonFile).GetFullName()
               + "  (board: " + board.name + ")");

    int exitCode = Execute(cmd, output, errors, pnrPath);

    if (exitCode == 0)
        output.Add("P&R OK -> " + pnrOut);
    else
        errors.Add("Place & route failed. Check errors above.");

    return exitCode == 0;
}

//--
// Pack  (ASC → BIN)
//--
bool FPGAToolchain::Pack(const wxString& ascFile,
                          const wxString& outBinFile,
                          wxArrayString& output,
                          wxArrayString& errors)
{
    if (!wxFileExists(ascFile))
    {
        errors.Add("P&R output not found: " + ascFile + "\nRun Place & Route first.");
        return false;
    }

    // Detect ECP5 by extension (.config) vs iCE40 (.asc)
    wxString ext = wxFileName(ascFile).GetExt().Lower();
    bool isECP5  = (ext == "config");

    wxString cmd;
    wxString packerPath = isECP5 ? m_ecppackPath : m_icepackPath;
    if (isECP5)
    {
        // ecppack input.config output.bit
        cmd = "\"" + m_ecppackPath + "\" \""
            + ascFile + "\" \"" + outBinFile + "\"";
    }
    else
    {
        // icepack input.asc output.bin
        cmd = "\"" + m_icepackPath + "\" \""
            + ascFile + "\" \"" + outBinFile + "\"";
    }

    output.Add("Packing: " + wxFileName(ascFile).GetFullName());

    int exitCode = Execute(cmd, output, errors, packerPath);

    if (exitCode == 0)
        output.Add("Pack OK -> " + outBinFile);
    else
        errors.Add(wxString(isECP5 ? "ecppack" : "icepack") + " failed. Check errors above.");

    return exitCode == 0;
}

//--
// Program board
//--
bool FPGAToolchain::Program(const wxString& bitstreamFile,
                             const BoardProfile& board,
                             wxArrayString& output,
                             wxArrayString& errors)
{
    if (!wxFileExists(bitstreamFile))
    {
        errors.Add("Bitstream not found: " + bitstreamFile);
        return false;
    }

    // openFPGALoader -b <boardId> <bitstream>
    wxString cmd = "\"" + m_loaderPath + "\" -b " + board.id
                 + " \"" + bitstreamFile + "\"";

    output.Add("Programming board: " + board.name);
    output.Add("Bitstream: " + bitstreamFile);

    int exitCode = Execute(cmd, output, errors, m_loaderPath);

    if (exitCode == 0)
        output.Add("Board programmed successfully.");
    else
        errors.Add("Programming failed. Check USB connection and board type.");

    return exitCode == 0;
}

//--
// RunFullFlow
//--
bool FPGAToolchain::RunFullFlow(const wxString& filePath,
                                 const wxString& topEntity,
                                 const wxString& pcfFile,
                                 const BoardProfile& board,
                                 const wxString& outDir,
                                 bool doProgram,
                                 wxArrayString& output,
                                 wxArrayString& errors,
                                 SynthReport& report)
{
    output.Add("=== FPGA Full Flow: " + board.name + " ===");

    // 1. Synthesize
    if (!Synthesize(filePath, topEntity, outDir, board, output, errors, report))
        return false;

    // 2. Place & Route
    wxString stem     = wxFileName(filePath).GetName();
    wxString jsonFile = outDir + "/" + stem + ".json";
    if (!PlaceAndRoute(jsonFile, pcfFile, board, outDir, output, errors))
        return false;

    // 3. Pack - use board-specific extensions
    wxString ascFile = outDir + "/" + stem + board.pnrOutExt;
    wxString binFile = outDir + "/" + stem + board.bitExt;
    if (!Pack(ascFile, binFile, output, errors))
        return false;

    // 4. (Optional) Program
    if (doProgram)
    {
        if (!Program(binFile, board, output, errors))
            return false;
    }
    else
    {
        output.Add("Bitstream ready: " + binFile);
        output.Add("Use Tools > FPGA > Program Board to flash.");
    }

    output.Add("=== FPGA Flow Complete ===");
    return true;
}
