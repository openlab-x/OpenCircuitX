#pragma once
#include <wx/wx.h>

//--
// OCXTheme - multi-theme colour and font system.
//
// Three built-in themes:
//   Dark - VS Code Dark+ palette (default)
//   Midnight - GitHub Dark palette (deep blue-black)
//   Light - VS Code Light palette (clean white)
//
// All UI files call OCXTheme::Xxx().  Switching themes at runtime and calling
// ApplyToFrame() updates every panel instantly.
//--
enum class OCXThemeId { Dark, Midnight, Light };

class OCXTheme
{
public:
    // ---- Theme selection --------------------------------------------------
    static void       Set(OCXThemeId id);
    static OCXThemeId Get();

    // ---- Backgrounds ------------------------------------------------------
    static wxColour BgApp();
    static wxColour BgPanel();
    static wxColour BgEditor();
    static wxColour BgMargin();
    static wxColour BgOutput();
    static wxColour BgLineCur();
    static wxColour BgSelection();
    static wxColour BgSash();
    static wxColour BgButton();

    // ---- Foregrounds ------------------------------------------------------
    static wxColour FgText();
    static wxColour FgDim();
    static wxColour FgCaret();

    // ---- Accent -----------------------------------------------------------
    static wxColour Accent();

    // ---- Syntax colours ---------------------------------------------------
    static wxColour SynComment();
    static wxColour SynString();
    static wxColour SynKeyword();
    static wxColour SynType();
    static wxColour SynFunction();
    static wxColour SynNumber();
    static wxColour SynPreproc();
    static wxColour SynOperator();

    // ---- Editor font ------------------------------------------------------
    static wxString FontFace();
    static int      FontSize();
    static wxString EditorFontFace();
    static int      EditorFontSize();

private:
    static OCXThemeId s_theme;
};
