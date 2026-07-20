#include <wx/wx.h>
#include <wx/splash.h>
#include <wx/dcmemory.h>
#include <wx/filename.h>
#include <wx/stopwatch.h>
#include "ui/mainwindow/mainwindow.h"
#include "core/version.h"
#include "mdap/wxMaterialDesignArtProvider.hpp"

//- - - - - -
// BuildSplashBitmap - drawn programmatically; no external image file needed.
//- - - - - -


static wxBitmap BuildSplashBitmap()
{
    const int W = 520, H = 300;
    wxBitmap bmp(W, H, 24);
    wxMemoryDC dc(bmp);

    // Background
    dc.SetBackground(wxBrush(wxColour(30, 30, 30)));
    dc.Clear();

    // Top accent bar
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(55, 148, 255)));   // OCX blue
    dc.DrawRectangle(0, 0, W, 5);

    // Circuit-trace decorative lines (faint)
    dc.SetPen(wxPen(wxColour(55, 148, 255, 40), 1));
    for (int x = 40; x < W; x += 60)
        dc.DrawLine(x, 5, x, 30);
    for (int x = 40; x < W; x += 60)
        dc.DrawLine(x, 30, x + 20, 30);

    // App title
    wxFont titleFont(wxFontInfo(28).Bold().FaceName("Segoe UI").AntiAliased(true));
    dc.SetFont(titleFont);
    dc.SetTextForeground(wxColour(255, 255, 255));
    dc.DrawText("OpenCircuitX", 36, 60);

    // Subtitle
    wxFont subFont(wxFontInfo(11).FaceName("Segoe UI").AntiAliased(true));
    dc.SetFont(subFont);
    dc.SetTextForeground(wxColour(160, 160, 160));
    dc.DrawText("Open-source EDA Platform for digital hardware design", 38, 108);

    // Horizontal separator
    dc.SetPen(wxPen(wxColour(60, 60, 60), 1));
    dc.DrawLine(36, 140, W - 36, 140);

    // Feature bullets
    wxFont featFont(wxFontInfo(10).FaceName("Segoe UI").AntiAliased(true));
    dc.SetFont(featFont);
    dc.SetTextForeground(wxColour(130, 200, 130));

    const wxString features[] = {
        "HDL Editor  |  VHDL + Verilog + SystemVerilog",
        "Waveform Viewer  |  VCD / GHDL / Icarus / Verilator",
        "Circuit Canvas  |  Visual Animation  |  FPGA Toolchain",
    };
    int y = 158;
    for (const wxString& f : features)
    {
        dc.DrawText(f, 38, y);
        y += 22;
    }

    // Version - bottom left
    wxFont verFont(wxFontInfo(9).FaceName("Segoe UI").AntiAliased(true));
    dc.SetFont(verFont);
    dc.SetTextForeground(wxColour(90, 90, 90));
    dc.DrawText(wxString("v") + OCX_VERSION_STRING, 38, H - 30);

    // Branding - bottom right
    dc.SetTextForeground(wxColour(90, 90, 90));
    wxString brand = "Built by OpenLabX";
    wxSize   sz    = dc.GetTextExtent(brand);
    dc.DrawText(brand, W - sz.GetWidth() - 36, H - 30);

    // Bottom accent bar
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(45, 45, 45)));
    dc.DrawRectangle(0, H - 42, W, 1);

    dc.SelectObject(wxNullBitmap);
    return bmp;
}

// - - - - - -
// Application class
// - - - - - -

class OpenCircuitXApp : public wxApp
{
public:
    virtual bool OnInit();
};

bool OpenCircuitXApp::OnInit()
{
    wxInitAllImageHandlers();   // registers PNG, JPEG, BMP, etc. - required before any SaveFile()
    wxArtProvider::Push(new wxMaterialDesignArtProvider());

    // Minimum time the splash stays visible (ms).
    // On a slow machine the main window may take longer - splash
    // will stay until the window is ready regardless.
    static const long MIN_SPLASH_MS = 1400;

    wxStopWatch sw;

    wxBitmap bmp = BuildSplashBitmap();

    // NO_TIMEOUT: we close the splash ourselves at exactly the right moment
    // instead of letting a dumb timer fire while the main window is already up.
    wxSplashScreen* splashWin = new wxSplashScreen(
        bmp,
        wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT,
        0, nullptr, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxBORDER_NONE | wxSTAY_ON_TOP);

    wxYield();  // let the splash actually paint before we start building the UI

    MainWindow* mainWin = new MainWindow(wxT("OpenCircuitX"));
    mainWin->Show(true);
    wxYield();  // flush all pending paint events so the main window is fully drawn

    // Enforce the minimum display time without blocking the event loop.
    // wxYield() inside the loop keeps both windows responsive while we wait.
    long remaining = MIN_SPLASH_MS - (long)sw.Time();
    while (remaining > 0)
    {
        wxYield();
        wxMilliSleep(16);   // ~1 frame; short enough to stay responsive
        remaining = MIN_SPLASH_MS - (long)sw.Time();
    }

    // Now the main window is painted AND the minimum time has elapsed - 
    // destroy the splash and bring the main window to front.
    splashWin->Destroy();
    mainWin->Raise();
    mainWin->SetFocus();

    // If launched by double-clicking a .ocxproj file (or via file association)
    // argv[1] will be the project path.
    if (argc > 1)
    {
        wxString arg = argv[1];
        if (wxFileExists(arg) && wxFileName(arg).GetExt().Lower() == "ocxproj")
            mainWin->OpenProjectFile(arg);
    }

    return true;
}

wxIMPLEMENT_APP(OpenCircuitXApp);
