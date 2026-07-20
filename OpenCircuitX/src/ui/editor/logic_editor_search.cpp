#include "logic_editor_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>
#include "ui/editor/logic_editor_private.h"

void LogicEditorPanel::FindInEditor(bool forward)
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor)
        return;

    wxString text = m_findField->GetValue();
    if (text.IsEmpty())
        return;

    int docLen = editor->GetLength();
    int curPos = editor->GetCurrentPos();
    int flags  = 0; // case-insensitive

    int found = wxNOT_FOUND;
    if (forward)
    {
        found = editor->FindText(curPos + 1, docLen, text, flags);
        if (found == wxNOT_FOUND)
            found = editor->FindText(0, curPos, text, flags); // wrap
    }
    else
    {
        found = editor->FindText(curPos - 1, 0, text, flags);
        if (found == wxNOT_FOUND)
            found = editor->FindText(docLen, curPos, text, flags); // wrap
    }

    if (found != wxNOT_FOUND)
    {
        editor->SetSelection(found, found + (int)text.Length());
        editor->EnsureCaretVisible();
        m_findField->SetBackgroundColour(OCXTheme::BgEditor());
    }
    else
    {
        m_findField->SetBackgroundColour(wxColour(100, 40, 40)); // not found tint
    }
    m_findField->Refresh();
}

void LogicEditorPanel::OnFindNext(wxCommandEvent&)  { FindInEditor(true);  }
void LogicEditorPanel::OnFindPrev(wxCommandEvent&)  { FindInEditor(false); }
void LogicEditorPanel::OnFindClose(wxCommandEvent&) { HideFindBar(); }

void LogicEditorPanel::OnFindKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE)
        HideFindBar();
    else if (event.GetKeyCode() == WXK_RETURN && event.ShiftDown())
        FindInEditor(false);
    else
        event.Skip();
}

void LogicEditorPanel::ReplaceOne()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor)
        return;

    wxString needle      = m_findField->GetValue();
    wxString replacement = m_replaceField->GetValue();
    if (needle.IsEmpty())
        return;

    // If the current selection already matches, replace it; then find next.
    wxString sel = editor->GetSelectedText();
    if (sel.IsSameAs(needle, false))   // case-insensitive match
    {
        editor->ReplaceSelection(replacement);
    }

    // Advance to next occurrence
    FindInEditor(true);
}

void LogicEditorPanel::ReplaceAll()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor)
        return;

    wxString needle      = m_findField->GetValue();
    wxString replacement = m_replaceField->GetValue();
    if (needle.IsEmpty())
        return;

    editor->BeginUndoAction();

    int  count  = 0;
    int  docLen = editor->GetLength();
    int  pos    = editor->FindText(0, docLen, needle, 0);

    while (pos != wxNOT_FOUND)
    {
        editor->SetSelection(pos, pos + (int)needle.Length());
        editor->ReplaceSelection(replacement);
        ++count;

        // Recalculate doc length after replacement
        docLen = editor->GetLength();
        pos    = editor->FindText(pos + (int)replacement.Length(), docLen, needle, 0);
    }

    editor->EndUndoAction();

    if (count > 0)
        wxMessageBox(wxString::Format("Replaced %d occurrence(s).", count),
                     "Replace All", wxOK | wxICON_INFORMATION, this);
    else
        wxMessageBox("No occurrences found.", "Replace All",
                     wxOK | wxICON_INFORMATION, this);
}

void LogicEditorPanel::OnReplaceOne(wxCommandEvent&) { ReplaceOne(); }
void LogicEditorPanel::OnReplaceAll(wxCommandEvent&) { ReplaceAll(); }

//--
// Breakpoint gutter
//--
void LogicEditorPanel::OnMarginClick(wxStyledTextEvent& event)
{
    wxStyledTextCtrl* editor = static_cast<wxStyledTextCtrl*>(event.GetEventObject());
    if (!editor)
        return;

    int line = editor->LineFromPosition(event.GetPosition());

    if (event.GetMargin() == 2)
    {
        // Breakpoint toggle - marker 1, mask bit = (1 << 1)
        int mask = editor->MarkerGet(line);
        if (mask & (1 << 1))
            editor->MarkerDelete(line, 1);
        else
            editor->MarkerAdd(line, 1);
    }
    else if (event.GetMargin() == 3)
    {
        // Code fold toggle
        editor->ToggleFold(line);
    }
    else
    {
        event.Skip();
    }
}

//--
// View toggles
//--
void LogicEditorPanel::SetLineNumbers(bool show)
{
    m_showLineNumbers = show;
    for (const EditorTab& t : tabs)
        t.editor->SetMarginWidth(0, show ? 52 : 0);
}

void LogicEditorPanel::SetCodeFolding(bool enable)
{
    m_codeFolding = enable;
    for (const EditorTab& t : tabs)
    {
        if (enable)
        {
            // Folding margin (margin 3)
            t.editor->SetMarginWidth(3, 14);
            t.editor->SetMarginType(3, wxSTC_MARGIN_SYMBOL);
            t.editor->SetMarginMask(3, wxSTC_MASK_FOLDERS);
            t.editor->SetMarginSensitive(3, true);

            // Fold markers
            t.editor->MarkerDefine(wxSTC_MARKNUM_FOLDER,        wxSTC_MARK_BOXPLUS);
            t.editor->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN,    wxSTC_MARK_BOXMINUS);
            t.editor->MarkerDefine(wxSTC_MARKNUM_FOLDEREND,     wxSTC_MARK_BOXPLUSCONNECTED);
            t.editor->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER);
            t.editor->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED);
            t.editor->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB,     wxSTC_MARK_VLINE);
            t.editor->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL,    wxSTC_MARK_LCORNER);

            for (int m = wxSTC_MARKNUM_FOLDER; m <= wxSTC_MARKNUM_FOLDERTAIL; ++m)
            {
                t.editor->MarkerSetForeground(m, OCXTheme::BgMargin());
                t.editor->MarkerSetBackground(m, OCXTheme::FgDim());
            }

            t.editor->SetProperty("fold", "1");
            t.editor->SetProperty("fold.compact", "0");
        }
        else
        {
            t.editor->SetMarginWidth(3, 0);
            t.editor->SetProperty("fold", "0");
        }
    }
}

//--
// Theme reapply
//--
void LogicEditorPanel::ReapplyTheme()
{
    SetBackgroundColour(OCXTheme::BgPanel());
    notebook->SetBackgroundColour(OCXTheme::BgPanel());

    for (const EditorTab& t : tabs)
    {
        ApplyBaseStyles(t.editor);
        // Re-apply syntax colours for the file type
        wxString ext = wxFileName(t.filePath).GetExt().Lower();
        ApplyLexer(t.editor, ext);
        t.editor->Refresh();
    }

    Refresh();
}

//--
// Smart auto-indent
//--
// Build a space-separated keyword list as a wxString for auto-complete filtering.
static wxString s_vhdlKwList    = wxString::FromAscii(VHDL_KEYWORDS);
static wxString s_verilogKwList = wxString::FromAscii(VERILOG_KEYWORDS);

void LogicEditorPanel::OnCharAdded(wxStyledTextEvent& event)
{
    wxStyledTextCtrl* editor = static_cast<wxStyledTextCtrl*>(event.GetEventObject());
    if (!editor)
        return;

    int ch = event.GetKey();

    //** Keyword auto-complete (trigger after 2+ letters) **//
    if (ch != '\n' && ch != '\r' && ch != ' ' && ch != '\t')
    {
        int curPos  = editor->GetCurrentPos();
        int wordStart = editor->WordStartPosition(curPos, true);
        int wordLen   = curPos - wordStart;

        if (wordLen >= 2 && !editor->AutoCompActive())
        {
            // Pick keyword list based on lexer
            int lexer = editor->GetLexer();
            const wxString* kwList = nullptr;
            if (lexer == wxSTC_LEX_VHDL)
                kwList = &s_vhdlKwList;
            else if (lexer == wxSTC_LEX_VERILOG)
                kwList = &s_verilogKwList;

            if (kwList)
            {
                wxString typed = editor->GetTextRange(wordStart, curPos).Lower();
                wxString matches;
                wxStringTokenizer tok(*kwList, " \t\n\r");
                while (tok.HasMoreTokens())
                {
                    wxString kw = tok.GetNextToken();
                    if (kw.Lower().StartsWith(typed))
                    {
                        if (!matches.IsEmpty())
                            matches += ' ';
                        matches += kw;
                    }
                }
                // Also include project symbols (signals, ports, identifiers)
                for (const wxString& sym : m_extraSymbols)
                {
                    if (sym.Lower().StartsWith(typed))
                    {
                        if (!matches.IsEmpty())
                            matches += ' ';
                        matches += sym;
                    }
                }
                if (!matches.IsEmpty())
                {
                    editor->AutoCompSetSeparator(' ');
                    editor->AutoCompShow(wordLen, matches);
                }
            }
        }
    }

    // Auto-close brackets
    if (ch == '(' || ch == '[' || ch == '{')
    {
        wxChar closing = (ch == '(') ? ')' : (ch == '[') ? ']' : '}';
        int pos = editor->GetCurrentPos();
        editor->InsertText(pos, wxString(closing));
        // caret stays between the pair - no extra movement needed
    }

    if (ch != '\n')
        return;

    int curLine = editor->GetCurrentLine();
    if (curLine <= 0)
        return;

    int prevLine = curLine - 1;
    wxString prevText = editor->GetLine(prevLine);

    // Count leading whitespace on previous line
    int indent = 0;
    for (size_t i = 0; i < prevText.Length(); ++i)
    {
        if (prevText[i] == ' ')
            indent++;
        else if (prevText[i] == '\t')
            indent += editor->GetTabWidth();
        else
            break;
    }

    // Find last non-whitespace character to check indent-trigger keywords
    int lastReal = (int)prevText.Length() - 1;
    while (lastReal >= 0 && (prevText[lastReal] == '\r' || prevText[lastReal] == '\n' ||
                              prevText[lastReal] == ' '  || prevText[lastReal] == '\t'))
        lastReal--;

    if (lastReal >= 0)
    {
        wxString tail = prevText.Left(lastReal + 1).Lower();

        // VHDL keywords that open a new indented block
        bool inc = tail.EndsWith("begin")    || tail.EndsWith("then")     ||
                   tail.EndsWith("loop")     || tail.EndsWith("generate") ||
                   tail.EndsWith("process")  || tail.EndsWith("record")   ||
                   tail.EndsWith("is")       || tail.EndsWith("=>")       ||
                   // Verilog / SystemVerilog
                   tail.EndsWith("(")        || tail.EndsWith("{")        ||
                   tail.EndsWith("always")   || tail.EndsWith("initial")  ||
                   tail.EndsWith("module")   || tail.EndsWith("function") ||
                   tail.EndsWith("task");

        if (inc)
            indent += editor->GetIndent();
    }

    if (indent > 0)
    {
        editor->SetLineIndentation(curLine, indent);
        editor->GotoPos(editor->GetLineIndentPosition(curLine));
    }
}

std::vector<int> LogicEditorPanel::GetBreakpoints() const
{
    std::vector<int> lines;
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor)
        return lines;

    int lineCount = editor->GetLineCount();
    for (int i = 0; i < lineCount; ++i)
    {
        if (editor->MarkerGet(i) & (1 << 1))
            lines.push_back(i + 1); // return 1-based
    }
    return lines;
}

//--
// Outline / Symbol intelligence
//--
void LogicEditorPanel::SetOutlineCallback(std::function<void(const wxString&, const wxString&)> cb)
{
    m_outlineCallback = cb;

    // Fire immediately for the current tab (if any)
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor && m_outlineCallback)
    {
        wxString ext = GetCurrentFileExt();
        m_outlineCallback(editor->GetText(), ext);
    }

    // Re-fire whenever the active tab changes
    notebook->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, [this](wxAuiNotebookEvent& e) {
        if (m_outlineCallback)
        {
            wxStyledTextCtrl* ed = GetCurrentEditor();
            if (ed)
                m_outlineCallback(ed->GetText(), GetCurrentFileExt());
        }
        e.Skip();
    });

    // Re-fire on every document change (debounced via STC_MODIFIED)
    // We use STC_SAVEPOINTLEFT which fires on first modification after save - 
    // to get continuous updates we bind STC_MODIFIED on each new editor in CreateEditor().
    // The binding below catches it for all already-open tabs.
    for (auto& tab : tabs)
    {
        if (tab.editor)
        {
            tab.editor->Bind(wxEVT_STC_MODIFIED, [this](wxStyledTextEvent& ev) {
                if (m_outlineCallback)
                {
                    wxStyledTextCtrl* ed = GetCurrentEditor();
                    // Only update if this is the active editor
                    if (ed && ev.GetEventObject() == ed)
                        m_outlineCallback(ed->GetText(), GetCurrentFileExt());
                }
                ev.Skip();
            });
        }
    }
}

wxString LogicEditorPanel::GetCurrentFileExt() const
{
    int sel = notebook->GetSelection();
    if (sel == wxNOT_FOUND || sel < 0 || sel >= (int)tabs.size())
        return wxEmptyString;
    return wxFileName(tabs[sel].filePath).GetExt().Lower();
}

wxString LogicEditorPanel::GetWordAtCursor() const
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return wxEmptyString;

    int pos   = editor->GetCurrentPos();
    int start = editor->WordStartPosition(pos, true);
    int end   = editor->WordEndPosition(pos, true);
    if (end <= start) return wxEmptyString;
    return editor->GetTextRange(start, end);
}

//--
// Editor power features (Session 38)
//--
void LogicEditorPanel::DuplicateLine()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;

    // If there is a selection, duplicate it; otherwise duplicate the whole line.
    if (editor->GetSelectionStart() != editor->GetSelectionEnd())
    {
        wxString sel = editor->GetSelectedText();
        int end = editor->GetSelectionEnd();
        editor->InsertText(end, sel);
        editor->SetSelection(end, end + (int)sel.Length());
    }
    else
    {
        editor->LineDuplicate();
    }
}

void LogicEditorPanel::SelectAllOccurrences()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;

    wxString word = GetWordAtCursor();
    if (word.IsEmpty()) return;

    // Use indicator 8 to highlight all occurrences (non-destructive)
    editor->SetIndicatorCurrent(8);
    editor->IndicatorClearRange(0, editor->GetLength());
    editor->IndicatorSetStyle(8, wxSTC_INDIC_ROUNDBOX);
    editor->IndicatorSetForeground(8, wxColour(255, 200, 0));
    editor->SendMsg(2089, 8, 80); // SCI_INDICSETALPHA

    // Walk the document and mark every occurrence
    int docLen = editor->GetLength();
    int pos    = editor->FindText(0, docLen, word, wxSTC_FIND_WHOLEWORD);
    int count  = 0;
    while (pos != wxNOT_FOUND)
    {
        editor->IndicatorFillRange(pos, (int)word.Length());
        pos = editor->FindText(pos + (int)word.Length(), docLen, word, wxSTC_FIND_WHOLEWORD);
        ++count;
    }

    // Also set multi-selection anchors so the user can type to replace all
    editor->SetMultipleSelection(true);
    editor->SetAdditionalSelectionTyping(true);
    editor->ClearSelections();

    pos = editor->FindText(0, docLen, word, wxSTC_FIND_WHOLEWORD);
    bool first = true;
    while (pos != wxNOT_FOUND)
    {
        int endPos = pos + (int)word.Length();
        if (first)
        {
            editor->SetSelection(pos, endPos);
            first = false;
        }
        else
        {
            editor->AddSelection(endPos, pos);
        }
        pos = editor->FindText(endPos, docLen, word, wxSTC_FIND_WHOLEWORD);
    }
}

void LogicEditorPanel::JumpToMatchingBrace()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;

    int pos   = editor->GetCurrentPos();
    int match = editor->BraceMatch(pos);

    if (match == wxSTC_INVALID_POSITION && pos > 0)
        match = editor->BraceMatch(pos - 1);   // try char before caret

    if (match != wxSTC_INVALID_POSITION)
    {
        editor->GotoPos(match + 1);
        editor->EnsureCaretVisible();
    }
}

void LogicEditorPanel::SortSelectedLines()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor) return;

    int startLine = editor->LineFromPosition(editor->GetSelectionStart());
    int endLine   = editor->LineFromPosition(editor->GetSelectionEnd());

    // If selection ends at a line start, exclude that line
    if (endLine > startLine &&
        editor->PositionFromLine(endLine) == editor->GetSelectionEnd())
        --endLine;

    if (startLine >= endLine) return;

    // Collect lines
    wxArrayString lineTexts;
    for (int ln = startLine; ln <= endLine; ++ln)
    {
        wxString t = editor->GetLine(ln);
        // strip trailing \r\n
        while (!t.IsEmpty() && (t.Last() == '\r' || t.Last() == '\n'))
            t.RemoveLast();
        lineTexts.Add(t);
    }

    lineTexts.Sort();

    editor->BeginUndoAction();
    for (int ln = startLine; ln <= endLine; ++ln)
    {
        int lineStart = editor->PositionFromLine(ln);
        int lineEnd   = editor->GetLineEndPosition(ln);
        editor->SetSelection(lineStart, lineEnd);
        editor->ReplaceSelection(lineTexts[ln - startLine]);
    }
    editor->EndUndoAction();
}

int LogicEditorPanel::GetLineCount() const
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    return editor ? editor->GetLineCount() : 0;
}

//--
// Inline error decorations
//--
void LogicEditorPanel::ClearErrorDecorations()
{
    for (const EditorTab& t : tabs)
    {
        if (!t.editor) continue;
        t.editor->SetIndicatorCurrent(9);
        t.editor->IndicatorClearRange(0, t.editor->GetLength());
    }
}

void LogicEditorPanel::SetErrorDecorations(const std::vector<ErrorMark>& errors)
{
    ClearErrorDecorations();

    for (const EditorTab& t : tabs)
    {
        if (!t.editor) continue;

        // Configure indicator 9 - red squiggle underline
        t.editor->IndicatorSetStyle(9, wxSTC_INDIC_SQUIGGLE);
        t.editor->IndicatorSetForeground(9, wxColour(240, 80, 80));
        t.editor->SetIndicatorCurrent(9);

        // Apply to matching lines
        for (const ErrorMark& em : errors)
        {
            // Match if this tab's filePath ends with the error file name
            // (GHDL may emit relative paths)
            bool match = (!t.filePath.IsEmpty() && !em.filePath.IsEmpty()) &&
                         (t.filePath == em.filePath ||
                          t.filePath.EndsWith(em.filePath) ||
                          em.filePath.EndsWith(wxFileName(t.filePath).GetFullName()));

            if (!match) continue;

            int zeroLine = wxMax(0, em.line - 1);
            int lineStart = t.editor->PositionFromLine(zeroLine);
            int lineEnd   = t.editor->GetLineEndPosition(zeroLine);
            if (lineEnd > lineStart)
                t.editor->IndicatorFillRange(lineStart, lineEnd - lineStart);
        }
    }
}
