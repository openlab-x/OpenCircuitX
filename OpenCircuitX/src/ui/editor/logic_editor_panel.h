#pragma once
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/aui/auibook.h>
#include <vector>
#include <functional>

class LogicEditorPanel : public wxPanel
{
public:
    LogicEditorPanel(wxWindow* parent);

    void     LoadFile(const wxString& filePath);
    void     SaveCurrentFile();

    // Save the current tab to a new path (used for "Save As" on unsaved tabs).
    // Updates the tab's backing path and label, then writes the file.
    void     SaveCurrentFileAs(const wxString& newPath);

    // Open a new unsaved tab with the given content (used by canvas Export to Editor).
    // The tab is marked modified (*) since it has no backing file yet.
    void     NewTabWithContent(const wxString& tabName, const wxString& content);

    // Jump to a specific line (and optional column) in the given file.
    // Opens the file if it is not already loaded.
    void     JumpToFileLine(const wxString& filePath, int line, int col = 1);

    // Show / hide the inline find bar (Ctrl+F) or find+replace bar (Ctrl+H).
    void     ShowFindBar();
    void     ShowReplaceBar();
    void     HideFindBar();

    // Go to a specific line (1-based).
    void     GotoLine(int line);

    // Toggle line numbers / code folding across all open tabs.
    void     SetLineNumbers(bool show);
    void     SetCodeFolding(bool enable);

    // Re-apply the current OCXTheme to all open editor tabs (call after theme switch).
    void     ReapplyTheme();

    // Insert text at the current cursor position in the active editor tab.
    void InsertSnippet(const wxString& text);

    // Editor font zoom (applied to the active tab only).
    void EditorZoomIn();
    void EditorZoomOut();
    void EditorZoomReset();

    // Toggle line comments on the selected lines (or current line).
    // Detects VHDL (--) vs Verilog (//) from the file extension.
    void ToggleLineComment();

    // Bookmarks (marker 2 - blue bookmark shape in the gutter).
    void ToggleBookmark();
    void GotoNextBookmark();
    void GotoPrevBookmark();

    // Auto-save: saves all open tabs that have a backing file and are modified.
    // Returns the number of files saved.
    int SaveAllModified();

    // Reopen the last closed tab (Ctrl+Shift+T).
    // Returns true if a tab was restored, false if the closed-tab history is empty.
    bool ReopenLastClosedTab();

    // Word wrap across all open tabs.
    void SetWordWrap(bool enable);

    // Show spaces as dots and tabs as arrows across all open tabs.
    void SetShowWhitespace(bool show);

    // Move current line (or selection) up / down (Alt+Up / Alt+Down).
    void MoveLineUp();
    void MoveLineDown();

    // Callback fired on every caret movement: (1-based line, 1-based col, file ext).
    // Use this to drive a status-bar line/column indicator.
    void SetCaretCallback(std::function<void(int, int, const wxString&)> cb);

    // Callback fired whenever the active tab changes or is modified.
    // Passes (code, ext) so the OutlinePanel can re-parse without polling.
    void SetOutlineCallback(std::function<void(const wxString&, const wxString&)> cb);

    // Returns the word (identifier) under the cursor in the active editor.
    wxString GetWordAtCursor() const;

    // Returns the lowercase file extension of the active tab ("vhd","v","sv",…).
    wxString GetCurrentFileExt() const;

    // Duplicate the current line (or selection) - Ctrl+D.
    void DuplicateLine();

    // Highlight all occurrences of the word under the cursor - Ctrl+Shift+L.
    void SelectAllOccurrences();

    // Jump to the matching brace/parenthesis - Ctrl+].
    void JumpToMatchingBrace();

    // Sort selected lines alphabetically (ascending).
    void SortSelectedLines();

    // Returns total line count of the active editor (0 if no tab open).
    int GetLineCount() const;

    // Inline error decorations - red squiggle underline on error lines.
    // errors: list of {filePath, 1-based line} pairs. Clears previous decorations first.
    struct ErrorMark { wxString filePath; int line; };
    void SetErrorDecorations(const std::vector<ErrorMark>& errors);
    void ClearErrorDecorations();

    // Breakpoint gutter - returns 1-based line numbers of all breakpoints
    // in the currently visible editor tab.
    std::vector<int> GetBreakpoints() const;

    // Feed project symbol names (signals, ports, identifiers) into auto-complete.
    // Call this from the outline-update callback so completions stay current.
    void SetAutoCompleteSymbols(const wxArrayString& symbols);

    // Callback fired after every explicit save (Ctrl+S, auto-save).
    // Receives (filePath, lowercase extension) - used for live syntax checking.
    void SetSaveCallback(std::function<void(const wxString&, const wxString&)> cb);

    // Callback invoked on mouse-dwell over an identifier.
    // Given a signal name, returns a tooltip string (empty = no tooltip shown).
    void SetSignalValueCallback(std::function<wxString(const wxString&)> cb);

    // Close all tabs except the one at keepIdx.
    void CloseOtherTabs(int keepIdx);

    // Close every open tab unconditionally (used when switching projects).
    void CloseAllTabs();

    void Undo();
    void Redo();

    wxString GetCode() const;
    void     SetCode(const wxString& code);
    wxString      GetCurrentFilePath() const;
    wxArrayString GetOpenFilePaths() const;

private:
    struct EditorTab
    {
        wxStyledTextCtrl* editor;
        wxString          filePath;
    };

    struct ClosedTab
    {
        wxString tabName;
        wxString filePath;
        wxString content;
    };

    wxAuiNotebook*          notebook;
    std::vector<EditorTab>  tabs;
    std::vector<ClosedTab>  m_closedTabs;  // LIFO stack for Ctrl+Shift+T

    bool m_showLineNumbers  = true;
    bool m_codeFolding      = false;
    bool m_wordWrap         = false;
    bool m_showWhitespace   = false;

    // Find / Replace bar
    wxPanel*    m_findBar;
    wxTextCtrl* m_findField;
    wxPanel*    m_replaceRow;      // second row - hidden in Find-only mode
    wxTextCtrl* m_replaceField;
    wxBoxSizer* m_rootSizer;

    void FindInEditor(bool forward);
    void ReplaceOne();
    void ReplaceAll();
    void OnFindNext(wxCommandEvent& event);
    void OnFindPrev(wxCommandEvent& event);
    void OnFindClose(wxCommandEvent& event);
    void OnFindKeyDown(wxKeyEvent& event);
    void OnReplaceOne(wxCommandEvent& event);
    void OnReplaceAll(wxCommandEvent& event);
    void OnMarginClick(wxStyledTextEvent& event);
    void OnCharAdded(wxStyledTextEvent& event);

    wxStyledTextCtrl* CreateEditor();
    wxStyledTextCtrl* GetCurrentEditor() const;
    void              ApplyBaseStyles(wxStyledTextCtrl* editor);
    int               FindTabByPath(const wxString& filePath) const;

    void ApplyLexer(wxStyledTextCtrl* editor, const wxString& extension);
    void UpdateTabLabel(wxStyledTextCtrl* editor, bool modified);

    std::function<void(int, int, const wxString&)>      m_caretCallback;
    std::function<void(const wxString&, const wxString&)> m_outlineCallback;
    std::function<void(const wxString&, const wxString&)> m_saveCallback;
    std::function<wxString(const wxString&)>             m_signalValueCallback;

    bool          m_loadingFile = false;   // suppresses save callback during LoadFile
    wxArrayString m_extraSymbols;  // project symbol names for auto-complete

    void OnTabClose(wxAuiNotebookEvent& event);
    void OnTabRightClick(wxAuiNotebookEvent& event);
    void OnEditorModified(wxStyledTextEvent& event);
    void OnEditorSaved(wxStyledTextEvent& event);
};
