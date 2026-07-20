#include "project_file.h"
#include <wx/fileconf.h>
#include <wx/wfstream.h>

ProjectFile::ProjectFile()
{
    SetDefaults("");
}

void ProjectFile::SetDefaults(const wxString& name)
{
    version              = "1.0";
    projectName          = name;
    created              = wxDateTime::Now().FormatISOCombined(' ');
    lastModified         = created;
    author               = "";
    company              = "";
    description          = "";
    license              = "";

    primaryLanguage      = "VHDL";
    vhdlStandard         = "2008";
    verilogStandard      = "2001";
    mixed                = false;

    analyzeOrder         = "";
    workLibrary          = "work";

    topEntity            = "";
    topArchitecture      = "Behavioral";
    outputDir            = "build";
    workDir              = "work";

    simTopEntity         = "";
    simTopArchitecture   = "Behavioral";
    runTime              = "1000ns";
    timeResolution       = "1ns";
    waveFormat           = "GHW";
    waveOutput           = name.IsEmpty() ? "sim_output.ocxwave" : name + ".ocxwave";
    simSaveFile          = name.IsEmpty() ? "simulation.ocxsim"  : name + ".ocxsim";

    ghdlExecutablePath   = "";
    additionalAnalyzeFlags = "";
    additionalRunFlags   = "";
    useIEEESynopsys      = false;

    lastOpenedFile       = "";
    openTabs             = "";
    cursorLine           = 1;
    cursorColumn         = 1;
    theme                = "dark";

    workspaceFile        = "";
    notes                = "";
    tags                 = "";
}

bool ProjectFile::Load(const wxString& filePath)
{
    wxFileInputStream input(filePath);
    if (!input.IsOk())
        return false;

    wxFileConfig cfg(input);

    // [OpenCircuitX]
    cfg.Read("OpenCircuitX/Version",      &version,      "1.0");
    cfg.Read("OpenCircuitX/ProjectName",  &projectName,  "");
    cfg.Read("OpenCircuitX/Created",      &created,      "");
    cfg.Read("OpenCircuitX/LastModified", &lastModified, "");
    cfg.Read("OpenCircuitX/Author",       &author,       "");
    cfg.Read("OpenCircuitX/Company",      &company,      "");
    cfg.Read("OpenCircuitX/Description",  &description,  "");
    cfg.Read("OpenCircuitX/License",      &license,      "");

    // [Language]
    cfg.Read("Language/Primary",         &primaryLanguage,  "VHDL");
    cfg.Read("Language/VHDLStandard",    &vhdlStandard,     "2008");
    cfg.Read("Language/VerilogStandard", &verilogStandard,  "2001");
    cfg.Read("Language/Mixed",           &mixed,            false);

    // [Files]
    sourceFiles.Clear();
    long count = 0;
    cfg.Read("Files/Count", &count, 0L);
    for (long i = 0; i < count; ++i)
    {
        wxString val;
        if (cfg.Read(wxString::Format("Files/File%ld", i), &val))
            sourceFiles.Add(val);
    }
    cfg.Read("Files/AnalyzeOrder", &analyzeOrder, "");

    // [Libraries]
    cfg.Read("Libraries/WorkLibrary", &workLibrary, "work");
    externalLibs.Clear();
    count = 0;
    cfg.Read("Libraries/ExternalCount", &count, 0L);
    for (long i = 0; i < count; ++i)
    {
        wxString val;
        if (cfg.Read(wxString::Format("Libraries/External%ld", i), &val))
            externalLibs.Add(val);
    }

    // [Build]
    cfg.Read("Build/TopEntity",       &topEntity,       "");
    cfg.Read("Build/TopArchitecture", &topArchitecture, "Behavioral");
    cfg.Read("Build/OutputDir",       &outputDir,       "build");
    cfg.Read("Build/WorkDir",         &workDir,         "work");

    // [Simulation]
    cfg.Read("Simulation/TopEntity",       &simTopEntity,       "");
    cfg.Read("Simulation/TopArchitecture", &simTopArchitecture, "Behavioral");
    cfg.Read("Simulation/RunTime",         &runTime,            "1000ns");
    cfg.Read("Simulation/TimeResolution",  &timeResolution,     "1ns");
    cfg.Read("Simulation/WaveFormat",      &waveFormat,         "GHW");
    cfg.Read("Simulation/WaveOutput",      &waveOutput,         "");
    cfg.Read("Simulation/SaveFile",        &simSaveFile,        "");

    // [Schematic]
    schematicFiles.Clear();
    count = 0;
    cfg.Read("Schematic/Count", &count, 0L);
    for (long i = 0; i < count; ++i)
    {
        wxString val;
        if (cfg.Read(wxString::Format("Schematic/File%ld", i), &val))
            schematicFiles.Add(val);
    }

    // [Constraints]
    constraintFiles.Clear();
    count = 0;
    cfg.Read("Constraints/Count", &count, 0L);
    for (long i = 0; i < count; ++i)
    {
        wxString val;
        if (cfg.Read(wxString::Format("Constraints/File%ld", i), &val))
            constraintFiles.Add(val);
    }

    // [GHDL]
    cfg.Read("GHDL/ExecutablePath",          &ghdlExecutablePath,    "");
    cfg.Read("GHDL/AdditionalAnalyzeFlags",  &additionalAnalyzeFlags,"");
    cfg.Read("GHDL/AdditionalRunFlags",      &additionalRunFlags,    "");
    cfg.Read("GHDL/UseIEEESynopsys",         &useIEEESynopsys,       false);

    // [Editor]
    cfg.Read("Editor/LastOpenedFile", &lastOpenedFile, "");
    cfg.Read("Editor/OpenTabs",       &openTabs,       "");
    cfg.Read("Editor/CursorLine",     &cursorLine,     1L);
    cfg.Read("Editor/CursorColumn",   &cursorColumn,   1L);
    cfg.Read("Editor/Theme",          &theme,          "dark");

    // [Workspace]
    cfg.Read("Workspace/WorkspaceFile", &workspaceFile, "");

    // [Notes]
    cfg.Read("Notes/Text", &notes, "");
    cfg.Read("Notes/Tags", &tags,  "");

    return true;
}

bool ProjectFile::Save(const wxString& filePath)
{
    lastModified = wxDateTime::Now().FormatISOCombined(' ');

    wxFileConfig cfg;

    // [OpenCircuitX]
    cfg.Write("OpenCircuitX/Version",      version);
    cfg.Write("OpenCircuitX/ProjectName",  projectName);
    cfg.Write("OpenCircuitX/Created",      created);
    cfg.Write("OpenCircuitX/LastModified", lastModified);
    cfg.Write("OpenCircuitX/Author",       author);
    cfg.Write("OpenCircuitX/Company",      company);
    cfg.Write("OpenCircuitX/Description",  description);
    cfg.Write("OpenCircuitX/License",      license);

    // [Language]
    cfg.Write("Language/Primary",         primaryLanguage);
    cfg.Write("Language/VHDLStandard",    vhdlStandard);
    cfg.Write("Language/VerilogStandard", verilogStandard);
    cfg.Write("Language/Mixed",           mixed);

    // [Files]
    cfg.Write("Files/Count", (long)sourceFiles.GetCount());
    for (long i = 0; i < (long)sourceFiles.GetCount(); ++i)
        cfg.Write(wxString::Format("Files/File%ld", i), sourceFiles[i]);
    cfg.Write("Files/AnalyzeOrder", analyzeOrder);

    // [Libraries]
    cfg.Write("Libraries/WorkLibrary",   workLibrary);
    cfg.Write("Libraries/ExternalCount", (long)externalLibs.GetCount());
    for (long i = 0; i < (long)externalLibs.GetCount(); ++i)
        cfg.Write(wxString::Format("Libraries/External%ld", i), externalLibs[i]);

    // [Build]
    cfg.Write("Build/TopEntity",       topEntity);
    cfg.Write("Build/TopArchitecture", topArchitecture);
    cfg.Write("Build/OutputDir",       outputDir);
    cfg.Write("Build/WorkDir",         workDir);

    // [Simulation]
    cfg.Write("Simulation/TopEntity",       simTopEntity);
    cfg.Write("Simulation/TopArchitecture", simTopArchitecture);
    cfg.Write("Simulation/RunTime",         runTime);
    cfg.Write("Simulation/TimeResolution",  timeResolution);
    cfg.Write("Simulation/WaveFormat",      waveFormat);
    cfg.Write("Simulation/WaveOutput",      waveOutput);
    cfg.Write("Simulation/SaveFile",        simSaveFile);

    // [Schematic]
    cfg.Write("Schematic/Count", (long)schematicFiles.GetCount());
    for (long i = 0; i < (long)schematicFiles.GetCount(); ++i)
        cfg.Write(wxString::Format("Schematic/File%ld", i), schematicFiles[i]);

    // [Constraints]
    cfg.Write("Constraints/Count", (long)constraintFiles.GetCount());
    for (long i = 0; i < (long)constraintFiles.GetCount(); ++i)
        cfg.Write(wxString::Format("Constraints/File%ld", i), constraintFiles[i]);

    // [GHDL]
    cfg.Write("GHDL/ExecutablePath",         ghdlExecutablePath);
    cfg.Write("GHDL/AdditionalAnalyzeFlags", additionalAnalyzeFlags);
    cfg.Write("GHDL/AdditionalRunFlags",     additionalRunFlags);
    cfg.Write("GHDL/UseIEEESynopsys",        useIEEESynopsys);

    // [Editor]
    cfg.Write("Editor/LastOpenedFile", lastOpenedFile);
    cfg.Write("Editor/OpenTabs",       openTabs);
    cfg.Write("Editor/CursorLine",     cursorLine);
    cfg.Write("Editor/CursorColumn",   cursorColumn);
    cfg.Write("Editor/Theme",          theme);

    // [Workspace]
    cfg.Write("Workspace/WorkspaceFile", workspaceFile);

    // [Notes]
    cfg.Write("Notes/Text", notes);
    cfg.Write("Notes/Tags", tags);

    wxFileOutputStream output(filePath);
    if (!output.IsOk())
        return false;

    cfg.Save(output);
    return true;
}
