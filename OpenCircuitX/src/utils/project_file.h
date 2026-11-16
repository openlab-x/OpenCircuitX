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

    // Source file list helpers. Paths are compared after normalization, so
    // "dir\a.v" and "dir/a.v" count as the same file. Relative paths are
    // resolved against baseDir (the project directory).
    static wxString NormalizePath(const wxString& path, const wxString& baseDir);
    // Form a path is written to the project file in: relative with '/' when it
    // lies inside baseDir, unchanged otherwise.
    static wxString ToStoredPath(const wxString& path, const wxString& baseDir);
    int  FindSourceFile(const wxString& path, const wxString& baseDir) const;
    bool AddSourceFile(const wxString& path, const wxString& baseDir);
    void RemoveDuplicateSourceFiles(const wxString& baseDir);

    // Sets primaryLanguage and mixed from the extensions in sourceFiles.
    void UpdateLanguageFromFiles();

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
