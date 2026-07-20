#include "ocx_toolbar.h"
#include "app_theme.h"
#include <wx/dcbuffer.h>

static constexpr int TOOL_W  = 60;   // each button's width
static constexpr int SEP_W   = 14;   // separator width
static constexpr int BMP_Y   = 5;    // icon y offset from top
static constexpr int LBL_OFF = 3;    // gap between icon bottom and label top

OCXToolBar::OCXToolBar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition,
              wxSize(-1, HEIGHT), wxBORDER_NONE)
{
    SetBackgroundColour(OCXTheme::BgPanel());
    SetBackgroundStyle(wxBG_STYLE_PAINT); // required for wxBufferedPaintDC

    Bind(wxEVT_PAINT,       &OCXToolBar::OnPaint,       this);
    Bind(wxEVT_MOTION,      &OCXToolBar::OnMouseMove,   this);
    Bind(wxEVT_LEAVE_WINDOW,&OCXToolBar::OnMouseLeave,  this);
    Bind(wxEVT_LEFT_DOWN,   &OCXToolBar::OnLeftDown,    this);
    Bind(wxEVT_LEFT_UP,     &OCXToolBar::OnLeftUp,      this);
    Bind(wxEVT_SIZE,        &OCXToolBar::OnSize,        this);
}

void OCXToolBar::AddTool(int id, const wxBitmap& bmp,
                          const wxString& label, const wxString& tip)
{
    Tool t;
    t.id    = id;
    t.bmp   = bmp;
    t.label = label;
    t.tip   = tip;
    m_tools.push_back(t);
    Refresh();
}

void OCXToolBar::AddSeparator()
{
    Tool t;
    t.isSep = true;
    m_tools.push_back(t);
    Refresh();
}

void OCXToolBar::SetToolBitmap(int id, const wxBitmap& bmp)
{
    for (auto& t : m_tools)
    {
        if (!t.isSep && t.id == id)
        {
            t.bmp = bmp;
            break;
        }
    }
    Refresh();
}

//--
// Layout
//--
void OCXToolBar::RecalcLayout()
{
    int x = 6; // left padding
    for (auto& t : m_tools)
    {
        int w = t.isSep ? SEP_W : TOOL_W;
        t.rect = wxRect(x, 0, w, HEIGHT);
        x += w;
    }
}

int OCXToolBar::HitTest(wxPoint pt) const
{
    for (int i = 0; i < (int)m_tools.size(); ++i)
    {
        if (!m_tools[i].isSep && m_tools[i].rect.Contains(pt))
            return i;
    }
    return -1;
}

//--
// Paint
//--
void OCXToolBar::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize sz = GetClientSize();

    // Background
    dc.SetBackground(wxBrush(OCXTheme::BgPanel()));
    dc.Clear();

    // Bottom border - thin line separating toolbar from content
    dc.SetPen(wxPen(OCXTheme::BgSash()));
    dc.DrawLine(0, sz.y - 1, sz.x, sz.y - 1);

    RecalcLayout();

    // Label font - small, clean
    wxFont lblFont = GetFont();
    lblFont.SetPointSize(wxMax(7, lblFont.GetPointSize() - 2));
    dc.SetFont(lblFont);

    const int lblY = BMP_Y + BMP_SZ + LBL_OFF;

    for (int i = 0; i < (int)m_tools.size(); ++i)
    {
        const Tool& t = m_tools[i];

        if (t.isSep)
        {
            // Thin vertical separator
            int cx = t.rect.x + SEP_W / 2;
            dc.SetPen(wxPen(OCXTheme::BgSash()));
            dc.DrawLine(cx, 8, cx, sz.y - 9);
            continue;
        }

        const wxRect& r = t.rect;

        // Hover / press background (rounded rect, inset 2px h, 3px v)
        if (i == m_pressed)
        {
            wxColour pressCol = OCXTheme::Accent().ChangeLightness(65);
            dc.SetBrush(wxBrush(pressCol));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRoundedRectangle(r.x + 2, 3, r.width - 4, sz.y - 6, 4);
        }
        else if (i == m_hovered)
        {
            dc.SetBrush(wxBrush(OCXTheme::BgButton()));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRoundedRectangle(r.x + 2, 3, r.width - 4, sz.y - 6, 4);
        }

        // Bitmap - centred horizontally, fixed vertical offset
        if (t.bmp.IsOk())
        {
            int bx = r.x + (r.width - BMP_SZ) / 2;
            dc.DrawBitmap(t.bmp, bx, BMP_Y, true);
        }

        // Label - centred, dim colour on normal, full text on hover
        if (!t.label.IsEmpty())
        {
            wxColour tc = (i == m_hovered || i == m_pressed)
                              ? OCXTheme::FgText()
                              : OCXTheme::FgDim();
            dc.SetTextForeground(tc);
            wxSize ts = dc.GetTextExtent(t.label);
            dc.DrawText(t.label, r.x + (r.width - ts.x) / 2, lblY);
        }
    }
}

//--
// Mouse
//--
void OCXToolBar::OnMouseMove(wxMouseEvent& event)
{
    int idx = HitTest(event.GetPosition());
    if (idx != m_hovered)
    {
        m_hovered = idx;
        Refresh();

        // Show tooltip / status hint via parent's GetParent chain
        // (simple: rely on wxToolTip set per button - we use Refresh only)
    }
    event.Skip();
}

void OCXToolBar::OnMouseLeave(wxMouseEvent& event)
{
    m_hovered = -1;
    m_pressed = -1;
    Refresh();
    event.Skip();
}

void OCXToolBar::OnLeftDown(wxMouseEvent& event)
{
    int idx = HitTest(event.GetPosition());
    m_pressed = idx;
    Refresh();
    event.Skip();
}

void OCXToolBar::OnLeftUp(wxMouseEvent& event)
{
    int idx = HitTest(event.GetPosition());
    if (idx >= 0 && idx == m_pressed && !m_tools[idx].isSep)
    {
        // Fire a menu-style command event with the tool's ID so MainWindow
        // event table handlers catch it (EVT_MENU entries).
        wxCommandEvent cmd(wxEVT_MENU, m_tools[idx].id);
        cmd.SetEventObject(this);
        GetEventHandler()->ProcessEvent(cmd);
    }
    m_pressed = -1;
    Refresh();
    event.Skip();
}

void OCXToolBar::OnSize(wxSizeEvent& event)
{
    Refresh();
    event.Skip();
}
