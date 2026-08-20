#include "circuit_simulator.h"
#include <wx/utils.h>
#include <wx/filename.h>

CircuitSimulator::CircuitSimulator()
    : ghdlPath("ghdl")
    , icarusPath("iverilog")
    , verilatorPath("verilator")
    , workDir("")
    , m_vhdlStd("08")
{
}

void CircuitSimulator::SetGHDLPath(const wxString& path)
{
    ghdlPath = path.IsEmpty() ? "ghdl" : path;
    m_ghdlAvail = -1;   // invalidate cache on path change
}

void CircuitSimulator::SetIcarusPath(const wxString& path)
{
    icarusPath = path.IsEmpty() ? "iverilog" : path;
    m_icarusAvail = -1;
}

void CircuitSimulator::SetVerilatorPath(const wxString& path)
{
    verilatorPath = path.IsEmpty() ? "verilator" : path;
    m_verilatorAvail = -1;
}

void CircuitSimulator::SetVHDLStd(const wxString& std)
{
    m_vhdlStd = std.IsEmpty() ? "08" : std;
}

void CircuitSimulator::SetWorkDir(const wxString& dir)
{
    workDir = dir;
}

void CircuitSimulator::SetProjectFiles(const wxArrayString& files)
{
    m_projectFiles = files;
}

void CircuitSimulator::SetRunTime(const wxString& stopTime)
{
    m_stopTime = stopTime;
    m_stopTime.Trim(true);
    m_stopTime.Trim(false);
}

bool CircuitSimulator::IsGHDLAvailable() const
{
    if (m_ghdlAvail < 0)
    {
        wxArrayString out, err;
        m_ghdlAvail = (Execute("\"" + ghdlPath + "\" --version", out, err, ghdlPath) == 0) ? 1 : 0;
    }
    return m_ghdlAvail == 1;
}

bool CircuitSimulator::IsIcarusAvailable() const
{
    if (m_icarusAvail < 0)
    {
        wxArrayString out, err;
        m_icarusAvail = (Execute("\"" + icarusPath + "\" -V", out, err, icarusPath) == 0) ? 1 : 0;
    }
    return m_icarusAvail == 1;
}

bool CircuitSimulator::IsVerilatorAvailable() const
{
    if (m_verilatorAvail < 0)
    {
        wxArrayString out, err;
        m_verilatorAvail = (Execute("\"" + verilatorPath + "\" --version", out, err, verilatorPath) == 0) ? 1 : 0;
    }
    return m_verilatorAvail == 1;
}

wxString CircuitSimulator::GHDLWorkdirFlag() const
{
    if (workDir.IsEmpty()) return wxEmptyString;
    // GHDL on Windows is a MinGW binary - use forward slashes in all paths.
    wxString wd = workDir;
    wd.Replace("\\", "/");
    while (!wd.IsEmpty() && wd.Last() == '/') wd.RemoveLast();
    return " --workdir=\"" + wd + "\"";
}

int CircuitSimulator::Execute(const wxString& cmd,
                               wxArrayString& output,
                               wxArrayString& errors,
                               const wxString& toolPath) const
{
    wxExecuteEnv env;
    bool needEnv = false;

    if (!workDir.IsEmpty())
    {
        wxFileName fnwd(workDir);
        fnwd.Normalize(wxPATH_NORM_ALL);
        wxString normalizedCwd = fnwd.GetFullPath();
        if (!normalizedCwd.IsEmpty() && normalizedCwd.Last() == wxFILE_SEP_PATH)
            normalizedCwd.RemoveLast();
        env.cwd = normalizedCwd;
        needEnv = true;
    }

    // Some standalone Linux toolchain releases (e.g. GHDL's own tarballs,
    // which bundle a GNAT Ada runtime) ship their shared libraries in a
    // lib/ directory next to bin/ and rely on their own launcher script to
    // put both on the loader's search path. Pointing Settings straight at
    // bin/ghdl skips that script, so the binary can fail to even start
    // unless we add lib/ back onto PATH ourselves - same class of bug as
    // KNOWN_ISSUES #22 (Yosys/libffi), just on the GHDL/Icarus side.
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

            // On Linux the dynamic loader consults LD_LIBRARY_PATH directly,
            // not PATH - PATH alone (the Windows/Yosys fix) doesn't help here.
            wxString existingLdPath;
            wxArrayString staleLdKeys;
            for (const auto& kv : env.env)
                if (kv.first.CmpNoCase("LD_LIBRARY_PATH") == 0)
                {
                    existingLdPath = kv.second;
                    staleLdKeys.Add(kv.first);
                }
            for (const wxString& k : staleLdKeys)
                env.env.erase(k);

            env.env["LD_LIBRARY_PATH"] = libDir
                                        + (existingLdPath.IsEmpty() ? wxString()
                                                                     : wxPATH_SEP + existingLdPath);
            needEnv = true;
        }
    }

    return (int)wxExecute(cmd, output, errors, wxEXEC_SYNC, needEnv ? &env : nullptr);
}

bool CircuitSimulator::CompileHDL(const wxString& filePath,
                                   wxArrayString& output,
                                   wxArrayString& errors)
{
    if (filePath.IsEmpty() && m_projectFiles.IsEmpty())
    {
        errors.Add("No file is open. Open a VHDL file and try again.");
        return false;
    }

    // If the project supplied a full ordered file list, analyze all of them
    // in one GHDL call (dependencies before testbenches).
    // Otherwise fall back to the single open file.
    wxString cmd = "\"" + ghdlPath + "\" -a --std=" + m_vhdlStd + GHDLWorkdirFlag();

    if (!m_projectFiles.IsEmpty())
    {
        for (size_t i = 0; i < m_projectFiles.GetCount(); ++i)
        {
            // Normalize path and convert to forward slashes for GHDL (MinGW).
            wxFileName fn(m_projectFiles[i]);
            fn.Normalize(wxPATH_NORM_ALL);
            wxString fp = fn.GetFullPath();
            fp.Replace("\\", "/");
            cmd += " \"" + fp + "\"";
        }
        output.Add(wxString::Format("Analyzing %d project file(s)...",
                                    (int)m_projectFiles.GetCount()));
    }
    else
    {
        wxString fp = filePath;
        fp.Replace("\\", "/");
        cmd += " \"" + fp + "\"";
    }

    int exitCode = Execute(cmd, output, errors, ghdlPath);

    if (exitCode == 0)
        output.Add("Analysis complete.");

    return exitCode == 0;
}

bool CircuitSimulator::ElaborateHDL(const wxString& topEntity,
                                     wxArrayString& output,
                                     wxArrayString& errors)
{
    wxString cmd = "\"" + ghdlPath + "\" -e --std=" + m_vhdlStd + GHDLWorkdirFlag() + " " + topEntity;
    int exitCode = Execute(cmd, output, errors, ghdlPath);
    if (exitCode == 0)
        output.Add("Elaboration complete: " + topEntity);
    return exitCode == 0;
}

bool CircuitSimulator::RunHDL(const wxString& topEntity,
                               const wxString& vcdPath,
                               wxArrayString& output,
                               wxArrayString& errors)
{
    wxString cmd = "\"" + ghdlPath + "\" -r --std=" + m_vhdlStd + GHDLWorkdirFlag() + " " + topEntity;
    if (!vcdPath.IsEmpty())
    {
        wxString vp = vcdPath;
        vp.Replace("\\", "/");
        cmd += " --vcd=\"" + vp + "\"";
    }
    if (!m_stopTime.IsEmpty())
        cmd += " --stop-time=" + m_stopTime;
    int exitCode = Execute(cmd, output, errors, ghdlPath);
    if (exitCode == 0)
    {
        output.Add("Simulation complete: " + topEntity);
        if (!vcdPath.IsEmpty())
            output.Add("Waveform written: " + vcdPath);
    }
    return exitCode == 0;
}

bool CircuitSimulator::RunSimulation(const wxString& filePath,
                                      const wxString& topEntity,
                                      wxArrayString& output,
                                      wxArrayString& errors)
{
    // Analyze
    if (!CompileHDL(filePath, output, errors))
        return false;

    // Elaborate
    wxString elab = "\"" + ghdlPath + "\" -e --std=" + m_vhdlStd + GHDLWorkdirFlag() + " " + topEntity;
    if (Execute(elab, output, errors, ghdlPath) != 0)
        return false;

    // Run
    wxString run = "\"" + ghdlPath + "\" -r --std=" + m_vhdlStd + GHDLWorkdirFlag() + " " + topEntity;
    if (!m_stopTime.IsEmpty())
        run += " --stop-time=" + m_stopTime;
    int exitCode = Execute(run, output, errors, ghdlPath);

    if (exitCode == 0)
        output.Add("Simulation complete: " + topEntity);

    return exitCode == 0;
}

bool CircuitSimulator::RunSimulationWithVCD(const wxString& filePath,
                                             const wxString& topEntity,
                                             const wxString& vcdOutputPath,
                                             wxArrayString& output,
                                             wxArrayString& errors)
{
    // Analyze
    if (!CompileHDL(filePath, output, errors))
        return false;

    // Elaborate
    wxString elab = "\"" + ghdlPath + "\" -e --std=" + m_vhdlStd + GHDLWorkdirFlag() + " " + topEntity;
    if (Execute(elab, output, errors, ghdlPath) != 0)
        return false;

    // Run with VCD output
    wxString vp = vcdOutputPath;
    vp.Replace("\\", "/");
    wxString run = "\"" + ghdlPath + "\" -r --std=" + m_vhdlStd + GHDLWorkdirFlag() + " " + topEntity
                 + " --vcd=\"" + vp + "\"";
    if (!m_stopTime.IsEmpty())
        run += " --stop-time=" + m_stopTime;
    int exitCode = Execute(run, output, errors, ghdlPath);

    if (exitCode == 0)
    {
        output.Add("Simulation complete: " + topEntity);
        output.Add("Waveform written: " + vcdOutputPath);
    }

    return exitCode == 0;
}

bool CircuitSimulator::DebugSimulation(const wxString& filePath,
                                        wxArrayString& output,
                                        wxArrayString& errors)
{
    errors.Add("Use Tools > Run Simulation to generate a .vcd waveform file.");
    return false;
}

bool CircuitSimulator::SyntaxCheckVHDL(const wxString& filePath,
                                        wxArrayString& errors)
{
    if (filePath.IsEmpty())
    {
        errors.Add("No file path provided for syntax check.");
        return false;
    }

    wxArrayString out;
    wxString fp = filePath;
    fp.Replace("\\", "/");
    wxString cmd = "\"" + ghdlPath + "\" -s --std=" + m_vhdlStd + GHDLWorkdirFlag() + " \"" + fp + "\"";
    int exitCode = Execute(cmd, out, errors, ghdlPath);
    return exitCode == 0;
}

//--
// Icarus Verilog
//--
bool CircuitSimulator::CompileVerilog(const wxString& filePath,
                                       wxArrayString& output,
                                       wxArrayString& errors)
{
    if (filePath.IsEmpty() && m_projectFiles.IsEmpty())
    {
        errors.Add("No file is open. Open a Verilog file and try again.");
        return false;
    }

    // Use the primary file for naming the output .vvp
    wxString primaryFile = filePath.IsEmpty() ? m_projectFiles[0] : filePath;
    wxString stem    = wxFileName(primaryFile).GetName();
    wxString outFile = (!workDir.IsEmpty() ? workDir + "/" : "") + stem + ".vvp";

    // iverilog -o <stem>.vvp [all project .v/.sv files] or single file
    wxString cmd = "\"" + icarusPath + "\" -o \"" + outFile + "\"";
    if (!m_projectFiles.IsEmpty())
    {
        output.Add(wxString::Format("Compiling %d Verilog file(s)...",
                                    (int)m_projectFiles.GetCount()));
        for (size_t i = 0; i < m_projectFiles.GetCount(); ++i)
            cmd += " \"" + m_projectFiles[i] + "\"";
    }
    else
    {
        cmd += " \"" + filePath + "\"";
    }

    int exitCode = Execute(cmd, output, errors, icarusPath);

    if (exitCode == 0)
        output.Add("Verilog compilation successful: " + outFile);

    return exitCode == 0;
}

bool CircuitSimulator::RunVVP(const wxString& vvpPath,
                               wxArrayString& output,
                               wxArrayString& errors)
{
    // vvp ships alongside iverilog in the same bin/ - derive lib/ from
    // icarusPath even though the literal command run here is "vvp".
    int exitCode = Execute("vvp \"" + vvpPath + "\"", output, errors, icarusPath);
    if (exitCode == 0)
        output.Add("Verilog simulation complete.");
    return exitCode == 0;
}

bool CircuitSimulator::RunVerilog(const wxString& filePath,
                                   wxArrayString& output,
                                   wxArrayString& errors)
{
    if (!CompileVerilog(filePath, output, errors))
        return false;

    wxString stem = wxFileName(filePath).GetName();
    wxString vvpFile = (!workDir.IsEmpty() ? workDir + "/" : "") + stem + ".vvp";

    // vvp <file.vvp>
    wxString cmd = "vvp \"" + vvpFile + "\"";
    int exitCode = Execute(cmd, output, errors, icarusPath);

    if (exitCode == 0)
        output.Add("Verilog simulation complete.");

    return exitCode == 0;
}

//--
// Verilator
//--
bool CircuitSimulator::LintVerilator(const wxString& filePath,
                                      wxArrayString& output,
                                      wxArrayString& errors)
{
    if (filePath.IsEmpty())
    {
        errors.Add("No file is open. Open a Verilog file and try again.");
        return false;
    }

    wxString cmd = "\"" + verilatorPath + "\" --lint-only -Wall \"" + filePath + "\"";
    int exitCode = Execute(cmd, output, errors, verilatorPath);

    if (exitCode == 0)
        output.Add("Verilator lint passed: " + wxFileName(filePath).GetFullName());

    return exitCode == 0;
}

bool CircuitSimulator::RunVerilator(const wxString& filePath,
                                     wxArrayString& output,
                                     wxArrayString& errors)
{
    if (filePath.IsEmpty())
    {
        errors.Add("No file is open. Open a Verilog file and try again.");
        return false;
    }

    wxString stem    = wxFileName(filePath).GetName();
    wxString objDir  = (!workDir.IsEmpty() ? workDir + "/" : "") + "obj_dir";
    wxString exeName = objDir + "/V" + stem;

    // verilator --binary -j 0 builds an executable directly (Verilator 5.x+)
    wxString buildCmd = "\"" + verilatorPath + "\" --binary -j 0"
                        + " --Mdir \"" + objDir + "\""
                        + " \"" + filePath + "\"";
    int exitCode = Execute(buildCmd, output, errors, verilatorPath);

    if (exitCode != 0)
    {
        errors.Add("Verilator build failed. Check that Verilator 5.x is installed.");
        return false;
    }

    output.Add("Verilator build succeeded. Running simulation...");

    // Run the generated executable (our own build output, not verilatorPath
    // itself - no lib/ derivation needed here).
    int runCode = Execute("\"" + exeName + "\"", output, errors);

    if (runCode == 0)
        output.Add("Verilator simulation complete.");

    return runCode == 0;
}
