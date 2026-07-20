#include "new_project_dialog.h"

enum { ID_TemplateChoice = wxID_HIGHEST + 400 };

wxBEGIN_EVENT_TABLE(NewProjectDialog, wxDialog)
    EVT_CHOICE(ID_TemplateChoice, NewProjectDialog::OnTemplateChanged)
wxEND_EVENT_TABLE()

//--
// Descriptions - one per template (order must match choice list)
//--
const wxString NewProjectDialog::s_descriptions[] =
{
    "An empty project with a minimal top-level VHDL file.",

    "A synchronous 4-bit up-counter with synchronous reset.\n"
    "Ports: clk, rst, q(3:0).",

    "A simple 4-bit ALU supporting ADD, SUB, AND, OR.\n"
    "Ports: a(3:0), b(3:0), op(1:0), result(3:0), carry.",

    "A Moore-model traffic-light FSM (Red, Green, Yellow).\n"
    "Ports: clk, rst, red, green, yellow.",

    "A basic 8-N-1 UART transmitter at configurable baud.\n"
    "Ports: clk, tx_data(7:0), tx_start, tx, tx_busy.",

    "A D flip-flop with synchronous clock enable.\n"
    "Ports: clk, en, d, q.",

    "A BCD to 7-segment display decoder (active-low segments).\n"
    "Ports: bcd(3:0), seg(6:0).  Covers digits 0-9.",

    "A 1-bit full adder (combinational).\n"
    "Ports: a, b, cin, sum, cout.",

    "A JK flip-flop with synchronous reset.\n"
    "Ports: clk, rst, j, k, q, qn.",
};

//--
// Preview snippets - entity skeleton shown in dialog (uses PROJECT placeholder)
//--
const wxString NewProjectDialog::s_previews[] =
{
    // 0 - Blank
    "entity PROJECT is\n"
    "    port (\n"
    "        clk : in std_logic\n"
    "    );\n"
    "end entity PROJECT;\n"
    "\n"
    "architecture rtl of PROJECT is\n"
    "begin\n"
    "\n"
    "end architecture rtl;",

    // 1 - 4-bit Counter
    "entity PROJECT is\n"
    "    port (\n"
    "        clk : in  std_logic;\n"
    "        rst : in  std_logic;\n"
    "        q   : out std_logic_vector(3 downto 0)\n"
    "    );\n"
    "end entity PROJECT;\n"
    "\n"
    "-- Synchronous 4-bit up-counter with synchronous reset\n"
    "-- q increments on each rising clock edge when rst='0'",

    // 2 - 4-bit ALU
    "entity PROJECT is\n"
    "    port (\n"
    "        a, b   : in  std_logic_vector(3 downto 0);\n"
    "        op     : in  std_logic_vector(1 downto 0);\n"
    "        result : out std_logic_vector(3 downto 0);\n"
    "        carry  : out std_logic\n"
    "    );\n"
    "end entity PROJECT;\n"
    "\n"
    "-- op: 00=ADD  01=SUB  10=AND  11=OR",

    // 3 - Traffic Light FSM
    "-- States: ST_RED -> ST_GREEN -> ST_YELLOW -> ST_RED\n"
    "entity PROJECT is\n"
    "    port (\n"
    "        clk    : in  std_logic;\n"
    "        rst    : in  std_logic;\n"
    "        red    : out std_logic;\n"
    "        green  : out std_logic;\n"
    "        yellow : out std_logic\n"
    "    );\n"
    "end entity PROJECT;",

    // 4 - UART TX
    "entity PROJECT is\n"
    "    generic (\n"
    "        CLK_FREQ  : integer := 50_000_000;\n"
    "        BAUD_RATE : integer := 115_200\n"
    "    );\n"
    "    port (\n"
    "        clk      : in  std_logic;\n"
    "        tx_data  : in  std_logic_vector(7 downto 0);\n"
    "        tx_start : in  std_logic;\n"
    "        tx       : out std_logic;\n"
    "        tx_busy  : out std_logic\n"
    "    );\n"
    "end entity PROJECT;",

    // 5 - D Flip-Flop
    "entity PROJECT is\n"
    "    port (\n"
    "        clk : in  std_logic;  -- rising edge\n"
    "        en  : in  std_logic;  -- clock enable\n"
    "        d   : in  std_logic;\n"
    "        q   : out std_logic\n"
    "    );\n"
    "end entity PROJECT;\n"
    "\n"
    "-- q captures d on rising_edge(clk) when en='1'",

    // 6 - 7-Segment Decoder
    "entity PROJECT is\n"
    "    port (\n"
    "        bcd : in  std_logic_vector(3 downto 0);\n"
    "        seg : out std_logic_vector(6 downto 0)\n"
    "    );\n"
    "end entity PROJECT;\n"
    "\n"
    "-- seg(6..0) = segments a b c d e f g  (active-low)\n"
    "-- Combinational decoder, covers digits 0-9",

    // 7 - Full Adder
    "entity PROJECT is\n"
    "    port (\n"
    "        a    : in  std_logic;\n"
    "        b    : in  std_logic;\n"
    "        cin  : in  std_logic;\n"
    "        sum  : out std_logic;\n"
    "        cout : out std_logic\n"
    "    );\n"
    "end entity PROJECT;\n"
    "\n"
    "-- 1-bit full adder  (sum = a XOR b XOR cin)",

    // 8 - JK Flip-Flop
    "entity PROJECT is\n"
    "    port (\n"
    "        clk : in  std_logic;\n"
    "        rst : in  std_logic;  -- sync reset\n"
    "        j   : in  std_logic;\n"
    "        k   : in  std_logic;\n"
    "        q   : out std_logic;\n"
    "        qn  : out std_logic\n"
    "    );\n"
    "end entity PROJECT;\n"
    "\n"
    "-- JK=00 hold | 01 reset | 10 set | 11 toggle",
};

//--
// Constructor
//--
NewProjectDialog::NewProjectDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "New Project",
               wxDefaultPosition, wxSize(620, 520))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    //** Project Name **//
    wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* nameLabel = new wxStaticText(this, wxID_ANY, "Project Name:");
    nameLabel->SetMinSize(wxSize(105, -1));
    m_projectNameCtrl = new wxTextCtrl(this, wxID_ANY);
    nameSizer->Add(nameLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    nameSizer->Add(m_projectNameCtrl, 1, wxALL | wxEXPAND, 5);

    //** Directory **//
    wxBoxSizer* dirSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* dirLabel = new wxStaticText(this, wxID_ANY, "Directory:");
    dirLabel->SetMinSize(wxSize(105, -1));
    m_dirPicker = new wxDirPickerCtrl(this, wxID_ANY, "", "Select a directory");
    dirSizer->Add(dirLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    dirSizer->Add(m_dirPicker, 1, wxALL | wxEXPAND, 5);

    //** Template **//
    wxBoxSizer* tplSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* tplLabel = new wxStaticText(this, wxID_ANY, "Template:");
    tplLabel->SetMinSize(wxSize(105, -1));
    const wxString choices[] = {
        "Blank",
        "4-bit Counter",
        "4-bit ALU",
        "Traffic Light FSM",
        "UART TX",
        "D Flip-Flop",
        "7-Segment Decoder",
        "Full Adder",
        "JK Flip-Flop",
    };
    m_templateChoice = new wxChoice(this, ID_TemplateChoice,
                                    wxDefaultPosition, wxDefaultSize,
                                    9, choices);
    m_templateChoice->SetSelection(0);
    tplSizer->Add(tplLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    tplSizer->Add(m_templateChoice, 1, wxALL | wxEXPAND, 5);

    //** Description **//
    m_templateDesc = new wxStaticText(this, wxID_ANY, s_descriptions[0],
                                      wxDefaultPosition, wxSize(-1, 36),
                                      wxST_NO_AUTORESIZE);
    m_templateDesc->Wrap(580);

    //** Code preview **//
    wxStaticText* previewLabel = new wxStaticText(this, wxID_ANY, "Preview:");
    m_previewCtrl = new wxTextCtrl(this, wxID_ANY, s_previews[0],
                                   wxDefaultPosition, wxSize(-1, 160),
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP |
                                   wxHSCROLL | wxBORDER_SUNKEN);
    wxFont monoFont = wxFont(wxFontInfo(9).FaceName("Consolas").AntiAliased(true));
    if (!monoFont.IsOk())
        monoFont = wxFont(wxFontInfo(9).Family(wxFONTFAMILY_TELETYPE));
    m_previewCtrl->SetFont(monoFont);

    //** Buttons **//
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer(1);
    buttonSizer->Add(new wxButton(this, wxID_OK,     "Create"), 0, wxALL, 8);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 8);

    mainSizer->Add(nameSizer,      0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    mainSizer->Add(dirSizer,       0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
    mainSizer->Add(tplSizer,       0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
    mainSizer->Add(m_templateDesc, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    mainSizer->Add(previewLabel,   0, wxLEFT | wxTOP, 10);
    mainSizer->Add(m_previewCtrl,  1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);
    mainSizer->Add(buttonSizer,    0, wxEXPAND | wxTOP, 4);

    SetSizer(mainSizer);
    Centre();
}

//--
// Event handler
//--
void NewProjectDialog::OnTemplateChanged(wxCommandEvent& /*event*/)
{
    int sel = m_templateChoice->GetSelection();
    if (sel >= 0 && sel < 9)
    {
        m_templateDesc->SetLabel(s_descriptions[sel]);
        m_templateDesc->Wrap(580);
        m_previewCtrl->SetValue(s_previews[sel]);
    }
}

//--
// Accessors
//--
wxString NewProjectDialog::GetProjectName() const
{
    return m_projectNameCtrl->GetValue();
}

wxString NewProjectDialog::GetProjectDirectory() const
{
    return m_dirPicker->GetPath();
}

int NewProjectDialog::GetTemplateIndex() const
{
    return m_templateChoice->GetSelection();
}
