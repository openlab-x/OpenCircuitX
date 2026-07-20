#include "app_theme.h"

OCXThemeId OCXTheme::s_theme = OCXThemeId::Dark;

void OCXTheme::Set(OCXThemeId id) { s_theme = id; }
OCXThemeId OCXTheme::Get()        { return s_theme; }

wxColour OCXTheme::BgApp()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 13,  17,  23);
    case OCXThemeId::Light:    return wxColour(245, 245, 245);
    default:                   return wxColour( 30,  30,  30);
    }
}

wxColour OCXTheme::BgPanel()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 22,  27,  34);
    case OCXThemeId::Light:    return wxColour(236, 236, 236);
    default:                   return wxColour( 37,  37,  38);
    }
}

wxColour OCXTheme::BgEditor()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 13,  17,  23);
    case OCXThemeId::Light:    return wxColour(255, 255, 255);
    default:                   return wxColour( 30,  30,  30);
    }
}

wxColour OCXTheme::BgMargin()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 22,  27,  34);
    case OCXThemeId::Light:    return wxColour(240, 240, 240);
    default:                   return wxColour( 37,  37,  38);
    }
}

wxColour OCXTheme::BgOutput()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(  1,   4,   9);
    case OCXThemeId::Light:    return wxColour(250, 250, 250);
    default:                   return wxColour( 20,  20,  20);
    }
}

wxColour OCXTheme::BgLineCur()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 28,  33,  40);
    case OCXThemeId::Light:    return wxColour(240, 240, 255);
    default:                   return wxColour( 40,  40,  40);
    }
}

wxColour OCXTheme::BgSelection()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 31,  61,  92);
    case OCXThemeId::Light:    return wxColour(173, 214, 255);
    default:                   return wxColour( 38,  79, 120);
    }
}

wxColour OCXTheme::BgSash()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 48,  54,  61);
    case OCXThemeId::Light:    return wxColour(204, 204, 204);
    default:                   return wxColour( 60,  60,  60);
    }
}

wxColour OCXTheme::BgButton()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 33,  38,  45);
    case OCXThemeId::Light:    return wxColour(220, 220, 220);
    default:                   return wxColour( 50,  50,  58);
    }
}

wxColour OCXTheme::FgText()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(230, 237, 243);
    case OCXThemeId::Light:    return wxColour( 30,  30,  30);
    default:                   return wxColour(212, 212, 212);
    }
}

wxColour OCXTheme::FgDim()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(125, 133, 144);
    case OCXThemeId::Light:    return wxColour(113, 113, 113);
    default:                   return wxColour(133, 133, 133);
    }
}

wxColour OCXTheme::FgCaret()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(230, 237, 243);
    case OCXThemeId::Light:    return wxColour(  0,   0,   0);
    default:                   return wxColour(255, 255, 255);
    }
}

wxColour OCXTheme::Accent()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour( 31, 111, 235);
    case OCXThemeId::Light:    return wxColour(  0,  95, 184);
    default:                   return wxColour(  0, 122, 204);
    }
}

wxColour OCXTheme::SynComment()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(139, 148, 158);
    case OCXThemeId::Light:    return wxColour(  0, 128,   0);
    default:                   return wxColour(106, 153,  85);
    }
}

wxColour OCXTheme::SynString()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(165, 214, 255);
    case OCXThemeId::Light:    return wxColour(163,  21,  21);
    default:                   return wxColour(206, 145, 120);
    }
}

wxColour OCXTheme::SynKeyword()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(255, 123, 114);
    case OCXThemeId::Light:    return wxColour(  0,   0, 255);
    default:                   return wxColour( 86, 156, 214);
    }
}

wxColour OCXTheme::SynType()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(126, 231, 135);
    case OCXThemeId::Light:    return wxColour( 38, 127, 153);
    default:                   return wxColour( 78, 201, 176);
    }
}

wxColour OCXTheme::SynFunction()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(210, 168, 255);
    case OCXThemeId::Light:    return wxColour(121,  94,  38);
    default:                   return wxColour(220, 220, 170);
    }
}

wxColour OCXTheme::SynNumber()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(121, 192, 255);
    case OCXThemeId::Light:    return wxColour(  9, 134,  88);
    default:                   return wxColour(181, 206, 168);
    }
}

wxColour OCXTheme::SynPreproc()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(255, 166,  87);
    case OCXThemeId::Light:    return wxColour(175,   0, 219);
    default:                   return wxColour(197, 134, 192);
    }
}

wxColour OCXTheme::SynOperator()
{
    switch (s_theme) {
    case OCXThemeId::Midnight: return wxColour(230, 237, 243);
    case OCXThemeId::Light:    return wxColour( 30,  30,  30);
    default:                   return wxColour(212, 212, 212);
    }
}

wxString OCXTheme::FontFace()       { return "Consolas"; }
int      OCXTheme::FontSize()       { return 11; }
wxString OCXTheme::EditorFontFace() { return "Consolas"; }
int      OCXTheme::EditorFontSize() { return 11; }
