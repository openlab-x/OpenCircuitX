#pragma once
#include <wx/wx.h>
#include <vector>
#include <functional>

//--
// OCXMenuBar - custom-drawn menu bar panel.
//
// Replaces the native wxMenuBar for full visual control:
// - 30 px tall, dark background
// - Menu title buttons with hover highlight (rounded rect)
// - Active/open state uses accent colour tint
// - Shows existing wxMenu* via PopupMenu() - events bubble to MainWindow
// - Right-aligned identity mark: app name + version, always visible;
//   switches to a clickable "update available" state when one is found
//--
class OCXMenuBar : public wxPanel
{
public:
    static constexpr int HEIGHT = 30;

    explicit OCXMenuBar(wxWindow* parent);

    // Append a menu.  'title' may contain '&' mnemonics (stripped for display).
    void Append(wxMenu* menu, const wxString& title);

    // Sets the version shown in the identity mark and the handler fired
    // when it's clicked (normally opens the Check for Updates dialog).
    void SetVersionInfo(const wxString& version, std::function<void()> onClick);

    // Switches the identity mark to the "update available" state.
    void SetUpdateAvailable(const wxString& newVersion);

private:
    struct Entry
    {
        wxString title;   // stripped of '&'
        wxMenu*  menu;
        wxRect   rect;    // computed in RecalcLayout()
    };

    std::vector<Entry> m_entries;
    int m_hovered = -1;
    int m_open    = -1;   // index of the currently-open popup (-1 = none)

    wxString m_version;
    wxString m_updateVersion;   // empty = no update available
    std::function<void()> m_onIdentityClick;
    wxRect m_identityRect;      // computed in OnPaint, used for hit-testing
    bool   m_identityHovered = false;

    void RecalcLayout();
    int  HitTest(wxPoint pt) const;
    void ShowMenu(int idx);

    void OnPaint(wxPaintEvent&);
    void OnMouseMove(wxMouseEvent&);
    void OnMouseLeave(wxMouseEvent&);
    void OnLeftDown(wxMouseEvent&);
    void OnSize(wxSizeEvent&);
};
