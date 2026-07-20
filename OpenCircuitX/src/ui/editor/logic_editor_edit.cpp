#include "logic_editor_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>

//--
// Jump to file / line
//--
void LogicEditorPanel::JumpToFileLine(const wxString& filePath, int line, int col)
{
    // Open the file if not already loaded
    LoadFile(filePath);

    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor)
        return;

    // Scintilla lines are 0-based
    int zeroLine = wxMax(0, line - 1);
    editor->EnsureVisibleEnforcePolicy(zeroLine);
    editor->GotoLine(zeroLine);

    // Move to column if provided
    if (col > 1)
    {
        int lineStart = editor->PositionFromLine(zeroLine);
        editor->GotoPos(lineStart + col - 1);
    }

    editor->SetFocus();
}

//--
// Find bar
//--
void LogicEditorPanel::ShowFindBar()
{
    m_replaceRow->Show(false);
    m_findBar->Show(true);
    m_findBar->Layout();
    m_rootSizer->Layout();
    m_findField->SetFocus();
    m_findField->SelectAll();
}

void LogicEditorPanel::ShowReplaceBar()
{
    m_replaceRow->Show(true);
    m_findBar->Show(true);
    m_findBar->Layout();
    m_rootSizer->Layout();
    m_findField->SetFocus();
    m_findField->SelectAll();
}

void LogicEditorPanel::HideFindBar()
{
    m_replaceRow->Show(false);
    m_findBar->Show(false);
    m_rootSizer->Layout();

    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor)
        editor->SetFocus();
}

void LogicEditorPanel::GotoLine(int line)
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;
    // line is 1-based from the dialog; STC uses 0-based
    int l = wxMax(0, line - 1);
    editor->GotoLine(l);
    editor->EnsureCaretVisible();
    editor->SetFocus();
}

void LogicEditorPanel::InsertSnippet(const wxString& text)
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;
    editor->InsertText(editor->GetCurrentPos(), text);
    // Move caret to end of inserted text
    editor->SetCurrentPos(editor->GetCurrentPos() + (int)text.Length());
    editor->SetAnchor(editor->GetCurrentPos());
    editor->EnsureCaretVisible();
    editor->SetFocus();
}

void LogicEditorPanel::ToggleBookmark()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;
    int line  = editor->GetCurrentLine();
    int state = editor->MarkerGet(line);
    if (state & (1 << 2))
        editor->MarkerDelete(line, 2);
    else
        editor->MarkerAdd(line, 2);
}

void LogicEditorPanel::GotoNextBookmark()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;
    int line = editor->GetCurrentLine();
    int next = editor->MarkerNext(line + 1, 1 << 2);
    if (next == -1)
        next = editor->MarkerNext(0, 1 << 2);   // wrap around
    if (next != -1)
    {
        editor->GotoLine(next);
        editor->EnsureCaretVisible();
    }
}

void LogicEditorPanel::GotoPrevBookmark()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;
    int line = editor->GetCurrentLine();
    int prev = editor->MarkerPrevious(line - 1, 1 << 2);
    if (prev == -1)
        prev = editor->MarkerPrevious(editor->GetLineCount() - 1, 1 << 2);  // wrap
    if (prev != -1)
    {
        editor->GotoLine(prev);
        editor->EnsureCaretVisible();
    }
}

void LogicEditorPanel::ToggleLineComment()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;

    // Determine comment prefix from file extension
    int sel = notebook->GetSelection();
    wxString ext;
    if (sel >= 0 && sel < (int)tabs.size())
        ext = wxFileName(tabs[sel].filePath).GetExt().Lower();
    wxString prefix = (ext == "v" || ext == "sv") ? "// " : "-- ";

    // Expand selection to full lines
    int startPos  = editor->GetSelectionStart();
    int endPos    = editor->GetSelectionEnd();
    int startLine = editor->LineFromPosition(startPos);
    int endLine   = editor->LineFromPosition(endPos);
    // If selection ends exactly at a line start, exclude that line
    if (endPos > startPos && editor->PositionFromLine(endLine) == endPos)
        --endLine;

    // Check if ALL selected lines are already commented with this prefix
    bool allCommented = true;
    for (int ln = startLine; ln <= endLine; ++ln)
    {
        wxString lineText = editor->GetLine(ln);
        lineText.Trim(false);   // strip leading whitespace
        if (!lineText.StartsWith(prefix))
        { allCommented = false; break; }
    }

    editor->BeginUndoAction();
    if (allCommented)
    {
        // Remove the prefix (first occurrence) from each line
        for (int ln = endLine; ln >= startLine; --ln)   // reverse to keep positions valid
        {
            wxString lineText = editor->GetLine(ln);
            int lineStart     = editor->PositionFromLine(ln);
            size_t prefixPos  = lineText.Find(prefix);
            if (prefixPos != wxString::npos)
                editor->DeleteRange(lineStart + (int)prefixPos, (int)prefix.Length());
        }
    }
    else
    {
        // Insert prefix at the start of each line (before any leading whitespace)
        for (int ln = endLine; ln >= startLine; --ln)   // reverse order
        {
            int lineStart = editor->PositionFromLine(ln);
            editor->InsertText(lineStart, prefix);
        }
    }
    editor->EndUndoAction();
}

void LogicEditorPanel::SetWordWrap(bool enable)
{
    m_wordWrap = enable;
    int mode = enable ? wxSTC_WRAP_WORD : wxSTC_WRAP_NONE;
    for (auto& tab : tabs)
        if (tab.editor) tab.editor->SetWrapMode(mode);
}

void LogicEditorPanel::SetShowWhitespace(bool show)
{
    m_showWhitespace = show;
    int mode = show ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE;
    for (auto& tab : tabs)
        if (tab.editor) tab.editor->SetViewWhiteSpace(mode);
}

void LogicEditorPanel::MoveLineUp()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor) editor->MoveSelectedLinesUp();
}

void LogicEditorPanel::MoveLineDown()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor) editor->MoveSelectedLinesDown();
}

void LogicEditorPanel::EditorZoomIn()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor) editor->ZoomIn();
}

void LogicEditorPanel::EditorZoomOut()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor) editor->ZoomOut();
}

void LogicEditorPanel::EditorZoomReset()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor) editor->SetZoom(0);
}

