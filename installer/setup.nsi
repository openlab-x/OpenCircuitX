; OpenCircuitX Windows Installer
; Built with NSIS (Nullsoft Scriptable Install System)
; Copyright (c) 2026 OpenLabX, MIT License
;
; To build:
;   1. Install NSIS from https://nsis.sourceforge.io
;   2. Right-click this file > "Compile NSIS Script"
;   OR from command line:  makensis setup.nsi

;---------------------------------------------------------------------------
; General
;---------------------------------------------------------------------------

!define APP_NAME        "OpenCircuitX"
!define APP_VERSION     "1.1.0"
!define APP_PUBLISHER   "OpenLabX"
!define APP_URL         "https://openLabX.com"
!define APP_EXE         "OpenCircuitX.exe"
!define INSTALL_DIR     "$PROGRAMFILES64\${APP_NAME}"
!define REG_KEY         "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

; Installer name (shown in window title and MUI pages)
Name "${APP_NAME}"

; Output installer file
OutFile "OpenCircuitX-${APP_VERSION}-Setup.exe"

; Default installation folder
InstallDir "${INSTALL_DIR}"

; Request application privileges (needed for Program Files)
RequestExecutionLevel admin

; Modern UI
!include "MUI2.nsh"

;---------------------------------------------------------------------------
; Interface settings
;---------------------------------------------------------------------------

!define MUI_ABORTWARNING
!define MUI_ICON          "..\OpenCircuitX\res\app.ico"
!define MUI_UNICON        "..\OpenCircuitX\res\app.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE    "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

;---------------------------------------------------------------------------
; Installer
;---------------------------------------------------------------------------

Section "OpenCircuitX (required)" SecMain
    SectionIn RO

    SetOutPath "$INSTDIR"

    ; Main executable (x64 Release build)
    File "..\x64\Release\${APP_EXE}"

    ; wxWidgets runtime DLLs, only needed if built as DLL linkage.
    ; If you used static wxWidgets libs (vc_lib), remove these lines.
    ; File "..\x64\Release\wxmsw*.dll"

    ; App icon (for uninstaller shortcut)
    SetOutPath "$INSTDIR\res"
    File "..\OpenCircuitX\res\app.ico"

    ; LICENSE
    SetOutPath "$INSTDIR"
    File "..\LICENSE"

    ; Plugins directory, empty at install time; users/third-parties drop .dll files here
    CreateDirectory "$INSTDIR\plugins"

    ; Write uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Add/Remove Programs entry
    WriteRegStr HKLM "${REG_KEY}" "DisplayName"      "${APP_NAME}"
    WriteRegStr HKLM "${REG_KEY}" "DisplayVersion"   "${APP_VERSION}"
    WriteRegStr HKLM "${REG_KEY}" "Publisher"        "${APP_PUBLISHER}"
    WriteRegStr HKLM "${REG_KEY}" "URLInfoAbout"     "${APP_URL}"
    WriteRegStr HKLM "${REG_KEY}" "InstallLocation"  "$INSTDIR"
    WriteRegStr HKLM "${REG_KEY}" "UninstallString"  "$INSTDIR\Uninstall.exe"
    WriteRegDWORD HKLM "${REG_KEY}" "NoModify"       1
    WriteRegDWORD HKLM "${REG_KEY}" "NoRepair"       1

    ; ---------------------------------------------------------------------------
    ; File type associations
    ; ---------------------------------------------------------------------------

    ; .ocxproj, project file
    WriteRegStr HKCR ".ocxproj"                     "" "OpenCircuitX.Project"
    WriteRegStr HKCR "OpenCircuitX.Project"         "" "OpenCircuitX Project"
    WriteRegStr HKCR "OpenCircuitX.Project\DefaultIcon" "" "$INSTDIR\res\app.ico,0"
    WriteRegStr HKCR "OpenCircuitX.Project\shell\open\command" \
        "" '"$INSTDIR\${APP_EXE}" "%1"'

    ; .ocxschem, schematic file
    WriteRegStr HKCR ".ocxschem"                     "" "OpenCircuitX.Schematic"
    WriteRegStr HKCR "OpenCircuitX.Schematic"        "" "OpenCircuitX Schematic"
    WriteRegStr HKCR "OpenCircuitX.Schematic\shell\open\command" \
        "" '"$INSTDIR\${APP_EXE}" "%1"'

    ; Notify shell of file association changes
    System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'

    ; Start Menu shortcut
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
        "$INSTDIR\${APP_EXE}" "" "$INSTDIR\res\app.ico"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" \
        "$INSTDIR\Uninstall.exe"

    ; Desktop shortcut
    CreateShortcut "$DESKTOP\${APP_NAME}.lnk" \
        "$INSTDIR\${APP_EXE}" "" "$INSTDIR\res\app.ico"

SectionEnd

;---------------------------------------------------------------------------
; Uninstaller
;---------------------------------------------------------------------------

Section "Uninstall"

    ; Remove installed files
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\Uninstall.exe"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\res\app.ico"
    RMDir  "$INSTDIR\res"
    RMDir  "$INSTDIR\plugins"   ; only removed if empty (user plugins stay if present)
    RMDir  "$INSTDIR"

    ; Remove shortcuts
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\${APP_NAME}"
    Delete "$DESKTOP\${APP_NAME}.lnk"

    ; Remove file type associations
    DeleteRegKey HKCR ".ocxproj"
    DeleteRegKey HKCR "OpenCircuitX.Project"
    DeleteRegKey HKCR ".ocxschem"
    DeleteRegKey HKCR "OpenCircuitX.Schematic"
    System::Call 'Shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'

    ; Remove Add/Remove Programs entry
    DeleteRegKey HKLM "${REG_KEY}"

SectionEnd
