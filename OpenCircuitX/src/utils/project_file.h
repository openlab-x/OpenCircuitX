#pragma once
#include <wx/wx.h>
#include <wx/arrstr.h>

class ProjectFile
{
public:
    ProjectFile();
    void SetDefaults(const wxString& name);
    bool Load(const wxString& filePath);
    bool Save(const wxString& filePath);

    // [OpenCircuitX]
    wxString version;
    wxString projectName;
    wxString created;
    wxString lastModified;
    wxString author;
    wxString company;
    wxString description;
    wxString license;

    // [Language]
    wxString primaryLanguage;
    wxString vhdlStandard;
    wxString verilogStandard;
    bool     mixed;

    // [Files]
    wxArrayString sourceFiles;
    wxString      analyzeOrder;

    // [Libraries]
    wxString      workLibrary;
    wxArrayString externalLibs;

    // [Build]
    wxString topEntity;
    wxString topArchitecture;
    wxString outputDir;
    wxString workDir;

    // [Simulation]
    wxString simTopEntity;
    wxString simTopArchitecture;
    wxString runTime;
    wxString timeResolution;
    wxString waveFormat;
    wxString waveOutput;
    wxString simSaveFile;

    // [Schematic]
    wxArrayString schematicFiles;

    // [Constraints]
    wxArrayString constraintFiles;

    // [GHDL]
    wxString ghdlExecutablePath;
    wxString additionalAnalyzeFlags;
    wxString additionalRunFlags;
    bool     useIEEESynopsys;

    // [Editor]
    wxString lastOpenedFile;
    wxString openTabs;
    long     cursorLine;
    long     cursorColumn;
    wxString theme;

    // [Workspace]
    wxString workspaceFile;

    // [Notes]
    wxString notes;
    wxString tags;
};
