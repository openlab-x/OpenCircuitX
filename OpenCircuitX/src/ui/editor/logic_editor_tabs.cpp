#include "logic_editor_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>

void LogicEditorPanel::LoadFile(const wxString& filePath)
{
    int existing = FindTabByPath(filePath);
    if (existing != -1)
    {
        notebook->SetSelection(existing);
        return;
    }

    wxFileInputStream input(filePath);
    if (!input.IsOk())
    {
        wxMessageBox("Could not open file: " + filePath, "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxTextInputStream text(input);
    wxString content;
    while (!input.Eof())
        content += text.ReadLine() + "\n";

    wxStyledTextCtrl* editor = CreateEditor();
    ApplyLexer(editor, wxFileName(filePath).GetExt());

    m_loadingFile = true;
    editor->SetText(content);
    editor->EmptyUndoBuffer();
    editor->SetSavePoint();   // marks doc clean - fires EVT_SAVEPOINTREACHED (suppressed)
    m_loadingFile = false;

    wxString label = wxFileName(filePath).GetFullName();
    notebook->AddPage(editor, label, true);

    EditorTab tab;
    tab.editor   = editor;
    tab.filePath = filePath;
    tabs.push_back(tab);
}

void LogicEditorPanel::SaveCurrentFileAs(const wxString& newPath)
{
    int sel = notebook->GetSelection();
    if (sel == wxNOT_FOUND || sel < 0 || sel >= (int)tabs.size())
        return;
    wxStyledTextCtrl* editor = tabs[sel].editor;
    if (!editor) return;

    wxFileOutputStream output(newPath);
    if (!output.IsOk())
    {
        wxMessageBox("Could not write file: " + newPath, "Save As", wxOK | wxICON_ERROR);
        return;
    }
    wxTextOutputStream text(output);
    text << editor->GetText();

    tabs[sel].filePath = newPath;
    notebook->SetPageText(sel, wxFileName(newPath).GetFullName());
    ApplyLexer(editor, wxFileName(newPath).GetExt());
    editor->SetSavePoint();
}

void LogicEditorPanel::NewTabWithContent(const wxString& tabName, const wxString& content)
{
    wxStyledTextCtrl* editor = CreateEditor();
    ApplyLexer(editor, wxFileName(tabName).GetExt());

    editor->SetText(content);
    editor->EmptyUndoBuffer();
    // Intentionally no SetSavePoint() - tab opens as modified (*) to prompt save

    notebook->AddPage(editor, tabName + " *", true);

    EditorTab tab;
    tab.editor   = editor;
    tab.filePath = wxEmptyString; // no backing file yet
    tabs.push_back(tab);
}

void LogicEditorPanel::SaveCurrentFile()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor)
        return;

    int sel = notebook->GetSelection();
    if (sel == wxNOT_FOUND || sel < 0 || sel >= (int)tabs.size())
        return;

    wxString filePath = tabs[sel].filePath;
    if (filePath.IsEmpty())
        return;

    wxFileOutputStream output(filePath);
    if (!output.IsOk())
    {
        wxMessageBox("Could not save file: " + filePath, "Error", wxOK | wxICON_ERROR);
        return;
    }

    wxTextOutputStream text(output);
    text << editor->GetText();

    editor->SetSavePoint();
}

int LogicEditorPanel::SaveAllModified()
{
    int saved = 0;
    for (auto& tab : tabs)
    {
        if (tab.filePath.IsEmpty()) continue;
        if (!tab.editor) continue;
        if (tab.editor->GetModify())
        {
            wxFileOutputStream output(tab.filePath);
            if (output.IsOk())
            {
                wxTextOutputStream text(output);
                text << tab.editor->GetText();
                tab.editor->SetSavePoint();
                ++saved;
            }
        }
    }
    return saved;
}

bool LogicEditorPanel::ReopenLastClosedTab()
{
    if (m_closedTabs.empty()) return false;

    ClosedTab snap = m_closedTabs.back();
    m_closedTabs.pop_back();

    // If the file is already open, just switch to it
    if (!snap.filePath.IsEmpty())
    {
        int existing = FindTabByPath(snap.filePath);
        if (existing != -1)
        {
            notebook->SetSelection(existing);
            return true;
        }
    }

    // Recreate the tab
    wxStyledTextCtrl* editor = CreateEditor();
    wxString tabName = snap.tabName.IsEmpty() ? "Untitled" : snap.tabName;
    notebook->AddPage(editor, tabName, true);

    EditorTab et;
    et.editor   = editor;
    et.filePath = snap.filePath;
    tabs.push_back(et);

    // Apply lexer from extension
    if (!snap.filePath.IsEmpty())
        ApplyLexer(editor, wxFileName(snap.filePath).GetExt().Lower());

    editor->SetText(snap.content);
    editor->SetSavePoint();  // treat as unmodified (content matches last-known state)
    editor->GotoPos(0);
    return true;
}

wxString LogicEditorPanel::GetCode() const
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (!editor)
        return wxEmptyString;
    return editor->GetText();
}

void LogicEditorPanel::SetCode(const wxString& code)
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor)
        editor->SetText(code);
}

void LogicEditorPanel::Undo()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor) editor->Undo();
}

void LogicEditorPanel::Redo()
{
    wxStyledTextCtrl* editor = GetCurrentEditor();
    if (editor) editor->Redo();
}

wxString LogicEditorPanel::GetCurrentFilePath() const
{
    int sel = notebook->GetSelection();
    if (sel == wxNOT_FOUND || sel < 0 || sel >= (int)tabs.size())
        return wxEmptyString;
    return tabs[sel].filePath;
}

wxArrayString LogicEditorPanel::GetOpenFilePaths() const
{
    wxArrayString paths;
    for (const EditorTab& tab : tabs)
        paths.Add(tab.filePath);
    return paths;
}

wxStyledTextCtrl* LogicEditorPanel::GetCurrentEditor() const
{
    int sel = notebook->GetSelection();
    if (sel == wxNOT_FOUND || sel < 0 || sel >= (int)tabs.size())
        return nullptr;
    return tabs[sel].editor;
}

int LogicEditorPanel::FindTabByPath(const wxString& filePath) const
{
    for (int i = 0; i < (int)tabs.size(); ++i)
    {
        if (tabs[i].filePath == filePath)
            return i;
    }
    return -1;
}

void LogicEditorPanel::UpdateTabLabel(wxStyledTextCtrl* editor, bool modified)
{
    for (int i = 0; i < (int)tabs.size(); ++i)
    {
        if (tabs[i].editor == editor)
        {
            wxString label = wxFileName(tabs[i].filePath).GetFullName();
            notebook->SetPageText(i, modified ? label + " *" : label);
            break;
        }
    }
}

void LogicEditorPanel::OnTabClose(wxAuiNotebookEvent& event)
{
    int sel = event.GetSelection();
    if (sel >= 0 && sel < (int)tabs.size())
    {
        // Save snapshot before erasing so Ctrl+Shift+T can restore it
        ClosedTab snap;
        snap.tabName  = notebook->GetPageText(sel);
        snap.filePath = tabs[sel].filePath;
        snap.content  = tabs[sel].editor ? tabs[sel].editor->GetText() : wxString();
        // Strip the modified asterisk from the tab name
        if (snap.tabName.EndsWith("*"))
            snap.tabName = snap.tabName.Left(snap.tabName.Length() - 1).Trim(true);
        m_closedTabs.push_back(snap);

        tabs.erase(tabs.begin() + sel);
    }
    event.Skip();
}

void LogicEditorPanel::OnEditorModified(wxStyledTextEvent& event)
{
    UpdateTabLabel(static_cast<wxStyledTextCtrl*>(event.GetEventObject()), true);
    event.Skip();
}

void LogicEditorPanel::OnEditorSaved(wxStyledTextEvent& event)
{
    UpdateTabLabel(static_cast<wxStyledTextCtrl*>(event.GetEventObject()), false);
    event.Skip();

    // Do NOT fire the save callback when the event was triggered by LoadFile.
    // LoadFile calls SetSavePoint() to mark the new doc as clean - that is not
    // a user save and should not trigger live syntax checking.
    if (m_loadingFile) return;

    // Fire save callback so MainWindow can trigger live syntax check
    if (m_saveCallback)
    {
        wxString path = GetCurrentFilePath();
        wxString ext  = wxFileName(path).GetExt().Lower();
        if (!path.IsEmpty())
            m_saveCallback(path, ext);
    }
}

void LogicEditorPanel::SetSaveCallback(
    std::function<void(const wxString&, const wxString&)> cb)
{
    m_saveCallback = cb;
}

void LogicEditorPanel::SetSignalValueCallback(std::function<wxString(const wxString&)> cb)
{
    m_signalValueCallback = cb;
}

void LogicEditorPanel::SetAutoCompleteSymbols(const wxArrayString& symbols)
{
    m_extraSymbols = symbols;
}

void LogicEditorPanel::CloseAllTabs()
{
    // Delete pages back-to-front so notebook indices stay valid.
    for (int i = (int)tabs.size() - 1; i >= 0; --i)
        notebook->DeletePage(i);
    tabs.clear();
    m_closedTabs.clear();
}

void LogicEditorPanel::CloseOtherTabs(int keepIdx)
{
    // Iterate backwards so indices stay valid after each removal
    for (int i = (int)tabs.size() - 1; i >= 0; --i)
    {
        if (i == keepIdx)
            continue;

        // Save snapshot for Ctrl+Shift+T
        ClosedTab snap;
        snap.tabName  = notebook->GetPageText(i);
        snap.filePath = tabs[i].filePath;
        snap.content  = tabs[i].editor ? tabs[i].editor->GetText() : wxString();
        if (snap.tabName.EndsWith("*"))
            snap.tabName = snap.tabName.Left(snap.tabName.Length() - 1).Trim(true);
        m_closedTabs.push_back(snap);

        tabs.erase(tabs.begin() + i);
        notebook->DeletePage(i);
    }
}

void LogicEditorPanel::OnTabRightClick(wxAuiNotebookEvent& event)
{
    int idx = event.GetSelection();
    if (idx < 0 || idx >= (int)tabs.size())
        return;

    wxString filePath = tabs[idx].filePath;

    enum { ID_CloseTab = wxID_HIGHEST + 900, ID_CloseOthers, ID_RevealExplorer, ID_CopyPath };

    wxMenu menu;
    menu.Append(ID_CloseTab,      "Close Tab");
    menu.Append(ID_CloseOthers,   "Close Other Tabs");
    if (!filePath.IsEmpty())
    {
        menu.AppendSeparator();
        menu.Append(ID_RevealExplorer, "Reveal in Explorer");
        menu.Append(ID_CopyPath,       "Copy Full Path");
    }

    int chosen = GetPopupMenuSelectionFromUser(menu);
    switch (chosen)
    {
    case ID_CloseTab:
        // Simulate the close-tab button click
        {
            wxAuiNotebookEvent fake(wxEVT_AUINOTEBOOK_PAGE_CLOSE, notebook->GetId());
            fake.SetSelection(idx);
            OnTabClose(fake);
            notebook->DeletePage(idx);
        }
        break;

    case ID_CloseOthers:
        CloseOtherTabs(idx);
        break;

    case ID_RevealExplorer:
#ifdef __WXMSW__
        wxExecute("explorer.exe /select,\"" + filePath + "\"", wxEXEC_ASYNC);
#else
        wxExecute("xdg-open \"" + wxFileName(filePath).GetPath() + "\"", wxEXEC_ASYNC);
#endif
        break;

    case ID_CopyPath:
        if (wxTheClipboard->Open())
        {
            wxTheClipboard->SetData(new wxTextDataObject(filePath));
            wxTheClipboard->Close();
        }
        break;

    default:
        break;
    }
}

