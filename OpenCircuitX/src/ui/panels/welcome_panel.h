#pragma once
#include <wx/wx.h>
#include <functional>

class WelcomePanel : public wxPanel
{
public:
    WelcomePanel(wxWindow* parent,
                 std::function<void()> onNewProject,
                 std::function<void()> onOpenProject,
                 std::function<void(const wxString&)> onOpenRecent);

    void RefreshRecent(const wxArrayString& paths);
    void ReapplyTheme();

private:
    std::function<void()>                m_onNewProject;
    std::function<void()>                m_onOpenProject;
    std::function<void(const wxString&)> m_onOpenRecent;

    wxPanel*      m_content    = nullptr;
    wxPanel*      m_recentBox  = nullptr;
    wxBoxSizer*   m_recentSizer= nullptr;

    wxStaticText* m_appName    = nullptr;
    wxStaticText* m_verLine    = nullptr;
    wxStaticText* m_domLbl     = nullptr;
    wxStaticText* m_recLbl     = nullptr;
    wxStaticText* m_footer     = nullptr;
};
