#pragma once
#include <wx/wx.h>
#include <vector>

//--
// OCXToolBar - custom-drawn toolbar panel.
//
// Replaces the standard wxToolBar to get full visual control:
// - 48 px tall, dark background matching the app theme
// - Per-tool rounded-rect hover/press highlight
// - 22×22 bitmap + small label per tool
// - Thin separator between tool groups
// - Bottom border that blends with the content below
//--
class OCXToolBar : public wxPanel
{
public:
    static constexpr int HEIGHT = 48;
    static constexpr int BMP_SZ = 22;

    explicit OCXToolBar(wxWindow* parent);

    // Add a tool button.  Fires wxEVT_MENU with 'id' when clicked.
    void AddTool(int id, const wxBitmap& bmp,
                 const wxString& label, const wxString& tip = "");

    // Add a visual separator between groups.
    void AddSeparator();

    // Replace the bitmap of an existing tool (for theme switches).
    void SetToolBitmap(int id, const wxBitmap& bmp);

private:
    struct Tool
    {
        int      id    = 0;
        wxBitmap bmp;
        wxString label;
        wxString tip;
        bool     isSep = false;
        wxRect   rect;          // filled by RecalcLayout()
    };

    std::vector<Tool> m_tools;
    int m_hovered = -1;
    int m_pressed = -1;

    void RecalcLayout();
    int  HitTest(wxPoint pt) const;

    void OnPaint(wxPaintEvent&);
    void OnMouseMove(wxMouseEvent&);
    void OnMouseLeave(wxMouseEvent&);
    void OnLeftDown(wxMouseEvent&);
    void OnLeftUp(wxMouseEvent&);
    void OnSize(wxSizeEvent&);
};
