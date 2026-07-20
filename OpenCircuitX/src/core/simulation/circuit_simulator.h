#pragma once
#include <wx/string.h>
#include <wx/arrstr.h>

class CircuitSimulator
{
public:
    CircuitSimulator();

    void SetGHDLPath(const wxString& path);
    void SetIcarusPath(const wxString& path);
    void SetWorkDir(const wxString& dir);

    // Provide the full ordered list of VHDL source files to analyze when
    // compiling/running.  Dependencies must come before testbenches.
    // If empty, only the single filePath passed to CompileHDL is used.
    void SetProjectFiles(const wxArrayString& files);

    // Optional stop time passed to GHDL via --stop-time (e.g. "100ns", "1us").
    // Empty string = let GHDL run until the testbench finishes.
    void SetRunTime(const wxString& stopTime);

    // VHDL standard for GHDL (e.g. "08", "93", "19"). Defaults to "08".
    void SetVHDLStd(const wxString& std);

    bool IsGHDLAvailable() const;
    bool IsIcarusAvailable() const;

    bool CompileHDL(const wxString& filePath,
                    wxArrayString& output,
                    wxArrayString& errors);

    // Elaborate only (ghdl -e). Call after CompileHDL succeeds.
    bool ElaborateHDL(const wxString& topEntity,
                      wxArrayString& output,
                      wxArrayString& errors);

    // Run simulation step (ghdl -r). vcdPath may be empty for no VCD output.
    bool RunHDL(const wxString& topEntity,
                const wxString& vcdPath,
                wxArrayString& output,
                wxArrayString& errors);

    // Fast syntax-only check (ghdl -s) - does NOT produce a .o.
    // Runs on a single file; errors are returned in the errors array.
    bool SyntaxCheckVHDL(const wxString& filePath,
                         wxArrayString& errors);

    bool RunSimulation(const wxString& filePath,
                       const wxString& topEntity,
                       wxArrayString& output,
                       wxArrayString& errors);

    // Like RunSimulation but also writes a .vcd waveform file.
    // vcdOutputPath: full path for the output .vcd file.
    // Returns the generated vcd path on success (same as vcdOutputPath).
    bool RunSimulationWithVCD(const wxString& filePath,
                              const wxString& topEntity,
                              const wxString& vcdOutputPath,
                              wxArrayString& output,
                              wxArrayString& errors);

    bool DebugSimulation(const wxString& filePath,
                         wxArrayString& output,
                         wxArrayString& errors);

    // Icarus Verilog - compiles then runs a .v/.sv file.
    bool CompileVerilog(const wxString& filePath,
                        wxArrayString& output,
                        wxArrayString& errors);

    // Run a compiled .vvp file (vvp step only). Call after CompileVerilog.
    bool RunVVP(const wxString& vvpPath,
                wxArrayString& output,
                wxArrayString& errors);

    bool RunVerilog(const wxString& filePath,
                    wxArrayString& output,
                    wxArrayString& errors);

    // Verilator - lint and simulate Verilog/SystemVerilog.
    void SetVerilatorPath(const wxString& path);
    bool IsVerilatorAvailable() const;

    // Lint only: verilator --lint-only <file>
    bool LintVerilator(const wxString& filePath,
                       wxArrayString& output,
                       wxArrayString& errors);

    // Full run: verilator --binary -j 0 <file>, then execute the result.
    bool RunVerilator(const wxString& filePath,
                      wxArrayString& output,
                      wxArrayString& errors);

private:
    wxString      ghdlPath;
    wxString      icarusPath;
    wxString      verilatorPath;
    wxString      workDir;
    wxString      m_stopTime;
    wxString      m_vhdlStd;
    wxArrayString m_projectFiles;

    // Cached availability - only spawn --version once per session
    mutable int m_ghdlAvail    = -1;  // -1=unchecked, 0=missing, 1=found
    mutable int m_icarusAvail  = -1;
    mutable int m_verilatorAvail = -1;

    // Returns " --workdir=\"<path>\"" using forward slashes (GHDL/MinGW compatible).
    // Returns empty string when workDir is not set.
    wxString GHDLWorkdirFlag() const;

    int Execute(const wxString& cmd,
                wxArrayString& output,
                wxArrayString& errors) const;
};
