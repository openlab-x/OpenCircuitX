#include "welcome_panel.h"
#include "ui/shell/app_theme.h"
#include "core/version.h"
#include <wx/filename.h>
#include <wx/dcbuffer.h>
#include <wx/statline.h>
#include <wx/tokenzr.h>

//--
// DomainCard - one of three equal-width cards in the horizontal domain row
//--
namespace
{

static const int CARD_H   = 90;
static const int ACCENT_W = 4;
static const int LEFT_PAD = 14;

class DomainCard : public wxPanel
{
public:
    DomainCard(wxWindow* parent,
               const wxString& title,
               const wxString& desc,
               const wxString& badge,
               const wxColour& accent,
               bool enabled,
               bool drawRightBorder = false)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, CARD_H), wxBORDER_NONE)
        , m_title(title), m_desc(desc), m_badge(badge)
        , m_accent(accent), m_enabled(enabled), m_drawRightBorder(drawRightBorder)
    {
        SetMinSize(wxSize(120, CARD_H));
        SetMaxSize(wxSize(-1,  CARD_H));
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &DomainCard::OnPaint, this);
    }

private:
    wxString m_title, m_desc, m_badge;
    wxColour m_accent;
    bool     m_enabled         = true;
    bool     m_drawRightBorder = false;

    void OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        wxSize sz = GetSize();

        const wxColour bg = OCXTheme::BgEditor();
        dc.SetBackground(wxBrush(bg));
        dc.Clear();

        // Border: top, bottom, left always; right only on the last card so adjacent
        // cards share one clean 1px divider instead of a double-line + gap.
        dc.SetPen(wxPen(OCXTheme::BgSash(), 1));
        dc.DrawLine(0,       0,       sz.x, 0);        // top
        dc.DrawLine(0,       sz.y-1,  sz.x, sz.y-1);   // bottom
        dc.DrawLine(0,       0,       0,    sz.y);      // left
        if (m_drawRightBorder)
            dc.DrawLine(sz.x-1, 0, sz.x-1, sz.y);      // right (last card only)

        // Left accent bar
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(m_enabled ? m_accent : wxColour(55, 55, 60)));
        dc.DrawRectangle(0, 0, ACCENT_W, sz.y);

        const int tx     = LEFT_PAD + ACCENT_W + 4;
        const int titleY = 18;

        // Title (11pt bold)
        dc.SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                          false, OCXTheme::EditorFontFace()));
        dc.SetTextForeground(m_enabled ? OCXTheme::FgText() : OCXTheme::FgDim());
        dc.DrawText(m_title, tx, titleY);
        wxSize titleSz = dc.GetTextExtent(m_title);

        // Badge - drawn inline, right after the title text
        dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                          false, OCXTheme::EditorFontFace()));
        const wxString badgeText = m_badge.Strip();
        wxSize bsz = dc.GetTextExtent(badgeText);
        const int GAP = 10, PAD_H = 5, PAD_V = 2;
        int bx = tx + titleSz.x + GAP;
        int by = titleY + (titleSz.y - bsz.y) / 2 - PAD_V;

        wxColour bt = m_enabled
            ? m_accent
            : wxColour((int)(m_accent.Red()  *0.55),
                       (int)(m_accent.Green()*0.55),
                       (int)(m_accent.Blue() *0.55));
        wxColour pillBg(
            (int)(bg.Red()  *0.82 + bt.Red()  *0.18),
            (int)(bg.Green()*0.82 + bt.Green()*0.18),
            (int)(bg.Blue() *0.82 + bt.Blue() *0.18));
        dc.SetPen(wxPen(bt, 1));
        dc.SetBrush(wxBrush(pillBg));
        dc.DrawRoundedRectangle(bx - PAD_H, by, bsz.x + PAD_H * 2, bsz.y + PAD_V * 2, 3);
        dc.SetTextForeground(bt);
        dc.DrawText(badgeText, bx, by + PAD_V);

        // Description - 8pt, word-wrapped to fill the card width
        dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                          false, OCXTheme::EditorFontFace()));
        dc.SetTextForeground(OCXTheme::FgDim());
        const int maxW  = sz.x - tx - 10;
        const int lineH = dc.GetTextExtent("Ag").y + 3;
        int       dY    = titleY + titleSz.y + 10;
        wxString  line;
        wxStringTokenizer tok(m_desc, " ");
        while (tok.HasMoreTokens())
        {
            wxString word = tok.GetNextToken();
            if (word.IsEmpty()) continue;
            wxString test = line.IsEmpty() ? word : (line + " " + word);
            if (!line.IsEmpty() && dc.GetTextExtent(test).x > maxW)
            {
                dc.DrawText(line, tx, dY);
                dY  += lineH;
                line = word;
            }
            else
            {
                line = test;
            }
        }
        if (!line.IsEmpty())
            dc.DrawText(line, tx, dY);
    }
};

//--
// ActionButton - styled flat button
//--
class ActionButton : public wxPanel
{
public:
    ActionButton(wxWindow* parent, const wxString& label, bool primary,
                 std::function<void()> onClick)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 44), wxBORDER_NONE)
        , m_label(label), m_primary(primary), m_onClick(onClick)
    {
        SetMinSize(wxSize(80, 44));
        SetMaxSize(wxSize(-1, 44));
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));
        Bind(wxEVT_PAINT,        &ActionButton::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN,    [this](wxMouseEvent&) { if (m_onClick) m_onClick(); });
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) { m_hovered = true;  Refresh(); e.Skip(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e) { m_hovered = false; Refresh(); e.Skip(); });
    }
private:
    wxString m_label;
    bool m_primary = false;
    bool m_hovered = false;
    std::function<void()> m_onClick;

    void OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        wxSize sz = GetSize();
        dc.SetBackground(wxBrush(OCXTheme::BgEditor()));
        dc.Clear();
        if (m_primary)
        {
            wxColour a = OCXTheme::Accent();
            wxColour fill = m_hovered
                ? wxColour(std::min(255, a.Red()   + 20),
                           std::min(255, a.Green() + 20),
                           std::min(255, a.Blue()  + 28))
                : a;
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(wxBrush(fill));
            dc.DrawRoundedRectangle(0, 0, sz.x, sz.y, 6);
            dc.SetTextForeground(*wxWHITE);
        }
        else
        {
            wxColour fill   = m_hovered ? OCXTheme::BgLineCur() : OCXTheme::BgEditor();
            wxColour border = m_hovered
                ? OCXTheme::FgText()
                : wxColour(OCXTheme::BgSash().Red()   + 30,
                           OCXTheme::BgSash().Green() + 30,
                           OCXTheme::BgSash().Blue()  + 30);
            dc.SetPen(wxPen(border, 1));
            dc.SetBrush(wxBrush(fill));
            dc.DrawRoundedRectangle(0, 0, sz.x, sz.y, 6);
            dc.SetTextForeground(OCXTheme::FgText());
        }
        dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                          false, OCXTheme::EditorFontFace()));
        wxSize tsz = dc.GetTextExtent(m_label);
        dc.DrawText(m_label, (sz.x - tsz.x) / 2, (sz.y - tsz.y) / 2);
    }
};

} // namespace

//--
// Helper - styled static label
//--
static wxStaticText* MkLabel(wxWindow* parent, const wxString& text,
                              int ptSize = 9, bool bold = false,
                              wxColour col = wxNullColour)
{
    auto* lbl = new wxStaticText(parent, wxID_ANY, text);
    wxFont f(ptSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
             bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
             false, OCXTheme::EditorFontFace());
    lbl->SetFont(f);
    lbl->SetBackgroundColour(parent->GetBackgroundColour());
    lbl->SetForegroundColour(col.IsOk() ? col : OCXTheme::FgDim());
    return lbl;
}

//--
// WelcomePanel
//--
WelcomePanel::WelcomePanel(wxWindow* parent,
                           std::function<void()> onNewProject,
                           std::function<void()> onOpenProject,
                           std::function<void(const wxString&)> onOpenRecent)
    : wxPanel(parent, wxID_ANY)
    , m_onNewProject(onNewProject)
    , m_onOpenProject(onOpenProject)
    , m_onOpenRecent(onOpenRecent)
{
    SetBackgroundColour(OCXTheme::BgEditor());

    // Re-layout on every resize so stretch spacers redistribute correctly.
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Layout(); e.Skip(); });

    //** Content container **//
    m_content = new wxPanel(this, wxID_ANY);
    m_content->SetBackgroundColour(OCXTheme::BgEditor());

    // Title block (centered)
    m_appName = MkLabel(m_content, "OpenCircuitX", 28, true, OCXTheme::FgText());
    m_verLine = MkLabel(m_content,
        wxString("v") + OCX_VERSION_STRING + "     \xB7     EDA Platform");

    wxBoxSizer* titleSz = new wxBoxSizer(wxVERTICAL);
    titleSz->Add(m_appName, 0, wxALIGN_CENTER_HORIZONTAL);
    titleSz->Add(m_verLine, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 5);

    // Domain cards (3-column horizontal row - equal widths)
    m_domLbl = MkLabel(m_content, "Design domains:", 9, true);

    auto* cardDigital = new DomainCard(m_content,
        "Digital Design",
        "VHDL / Verilog  \xB7  Simulation  \xB7  FPGA synthesis",
        " Available ", wxColour(76, 175, 80), true,  false);

    auto* cardAnalog = new DomainCard(m_content,
        "Analog Design",
        "R / C / L components  \xB7  SPICE simulation",
        " Coming Soon ", wxColour(255, 152, 0), false, false);

    auto* cardMixed = new DomainCard(m_content,
        "Mixed-Signal",
        "Digital + analog co-simulation  \xB7  ADC / DAC",
        " Coming Soon ", wxColour(156, 39, 176), false, true);

    // Zero gap - adjacent card borders merge into one clean 1px divider.
    wxBoxSizer* cardsSz = new wxBoxSizer(wxHORIZONTAL);
    cardsSz->Add(cardDigital, 1, wxEXPAND);
    cardsSz->Add(cardAnalog,  1, wxEXPAND);
    cardsSz->Add(cardMixed,   1, wxEXPAND);

    // Separator between domain section and action buttons
    auto* sepH = new wxStaticLine(m_content, wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);

    // Action buttons (side by side, 3:2 proportion)
    auto* btnNew  = new ActionButton(m_content, "New Digital Project", true,  m_onNewProject);
    auto* btnOpen = new ActionButton(m_content, "Open Project...",     false, m_onOpenProject);

    wxBoxSizer* btnSz = new wxBoxSizer(wxHORIZONTAL);
    btnSz->Add(btnNew,  3, wxEXPAND | wxRIGHT, 8);
    btnSz->Add(btnOpen, 2, wxEXPAND);

    // Recent projects
    m_recLbl    = MkLabel(m_content, "Recent Projects", 9, true);
    m_recentBox = new wxPanel(m_content, wxID_ANY);
    m_recentBox->SetBackgroundColour(OCXTheme::BgEditor());
    m_recentSizer = new wxBoxSizer(wxVERTICAL);
    m_recentBox->SetSizer(m_recentSizer);

    // Single-column content layout
    wxBoxSizer* contentSz = new wxBoxSizer(wxVERTICAL);
    contentSz->Add(titleSz,     0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 24);
    contentSz->Add(m_domLbl,    0, wxBOTTOM, 8);
    contentSz->Add(cardsSz,     0, wxEXPAND | wxBOTTOM, 20);
    contentSz->Add(sepH,        0, wxEXPAND | wxBOTTOM, 16);
    contentSz->Add(btnSz,       0, wxEXPAND | wxBOTTOM, 20);
    contentSz->Add(m_recLbl,    0, wxBOTTOM, 6);
    contentSz->Add(m_recentBox, 0, wxEXPAND);
    m_content->SetSizer(contentSz);

    // Footer
    m_footer = MkLabel(this, "OpenCircuitX  \xB7  built by OpenLabX", 8);

    // Center content horizontally (12.5% margins on each side)
    wxBoxSizer* hCentre = new wxBoxSizer(wxHORIZONTAL);
    hCentre->AddStretchSpacer(1);
    hCentre->Add(m_content, 6, wxEXPAND);
    hCentre->AddStretchSpacer(1);

    // Root sizer: vertical centering via stretch spacers (1 top, 2 bottom)
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    root->AddStretchSpacer(1);
    root->Add(hCentre, 0, wxEXPAND | wxLEFT | wxRIGHT, 40);
    root->AddStretchSpacer(2);
    root->Add(m_footer, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 16);
    SetSizer(root);
}

//--
// ReapplyTheme
//--
void WelcomePanel::ReapplyTheme()
{
    const wxColour bg  = OCXTheme::BgEditor();
    const wxColour fg  = OCXTheme::FgText();
    const wxColour dim = OCXTheme::FgDim();

    SetBackgroundColour(bg);
    m_content->SetBackgroundColour(bg);
    m_recentBox->SetBackgroundColour(bg);

    auto applyLabel = [](wxStaticText* lbl, const wxColour& bg, const wxColour& fg) {
        lbl->SetBackgroundColour(bg);
        lbl->SetForegroundColour(fg);
        lbl->Refresh();
    };

    applyLabel(m_appName, bg, fg);
    applyLabel(m_verLine, bg, dim);
    applyLabel(m_domLbl,  bg, dim);
    applyLabel(m_recLbl,  bg, dim);
    applyLabel(m_footer,  bg, dim);

    for (wxWindow* row : m_recentBox->GetChildren())
        row->SetBackgroundColour(bg);

    Refresh();
    m_content->Refresh();
    m_recentBox->Refresh();
}

//--
// RefreshRecent
//--
void WelcomePanel::RefreshRecent(const wxArrayString& paths)
{
    m_recentSizer->Clear(true);

    if (paths.IsEmpty())
    {
        auto* none = MkLabel(m_recentBox, "No recent projects", 9, false);
        none->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC,
                             wxFONTWEIGHT_NORMAL, false, OCXTheme::EditorFontFace()));
        m_recentSizer->Add(none, 0);
    }
    else
    {
        const size_t maxShow = 5;
        for (size_t i = 0; i < paths.GetCount() && i < maxShow; ++i)
        {
            const wxString& path = paths[i];
            wxFileName fn(path);
            wxString   name = fn.GetName();
            wxString   dir  = fn.GetPath();
            if (dir.Length() > 40) dir = "..." + dir.Right(37);

            static const int ROW_H = 32;
            wxPanel* row = new wxPanel(m_recentBox, wxID_ANY,
                                       wxDefaultPosition, wxSize(-1, ROW_H));
            row->SetMinSize(wxSize(-1, ROW_H));
            row->SetMaxSize(wxSize(-1, ROW_H));
            row->SetBackgroundColour(OCXTheme::BgEditor());
            row->SetBackgroundStyle(wxBG_STYLE_PAINT);
            row->SetCursor(wxCursor(wxCURSOR_HAND));

            row->Bind(wxEVT_PAINT, [row, name, dir](wxPaintEvent&) {
                wxAutoBufferedPaintDC dc(row);
                wxSize sz = row->GetSize();
                dc.SetBackground(wxBrush(row->GetBackgroundColour()));
                dc.Clear();

                dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                  wxFONTWEIGHT_BOLD, false, OCXTheme::EditorFontFace()));
                dc.SetTextForeground(OCXTheme::FgText());
                dc.DrawText(name, 6, 3);

                dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                                  wxFONTWEIGHT_NORMAL, false, OCXTheme::EditorFontFace()));
                dc.SetTextForeground(OCXTheme::FgDim());
                dc.DrawText(dir, 6, 17);

                dc.SetPen(wxPen(OCXTheme::BgSash(), 1));
                dc.DrawLine(0, sz.y - 1, sz.x, sz.y - 1);
            });

            auto onEnter = [row](wxMouseEvent& e)
                { row->SetBackgroundColour(OCXTheme::BgLineCur()); row->Refresh(); e.Skip(); };
            auto onLeave = [row](wxMouseEvent& e)
                { row->SetBackgroundColour(OCXTheme::BgEditor()); row->Refresh(); e.Skip(); };
            auto onClick = [this, path](wxMouseEvent&)
                { if (m_onOpenRecent) m_onOpenRecent(path); };

            row->Bind(wxEVT_ENTER_WINDOW, onEnter);
            row->Bind(wxEVT_LEAVE_WINDOW, onLeave);
            row->Bind(wxEVT_LEFT_DOWN,    onClick);

            m_recentSizer->Add(row, 0, wxEXPAND | wxBOTTOM, 1);
        }
    }

    m_recentBox->Layout();
    Layout();
}
