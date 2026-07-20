#pragma once
#include <wx/wx.h>
#include <wx/arrstr.h>
#include <vector>

//--
// BoardProfile - describes a supported FPGA target
//--
struct BoardProfile
{
    wxString id;              // short key used in config (e.g. "ibreaker")
    wxString name;            // human-readable name shown in UI
    wxString family;          // "ice40" or "ecp5"
    wxString nextpnrDevice;   // nextpnr device flag   (e.g. "--up5k", "--25k")
    wxString nextpnrPkg;      // nextpnr package flag  (e.g. "--package sg48")
    wxString synthTarget;     // yosys synth command   (e.g. "synth_ice40", "synth_ecp5")
    wxString constraintFlag;  // "--pcf" (ice40) or "--lpf" (ecp5)
    wxString pnrOutExt;       // P&R output extension: ".asc" (ice40) or ".config" (ecp5)
    wxString bitExt;          // bitstream extension:  ".bin" (ice40) or ".bit"  (ecp5)
    int      lutTotal;        // device LUT capacity
    int      ffTotal;         // device FF capacity
    int      bramTotal;       // device BRAM capacity (blocks)
};

//--
// SynthReport - resource usage extracted from yosys output
//--
struct SynthReport
{
    int  luts   = -1;   // SB_LUT4 count  (-1 = not found)
    int  ffs    = -1;   // SB_DFF* count
    int  brams  = -1;   // SB_RAM40_4K count
    bool valid  = false;
};

//--
// FPGAToolchain - wraps yosys / nextpnr / icepack / openFPGALoader
//--
class FPGAToolchain
{
public:
    FPGAToolchain();

    // Tool path setters (empty string resets to default name on PATH)
    void SetYosysPath(const wxString& path);
    void SetNextpnrPath(const wxString& path);
    void SetIcepackPath(const wxString& path);
    void SetEcppackPath(const wxString& path);
    void SetNextpnrECP5Path(const wxString& path);
    void SetOpenFPGALoaderPath(const wxString& path);
    // Path to the ghdl-yosys-plugin (.so/.dll).
    // When set, VHDL files are synthesized via "yosys -m <plugin>".
    // Leave empty to attempt loading the plugin by name ("ghdl" on PATH).
    void SetGHDLPluginPath(const wxString& path);
    void SetWorkDir(const wxString& dir);

    // Supply ordered list of all project VHDL files (design first, tb last).
    // Used when synthesizing VHDL so all dependencies are analyzed together.
    void SetProjectFiles(const wxArrayString& files);

    wxString GetYosysPath()           const { return m_yosysPath;      }
    wxString GetNextpnrPath()         const { return m_nextpnrPath;    }
    wxString GetIcepackPath()         const { return m_icepackPath;      }
    wxString GetEcppackPath()         const { return m_ecppackPath;     }
    wxString GetNextpnrECP5Path()     const { return m_nextpnrECP5Path; }
    wxString GetOpenFPGALoaderPath()  const { return m_loaderPath;      }
    wxString GetGHDLPluginPath()      const { return m_ghdlPluginPath;  }

    bool IsYosysAvailable()          const;
    bool IsNextpnrAvailable()        const;
    bool IsIcepackAvailable()        const;
    bool IsOpenFPGALoaderAvailable() const;

    // Synthesize with yosys.
    // Produces <outDir>/<stem>.json.  Fills report with resource counts.
    bool Synthesize(const wxString& filePath,
                    const wxString& topEntity,
                    const wxString& outDir,
                    const BoardProfile& board,
                    wxArrayString& output,
                    wxArrayString& errors,
                    SynthReport& report);

    // Place & route with nextpnr-ice40.
    // Needs the .json from Synthesize.  Produces <outDir>/<stem>.asc.
    // pcfFile may be empty (nextpnr runs without constraints - placement only).
    bool PlaceAndRoute(const wxString& jsonFile,
                       const wxString& pcfFile,
                       const BoardProfile& board,
                       const wxString& outDir,
                       wxArrayString& output,
                       wxArrayString& errors);

    // Pack ASC → BIN with icepack.
    bool Pack(const wxString& ascFile,
              const wxString& outBinFile,
              wxArrayString& output,
              wxArrayString& errors);

    // Program board via openFPGALoader.
    bool Program(const wxString& bitstreamFile,
                 const BoardProfile& board,
                 wxArrayString& output,
                 wxArrayString& errors);

    // Full flow: Synth → P&R → Pack → (optionally) Program.
    bool RunFullFlow(const wxString& filePath,
                     const wxString& topEntity,
                     const wxString& pcfFile,
                     const BoardProfile& board,
                     const wxString& outDir,
                     bool doProgram,
                     wxArrayString& output,
                     wxArrayString& errors,
                     SynthReport& report);

    // Supported board list
    static const std::vector<BoardProfile>& GetBoards();

    // Format a SynthReport as a human-readable table string.
    static wxString FormatReport(const SynthReport& r, const BoardProfile& board);

private:
    wxString      m_yosysPath;
    wxString      m_nextpnrPath;       // nextpnr-ice40
    wxString      m_nextpnrECP5Path;   // nextpnr-ecp5
    wxString      m_icepackPath;       // icepack  (iCE40 packer)
    wxString      m_ecppackPath;       // ecppack  (ECP5 packer)
    wxString      m_loaderPath;
    wxString      m_workDir;
    wxString      m_ghdlPluginPath;
    wxArrayString m_projectFiles;

    int Execute(const wxString& cmd,
                wxArrayString& output,
                wxArrayString& errors) const;

    static SynthReport ParseYosysReport(const wxArrayString& lines);
};
