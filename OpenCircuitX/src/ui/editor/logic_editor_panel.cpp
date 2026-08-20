#include "logic_editor_panel.h"
#include "ui/shell/app_theme.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>
#include "ui/editor/logic_editor_private.h"

LogicEditorPanel::LogicEditorPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    SetBackgroundColour(OCXTheme::BgPanel());

    int nbStyle = wxAUI_NB_TOP
                | wxAUI_NB_CLOSE_ON_ALL_TABS
                | wxAUI_NB_TAB_MOVE
                | wxAUI_NB_SCROLL_BUTTONS;

    notebook = new wxAuiNotebook(this, wxID_ANY,
                                  wxDefaultPosition, wxDefaultSize,
                                  nbStyle);

    notebook->SetBackgroundColour(OCXTheme::BgPanel());

    notebook->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE,
                   &LogicEditorPanel::OnTabClose, this);
    notebook->Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_DOWN,
                   &LogicEditorPanel::OnTabRightClick, this);

    //** Find / Replace bar (hidden by default) **//
    m_findBar = new wxPanel(this, wxID_ANY);
    m_findBar->SetBackgroundColour(OCXTheme::BgPanel());

    // -- Find row --
    wxStaticText* findLabel = new wxStaticText(m_findBar, wxID_ANY, "Find:");
    findLabel->SetForegroundColour(OCXTheme::FgDim());

    m_findField = new wxTextCtrl(m_findBar, wxID_ANY, "",
                                  wxDefaultPosition, wxSize(200, -1),
                                  wxTE_PROCESS_ENTER | wxBORDER_SIMPLE);
    m_findField->SetBackgroundColour(OCXTheme::BgEditor());
    m_findField->SetForegroundColour(OCXTheme::FgText());

    wxButton* btnNext  = new wxButton(m_findBar, wxID_FORWARD,  "Next",  wxDefaultPosition, wxSize(56, -1));
    wxButton* btnPrev  = new wxButton(m_findBar, wxID_BACKWARD, "Prev",  wxDefaultPosition, wxSize(56, -1));
    wxButton* btnClose = new wxButton(m_findBar, wxID_CLOSE,    "X",     wxDefaultPosition, wxSize(28, -1));

    // -- Replace row (hidden until Ctrl+H) --
    m_replaceRow = new wxPanel(m_findBar, wxID_ANY);
    m_replaceRow->SetBackgroundColour(OCXTheme::BgPanel());

    wxStaticText* replLabel = new wxStaticText(m_replaceRow, wxID_ANY, "Replace:");
    replLabel->SetForegroundColour(OCXTheme::FgDim());

    m_replaceField = new wxTextCtrl(m_replaceRow, wxID_ANY, "",
                                     wxDefaultPosition, wxSize(200, -1),
                                     wxTE_PROCESS_ENTER | wxBORDER_SIMPLE);
    m_replaceField->SetBackgroundColour(OCXTheme::BgEditor());
    m_replaceField->SetForegroundColour(OCXTheme::FgText());

    enum { ID_ReplaceOne = wxID_HIGHEST + 200, ID_ReplaceAll };
    wxButton* btnRepl    = new wxButton(m_replaceRow, ID_ReplaceOne, "Replace",     wxDefaultPosition, wxSize(72, -1));
    wxButton* btnReplAll = new wxButton(m_replaceRow, ID_ReplaceAll, "Replace All", wxDefaultPosition, wxSize(88, -1));

    wxBoxSizer* replSizer = new wxBoxSizer(wxHORIZONTAL);
    replSizer->AddSpacer(6);
    replSizer->Add(replLabel,      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    replSizer->Add(m_replaceField, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    replSizer->Add(btnRepl,        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    replSizer->Add(btnReplAll,     0, wxALIGN_CENTER_VERTICAL);
    m_replaceRow->SetSizer(replSizer);
    m_replaceRow->Show(false);

    btnRepl->Bind(wxEVT_BUTTON,    &LogicEditorPanel::OnReplaceOne, this);
    btnReplAll->Bind(wxEVT_BUTTON, &LogicEditorPanel::OnReplaceAll, this);
    m_replaceField->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&){ ReplaceOne(); });

    for (wxButton* b : { btnNext, btnPrev, btnClose })
    {
        b->SetBackgroundColour(OCXTheme::BgPanel());
        b->SetForegroundColour(OCXTheme::FgText());
    }

    wxBoxSizer* findRow = new wxBoxSizer(wxHORIZONTAL);
    findRow->AddSpacer(6);
    findRow->Add(findLabel,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    findRow->Add(m_findField, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    findRow->Add(btnNext,     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    findRow->Add(btnPrev,     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    findRow->Add(btnClose,    0, wxALIGN_CENTER_VERTICAL);
    findRow->AddSpacer(6);

    wxBoxSizer* fbSizer = new wxBoxSizer(wxVERTICAL);
    fbSizer->Add(findRow,     0, wxEXPAND | wxTOP | wxBOTTOM, 2);
    fbSizer->Add(m_replaceRow, 0, wxEXPAND | wxBOTTOM, 2);
    m_findBar->SetSizer(fbSizer);
    m_findBar->Show(false);

    btnNext->Bind(wxEVT_BUTTON,       &LogicEditorPanel::OnFindNext,  this);
    btnPrev->Bind(wxEVT_BUTTON,       &LogicEditorPanel::OnFindPrev,  this);
    btnClose->Bind(wxEVT_BUTTON,      &LogicEditorPanel::OnFindClose, this);
    m_findField->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&){ FindInEditor(true); });
    m_findField->Bind(wxEVT_KEY_DOWN, &LogicEditorPanel::OnFindKeyDown, this);

    m_rootSizer = new wxBoxSizer(wxVERTICAL);
    m_rootSizer->Add(notebook,  1, wxEXPAND);
    m_rootSizer->Add(m_findBar, 0, wxEXPAND);
    SetSizer(m_rootSizer);
}

void LogicEditorPanel::ApplyBaseStyles(wxStyledTextCtrl* editor)
{
    //** Base style **//
    editor->StyleSetBackground(wxSTC_STYLE_DEFAULT, OCXTheme::BgEditor());
    editor->StyleSetForeground(wxSTC_STYLE_DEFAULT, OCXTheme::FgText());
    editor->StyleSetFaceName(wxSTC_STYLE_DEFAULT, OCXTheme::EditorFontFace());
    editor->StyleSetSize(wxSTC_STYLE_DEFAULT, OCXTheme::EditorFontSize());
    editor->StyleClearAll();

    //** Line number margin **//
    editor->StyleSetBackground(wxSTC_STYLE_LINENUMBER, OCXTheme::BgMargin());
    editor->StyleSetForeground(wxSTC_STYLE_LINENUMBER, OCXTheme::FgDim());
    editor->StyleSetFaceName(wxSTC_STYLE_LINENUMBER, OCXTheme::EditorFontFace());
    editor->StyleSetSize(wxSTC_STYLE_LINENUMBER, OCXTheme::EditorFontSize() - 1);
    if (m_showLineNumbers)
        editor->SetMarginWidth(0, 52);

    //** Breakpoint margin background **//
    editor->SetMarginBackground(2, OCXTheme::BgMargin());

    //** Caret **//
    editor->SetCaretForeground(OCXTheme::FgCaret());

    //** Current line highlight **//
    editor->SetCaretLineVisible(true);
    editor->SetCaretLineBackground(OCXTheme::BgLineCur());

    //** Selection **//
    editor->SetSelBackground(true, OCXTheme::BgSelection());
    // false = don't override selected text's own syntax-highlight colour;
    // the colour argument is meant to be ignored in that case, but GTK's
    // SetSelForeground reads it unconditionally instead of checking the
    // flag first, so wxNullColour (deliberately IsOk()==false) crashes
    // there. Windows' implementation checks the flag first and never hits
    // this. Passing a real colour keeps both platforms safe; it's simply
    // unused on Windows and a sane fallback if GTK ever does apply it.
    editor->SetSelForeground(false, OCXTheme::FgText());

    //** Edge / whitespace **//
    editor->SetEdgeColour(OCXTheme::BgSash());
    editor->SetWhitespaceForeground(true, OCXTheme::FgDim());

    //** Auto-complete list colours **//
    editor->AutoCompSetMaxWidth(40);
    editor->StyleSetBackground(wxSTC_STYLE_DEFAULT, OCXTheme::BgEditor());

    //** Brace matching **//
    editor->StyleSetBackground(wxSTC_STYLE_BRACELIGHT, OCXTheme::Accent());
    editor->StyleSetForeground(wxSTC_STYLE_BRACELIGHT, OCXTheme::FgText());

    //** Indentation guides **//
    editor->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, OCXTheme::BgSash());
    editor->StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, OCXTheme::BgEditor());
}

wxStyledTextCtrl* LogicEditorPanel::CreateEditor()
{
    wxStyledTextCtrl* editor = new wxStyledTextCtrl(
        notebook, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxBORDER_NONE);

    ApplyBaseStyles(editor);

    //** Line number margin setup (geometry) **//
    editor->SetMarginWidth(0, 52);
    editor->SetMarginType(0, wxSTC_MARGIN_NUMBER);

    //** Second margin (separator) **//
    editor->SetMarginWidth(1, 4);
    editor->SetMarginType(1, wxSTC_MARGIN_FORE);
    editor->SetMarginBackground(1, OCXTheme::BgMargin());

    //** Breakpoint margin (margin 2) **//
    editor->SetMarginWidth(2, 16);
    editor->SetMarginType(2, wxSTC_MARGIN_SYMBOL);
    editor->SetMarginSensitive(2, true);
    editor->SetMarginBackground(2, OCXTheme::BgMargin());
    editor->MarkerDefine(1, wxSTC_MARK_CIRCLE);       // breakpoint
    editor->MarkerSetForeground(1, wxColour(220,  60,  60));
    editor->MarkerSetBackground(1, wxColour(200,  40,  40));

    editor->MarkerDefine(2, wxSTC_MARK_BOOKMARK);     // bookmark
    editor->MarkerSetForeground(2, wxColour( 80, 140, 255));
    editor->MarkerSetBackground(2, wxColour( 50, 110, 220));

    //** Caret **//
    editor->SetCaretWidth(2);

    //** Indentation **//
    editor->SetTabWidth(4);
    editor->SetUseTabs(false);
    editor->SetIndent(4);
    editor->SetIndentationGuides(wxSTC_IV_LOOKBOTH);

    //** EOL **//
    editor->SetEOLMode(wxSTC_EOL_CRLF);

    //** Events **//
    editor->Bind(wxEVT_STC_SAVEPOINTLEFT,    &LogicEditorPanel::OnEditorModified, this);
    editor->Bind(wxEVT_STC_SAVEPOINTREACHED, &LogicEditorPanel::OnEditorSaved,    this);
    editor->Bind(wxEVT_STC_MARGINCLICK,      &LogicEditorPanel::OnMarginClick,    this);
    editor->Bind(wxEVT_STC_CHARADDED,        &LogicEditorPanel::OnCharAdded,      this);

    // Caret position callback for the status bar
    editor->Bind(wxEVT_STC_UPDATEUI, [this](wxStyledTextEvent& e)
    {
        if (m_caretCallback)
        {
            wxStyledTextCtrl* ed = static_cast<wxStyledTextCtrl*>(e.GetEventObject());
            if (ed)
            {
                int line = ed->GetCurrentLine() + 1;
                int col  = ed->GetColumn(ed->GetCurrentPos()) + 1;
                int sel  = notebook->GetSelection();
                wxString ext;
                if (sel >= 0 && sel < (int)tabs.size())
                    ext = wxFileName(tabs[sel].filePath).GetExt().Upper();
                m_caretCallback(line, col, ext);
            }
        }
        e.Skip();
    });

    // Outline callback - fires on every text change for the active editor
    editor->Bind(wxEVT_STC_MODIFIED, [this](wxStyledTextEvent& ev) {
        if (m_outlineCallback)
        {
            wxStyledTextCtrl* ed = GetCurrentEditor();
            if (ed && ev.GetEventObject() == ed)
                m_outlineCallback(ed->GetText(), GetCurrentFileExt());
        }
        ev.Skip();
    });

    // Alt+Up / Alt+Down - move selected lines
    editor->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
        if (e.AltDown() && !e.ControlDown() && !e.ShiftDown())
        {
            if (e.GetKeyCode() == WXK_UP)   { MoveLineUp();   return; }
            if (e.GetKeyCode() == WXK_DOWN) { MoveLineDown(); return; }
        }
        e.Skip();
    });

    // Apply current word-wrap setting
    editor->SetWrapMode(m_wordWrap ? wxSTC_WRAP_WORD : wxSTC_WRAP_NONE);

    // Apply current whitespace visibility setting
    editor->SetViewWhiteSpace(m_showWhitespace ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);

    // Signal value tooltip on mouse dwell
    editor->SetMouseDwellTime(600);
    editor->Bind(wxEVT_STC_DWELLSTART, [this](wxStyledTextEvent& e) {
        wxStyledTextCtrl* ed = static_cast<wxStyledTextCtrl*>(e.GetEventObject());
        if (!ed || !m_signalValueCallback) { e.Skip(); return; }
        int pos = e.GetPosition();
        if (pos < 0) { e.Skip(); return; }
        int wordStart = ed->WordStartPosition(pos, true);
        int wordEnd   = ed->WordEndPosition(pos, true);
        if (wordEnd <= wordStart) { e.Skip(); return; }
        wxString word = ed->GetTextRange(wordStart, wordEnd);
        if (word.IsEmpty()) { e.Skip(); return; }
        wxString tip = m_signalValueCallback(word);
        if (!tip.IsEmpty())
            ed->CallTipShow(pos, tip);
        e.Skip();
    });
    editor->Bind(wxEVT_STC_DWELLEND, [this](wxStyledTextEvent& e) {
        wxStyledTextCtrl* ed = static_cast<wxStyledTextCtrl*>(e.GetEventObject());
        if (ed) ed->CallTipCancel();
        e.Skip();
    });

    return editor;
}

void LogicEditorPanel::SetCaretCallback(std::function<void(int, int, const wxString&)> cb)
{
    m_caretCallback = cb;
}

void LogicEditorPanel::ApplyLexer(wxStyledTextCtrl* editor,
                                   const wxString& ext)
{
    wxString e = ext.Lower();

    if (e == "pcf")
    {
        // iCE40 Physical Constraints File
        // Uses # comments and keywords: set_io, set_hd, set_freq, pin_config
        editor->SetLexer(wxSTC_LEX_BASH);
        editor->SetKeyWords(0, "set_io set_hd set_freq pin_config");

        editor->StyleSetForeground(wxSTC_SH_COMMENTLINE, OCXTheme::SynComment());
        editor->StyleSetForeground(wxSTC_SH_WORD,        OCXTheme::SynKeyword());
        editor->StyleSetForeground(wxSTC_SH_STRING,      OCXTheme::SynString());
        editor->StyleSetForeground(wxSTC_SH_NUMBER,      OCXTheme::SynNumber());
        editor->StyleSetForeground(wxSTC_SH_DEFAULT,     OCXTheme::FgText());
        for (int i = 0; i <= 20; ++i)
            editor->StyleSetBackground(i, OCXTheme::BgEditor());
    }
    else if (e == "lpf")
    {
        // ECP5 Lattice Preference File
        // Uses # comments and keywords: LOCATE, IOBUF, FREQUENCY, SYSCONFIG
        editor->SetLexer(wxSTC_LEX_BASH);
        editor->SetKeyWords(0,
            "LOCATE COMP SITE IOBUF PORT IO_TYPE PULLMODE FREQUENCY "
            "SYSCONFIG JTAG_PORT GSR_PORT MCCLK_FREQ CONFIG_IOVOLTAGE "
            "LVCMOS33 LVCMOS25 LVCMOS18 LVCMOS15 LVCMOS12 LVDS LVTTL "
            "IN OUT BIDIR MHZ KHZ");

        editor->StyleSetForeground(wxSTC_SH_COMMENTLINE, OCXTheme::SynComment());
        editor->StyleSetForeground(wxSTC_SH_WORD,        OCXTheme::SynKeyword());
        editor->StyleSetForeground(wxSTC_SH_STRING,      OCXTheme::SynString());
        editor->StyleSetForeground(wxSTC_SH_NUMBER,      OCXTheme::SynNumber());
        editor->StyleSetForeground(wxSTC_SH_DEFAULT,     OCXTheme::FgText());
        for (int i = 0; i <= 20; ++i)
            editor->StyleSetBackground(i, OCXTheme::BgEditor());
    }
    else if (e == "v" || e == "sv")
    {
        editor->SetLexer(wxSTC_LEX_VERILOG);
        editor->SetKeyWords(0, VERILOG_KEYWORDS);

        editor->StyleSetForeground(wxSTC_V_COMMENT,      OCXTheme::SynComment());
        editor->StyleSetForeground(wxSTC_V_COMMENTLINE,  OCXTheme::SynComment());
        editor->StyleSetForeground(wxSTC_V_STRING,       OCXTheme::SynString());
        editor->StyleSetForeground(wxSTC_V_WORD,         OCXTheme::SynKeyword());
        editor->StyleSetForeground(wxSTC_V_NUMBER,       OCXTheme::SynNumber());
        editor->StyleSetForeground(wxSTC_V_PREPROCESSOR, OCXTheme::SynPreproc());
        editor->StyleSetForeground(wxSTC_V_OPERATOR,     OCXTheme::SynOperator());
        editor->StyleSetForeground(wxSTC_V_IDENTIFIER,   OCXTheme::FgText());
    }
    else
    {
        editor->SetLexer(wxSTC_LEX_VHDL);
        editor->SetKeyWords(0, VHDL_KEYWORDS);

        editor->StyleSetForeground(wxSTC_VHDL_COMMENT,      OCXTheme::SynComment());
        editor->StyleSetForeground(wxSTC_VHDL_STRING,       OCXTheme::SynString());
        editor->StyleSetForeground(wxSTC_VHDL_KEYWORD,      OCXTheme::SynKeyword());
        editor->StyleSetForeground(wxSTC_VHDL_STDTYPE,      OCXTheme::SynType());
        editor->StyleSetForeground(wxSTC_VHDL_STDFUNCTION,  OCXTheme::SynFunction());
        editor->StyleSetForeground(wxSTC_VHDL_NUMBER,       OCXTheme::SynNumber());
        editor->StyleSetForeground(wxSTC_VHDL_ATTRIBUTE,    OCXTheme::SynFunction());
        editor->StyleSetForeground(wxSTC_VHDL_STDPACKAGE,   OCXTheme::SynType());
        editor->StyleSetForeground(wxSTC_VHDL_IDENTIFIER,   OCXTheme::FgText());
        editor->StyleSetForeground(wxSTC_VHDL_STDOPERATOR,  OCXTheme::SynOperator());

        // Background for all VHDL styles to stay dark
        for (int i = 0; i <= 15; ++i)
            editor->StyleSetBackground(i, OCXTheme::BgEditor());
    }
}

