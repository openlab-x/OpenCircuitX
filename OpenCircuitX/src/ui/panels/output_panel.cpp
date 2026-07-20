#include "output_panel.h"
#include "find_results_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/regex.h>
#include <vector>

OutputPanel::OutputPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    notebook = new wxNotebook(this, wxID_ANY);
    notebook->SetBackgroundColour(OCXTheme::BgPanel());

    outputLog = new wxTextCtrl(notebook, wxID_ANY, "",
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxBORDER_NONE);

    errorLog  = new wxTextCtrl(notebook, wxID_ANY, "",
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxBORDER_NONE);

    outputLog->SetBackgroundColour(OCXTheme::BgOutput());
    outputLog->SetForegroundColour(OCXTheme::FgText());
    errorLog->SetBackgroundColour(OCXTheme::BgOutput());
    errorLog->SetForegroundColour(OCXTheme::FgText());

    wxFont monoFont(OCXTheme::FontSize() - 1,
                    wxFONTFAMILY_TELETYPE,
                    wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL,
                    false,
                    OCXTheme::FontFace());

    outputLog->SetFont(monoFont);
    errorLog->SetFont(monoFont);

    notebook->AddPage(outputLog, "Output");
    notebook->AddPage(errorLog,  "Errors");

    m_findResults = new FindResultsPanel(notebook);
    notebook->AddPage(m_findResults, "Find Results");

    errorLog->Bind(wxEVT_LEFT_DCLICK, &OutputPanel::OnErrorDoubleClick, this);

    // Right-click context menu: clear the respective log
    outputLog->Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent& e) {
        wxMenu menu;
        menu.Append(1, "Clear Output");
        if (GetPopupMenuSelectionFromUser(menu, e.GetPosition()) == 1)
            ClearOutput();
    });
    errorLog->Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent& e) {
        wxMenu menu;
        menu.Append(1, "Clear Errors");
        if (GetPopupMenuSelectionFromUser(menu, e.GetPosition()) == 1)
            ClearErrors();
    });

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(notebook, 1, wxEXPAND);
    SetSizer(sizer);
}

void OutputPanel::ReapplyTheme()
{
    SetBackgroundColour(OCXTheme::BgPanel());
    notebook->SetBackgroundColour(OCXTheme::BgPanel());
    outputLog->SetBackgroundColour(OCXTheme::BgOutput());
    outputLog->SetForegroundColour(OCXTheme::FgText());
    errorLog->SetBackgroundColour(OCXTheme::BgOutput());
    errorLog->SetForegroundColour(OCXTheme::FgText());
    m_findResults->ReapplyTheme();
    notebook->Refresh();
    Refresh();
    Update();
}

void OutputPanel::LogMessage(const wxString& message)
{
    outputLog->SetDefaultStyle(wxTextAttr(OCXTheme::FgText()));
    outputLog->AppendText(message + "\n");
}

void OutputPanel::LogError(const wxString& message)
{
    errorLog->SetDefaultStyle(wxTextAttr(wxColour(244, 135, 113)));
    errorLog->AppendText(message + "\n");
    errorLog->SetDefaultStyle(wxTextAttr(OCXTheme::FgText()));
}

void OutputPanel::ClearOutput()
{
    outputLog->Clear();
}

void OutputPanel::ClearErrors()
{
    errorLog->Clear();
}

void OutputPanel::ShowOutputTab()
{
    notebook->SetSelection(0);
}

void OutputPanel::ShowErrorTab()
{
    notebook->SetSelection(1);
}

void OutputPanel::AddTab(wxWindow* page, const wxString& label)
{
    if (page->GetParent() != notebook)
        page->Reparent(notebook);
    notebook->AddPage(page, label);
}

void OutputPanel::ShowFindResultsTab()
{
    // Find Results is tab index 2 (Output=0, Errors=1, Find Results=2)
    for (int i = 0; i < (int)notebook->GetPageCount(); ++i)
    {
        if (notebook->GetPage(i) == m_findResults)
        { notebook->SetSelection(i); return; }
    }
}

void OutputPanel::SetErrorJumpCallback(
    std::function<void(const wxString&, int, int)> cb)
{
    m_jumpCallback = cb;
}

void OutputPanel::OnErrorDoubleClick(wxMouseEvent& event)
{
    event.Skip(); // allow default selection

    if (!m_jumpCallback)
        return;

    // Get the text of the line under the cursor
    long x, y;
    errorLog->HitTest(event.GetPosition(), &x, &y);
    long lineStart = errorLog->XYToPosition(0, y);
    long lineEnd   = errorLog->XYToPosition(0, y + 1);
    if (lineEnd < 0)
        lineEnd = errorLog->GetLastPosition();

    wxString lineText = errorLog->GetRange(lineStart, lineEnd);
    lineText.Trim(true);
    lineText.Trim(false);

    wxString file;
    int line = 0, col = 1;
    if (ParseErrorLine(lineText, file, line, col))
        m_jumpCallback(file, line, col);
}

bool OutputPanel::ParseErrorLine(const wxString& text,
                                  wxString& file, int& line, int& col)
{
    // Matches: <path>:<line>:<col>:
    // The greedy (.+) correctly handles Windows paths like C:\dir\file.vhd
    wxRegEx re("^(.+):([0-9]+):([0-9]+):");
    if (!re.Matches(text))
        return false;

    file = re.GetMatch(text, 1);
    re.GetMatch(text, 2).ToInt(&line);
    re.GetMatch(text, 3).ToInt(&col);
    return line > 0;
}

std::vector<OutputPanel::ParsedError> OutputPanel::GetParsedErrors() const
{
    std::vector<ParsedError> results;
    wxString all = errorLog->GetValue();
    wxArrayString lines = wxSplit(all, '\n');
    for (const wxString& ln : lines)
    {
        ParsedError pe;
        wxString text = ln;
        text.Trim(true).Trim(false);
        if (const_cast<OutputPanel*>(this)->ParseErrorLine(text, pe.file, pe.line, pe.col))
            results.push_back(pe);
    }
    return results;
}
