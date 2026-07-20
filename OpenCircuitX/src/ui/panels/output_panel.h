#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <functional>

class FindResultsPanel;

class OutputPanel : public wxPanel
{
public:
    OutputPanel(wxWindow* parent);

    void LogMessage(const wxString& message);
    void LogError(const wxString& message);

    void ClearOutput();
    void ClearErrors();

    void ShowOutputTab();
    void ShowErrorTab();

    // Add an extra tab to the output notebook (used by MainWindow for WatchPanel).
    void AddTab(wxWindow* page, const wxString& label);

    // Show the Find Results tab.
    void ShowFindResultsTab();

    // Access the Find Results panel so MainWindow can populate it directly.
    FindResultsPanel* GetFindResultsPanel() { return m_findResults; }

    void ReapplyTheme();

    // Register a callback invoked when the user double-clicks an error line.
    // The callback receives the file path, 1-based line number, and column.
    void SetErrorJumpCallback(
        std::function<void(const wxString& file, int line, int col)> cb);

    struct ParsedError { wxString file; int line; int col; };

    // Return all successfully parsed error locations from the Errors tab.
    std::vector<ParsedError> GetParsedErrors() const;

private:
    wxNotebook* notebook;
    wxTextCtrl* outputLog;
    wxTextCtrl* errorLog;

    std::function<void(const wxString&, int, int)> m_jumpCallback;
    FindResultsPanel* m_findResults = nullptr;

    void OnErrorDoubleClick(wxMouseEvent& event);

    // Parses a GHDL-style error line:  path/file.vhd:10:5: error: ...
    // Returns true and fills file/line/col on success.
    bool ParseErrorLine(const wxString& text,
                        wxString& file, int& line, int& col);
};
