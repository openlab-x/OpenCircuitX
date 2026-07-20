#include "ocx_menubar.h"
#include "app_theme.h"
#include <wx/dcbuffer.h>

static constexpr int HPAD = 14;  // horizontal text padding inside each entry

// Strip '&' mnemonic characters from a menu title
static wxString StripMnemonics(const wxString& s)
{
    wxString out;
    for (size_t i = 0; i < s.Length(); ++i)
    {
        if (s[i] == '&' && i + 1 < s.Length() && s[i + 1] != '&')
            continue; // skip single '&'
        else if (s[i] == '&' && i + 1 < s.Length() && s[i + 1] == '&')
        {
            out += '&'; ++i; // '&&' → literal '&'
        }
        else
            out += s[i];
    }
    return out;
}

OCXMenuBar::OCXMenuBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition,
              wxSize(-1, HEIGHT), wxBORDER_NONE)
{
    SetBackgroundColour(OCXTheme::BgPanel());
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    Bind(wxEVT_PAINT,       &OCXMenuBar::OnPaint,      this);
    Bind(wxEVT_MOTION,      &OCXMenuBar::OnMouseMove,  this);
    Bind(wxEVT_LEAVE_WINDOW,&OCXMenuBar::OnMouseLeave, this);
    Bind(wxEVT_LEFT_DOWN,   &OCXMenuBar::OnLeftDown,   this);
    Bind(wxEVT_SIZE,        &OCXMenuBar::OnSize,       this);
}

void OCXMenuBar::Append(wxMenu* menu, const wxString& title)
{
    Entry e;
    e.title = StripMnemonics(title);
    e.menu  = menu;
    m_entries.push_back(e);
    Refresh();
}

void OCXMenuBar::SetVersionInfo(const wxString& version, std::function<void()> onClick)
{
    m_version        = version;
    m_onIdentityClick = onClick;
    Refresh();
}

void OCXMenuBar::SetUpdateAvailable(const wxString& newVersion)
{
    m_updateVersion = newVersion;
    Refresh();
}

//--
// Layout
//--
void OCXMenuBar::RecalcLayout()
{
    // We need a DC to measure text - use a MemoryDC
    wxMemoryDC mdc;
    mdc.SetFont(GetFont());

    int x = 6;
    for (auto& e : m_entries)
    {
        wxSize ts = mdc.GetTextExtent(e.title);
        int w = ts.x + 2 * HPAD;
        e.rect = wxRect(x, 0, w, HEIGHT);
        x += w;
    }
}

int OCXMenuBar::HitTest(wxPoint pt) const
{
    for (int i = 0; i < (int)m_entries.size(); ++i)
        if (m_entries[i].rect.Contains(pt))
            return i;
    return -1;
}

void OCXMenuBar::ShowMenu(int idx)
{
    m_open = idx;
    Refresh();
    Update(); // force repaint before blocking in PopupMenu

    const Entry& e = m_entries[idx];
    // Position: bottom-left of the menu entry button
    wxPoint pt(e.rect.x, e.rect.GetBottom());
    PopupMenu(e.menu, pt);

    m_open    = -1;
    m_hovered = -1;
    Refresh();
}

//--
// Paint
//--
void OCXMenuBar::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize sz = GetClientSize();

    // Background - slightly darker strip than the toolbar
    wxColour bg = OCXTheme::BgApp();
    dc.SetBackground(wxBrush(bg));
    dc.Clear();

    // Bottom border
    dc.SetPen(wxPen(OCXTheme::BgSash()));
    dc.DrawLine(0, sz.y - 1, sz.x, sz.y - 1);

    RecalcLayout();

    dc.SetFont(GetFont());

    for (int i = 0; i < (int)m_entries.size(); ++i)
    {
        const Entry& e = m_entries[i];
        const wxRect& r = e.rect;
        bool isOpen    = (i == m_open);
        bool isHovered = (i == m_hovered && !isOpen);

        // Hover / open background
        if (isOpen)
        {
            wxColour openCol = OCXTheme::Accent().ChangeLightness(40);
            dc.SetBrush(wxBrush(openCol));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRoundedRectangle(r.x + 1, 2, r.width - 2, r.height - 3, 3);
        }
        else if (isHovered)
        {
            dc.SetBrush(wxBrush(OCXTheme::BgButton()));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRoundedRectangle(r.x + 1, 2, r.width - 2, r.height - 3, 3);
        }

        // Text - vertically centred
        wxSize ts = dc.GetTextExtent(e.title);
        int ty = (HEIGHT - ts.y) / 2;

        wxColour tc = isOpen ? OCXTheme::FgText()
                    : isHovered ? OCXTheme::FgText()
                    : OCXTheme::FgDim();
        dc.SetTextForeground(tc);
        dc.DrawText(e.title, r.x + HPAD, ty);
    }

    // Identity mark - app name + version, right-aligned; switches to a
    // clickable "update available" state when a newer version is found
    bool hasUpdate = !m_updateVersion.empty();

    wxFont idFont = GetFont();
    idFont.SetPointSize(wxMax(7, idFont.GetPointSize() - 1));
    if (hasUpdate) idFont.SetWeight(wxFONTWEIGHT_BOLD);
    dc.SetFont(idFont);

    wxString label = hasUpdate
        ? wxString::Format("Update available: v%s", m_updateVersion)
        : wxString::Format("OpenCircuitX  v%s", m_version);

    wxSize ls = dc.GetTextExtent(label);
    int idW = ls.x + 2 * HPAD;
    m_identityRect = wxRect(sz.x - idW - 4, 0, idW, HEIGHT);

    if (hasUpdate || m_identityHovered)
    {
        wxColour badgeCol = hasUpdate
            ? OCXTheme::Accent().ChangeLightness(m_identityHovered ? 120 : 100)
            : OCXTheme::BgButton();
        dc.SetBrush(wxBrush(badgeCol));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRoundedRectangle(m_identityRect.x + 1, 2, m_identityRect.width - 2, m_identityRect.height - 3, 3);
    }

    dc.SetTextForeground(hasUpdate ? wxColour(255, 255, 255) : OCXTheme::FgDim().ChangeLightness(60));
    dc.DrawText(label, m_identityRect.x + HPAD, (HEIGHT - ls.y) / 2);
}

//--
// Mouse
//--
void OCXMenuBar::OnMouseMove(wxMouseEvent& event)
{
    int idx = HitTest(event.GetPosition());
    if (idx != m_hovered)
    {
        m_hovered = idx;
        Refresh();
    }

    bool overIdentity = m_identityRect.Contains(event.GetPosition());
    if (overIdentity != m_identityHovered)
    {
        m_identityHovered = overIdentity;
        SetCursor(overIdentity ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
        Refresh();
    }
    event.Skip();
}

void OCXMenuBar::OnMouseLeave(wxMouseEvent& event)
{
    m_hovered = -1;
    if (m_identityHovered)
    {
        m_identityHovered = false;
        SetCursor(wxNullCursor);
    }
    Refresh();
    event.Skip();
}

void OCXMenuBar::OnLeftDown(wxMouseEvent& event)
{
    int idx = HitTest(event.GetPosition());
    if (idx >= 0)
    {
        ShowMenu(idx);
    }
    else if (m_identityRect.Contains(event.GetPosition()) && m_onIdentityClick)
    {
        m_onIdentityClick();
    }
    event.Skip();
}

void OCXMenuBar::OnSize(wxSizeEvent& event)
{
    Refresh();
    event.Skip();
}
