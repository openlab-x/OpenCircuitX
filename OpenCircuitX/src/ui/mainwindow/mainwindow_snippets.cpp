#include "mainwindow.h"
#include "ui/editor/logic_editor_panel.h"

//--
// Code snippet data - VHDL (indices 0-5) + Verilog (indices 6-9)
//--
static const wxString s_vhdlSnippets[] =
{
    // 0 - Process (clk + reset)
    "process(clk)\nbegin\n"
    "    if rising_edge(clk) then\n"
    "        if rst = '1' then\n"
    "            -- synchronous reset\n"
    "        else\n"
    "            -- sequential logic here\n"
    "        end if;\n"
    "    end if;\n"
    "end process;\n",

    // 1 - Moore State Machine
    "type t_state is (ST_IDLE, ST_A, ST_B);\n"
    "signal state, next_state : t_state := ST_IDLE;\n\n"
    "-- State register\n"
    "process(clk)\nbegin\n"
    "    if rising_edge(clk) then\n"
    "        if rst = '1' then state <= ST_IDLE;\n"
    "        else             state <= next_state;\n"
    "        end if;\n"
    "    end if;\n"
    "end process;\n\n"
    "-- Next-state logic\n"
    "process(state)\nbegin\n"
    "    next_state <= state;\n"
    "    case state is\n"
    "        when ST_IDLE => next_state <= ST_A;\n"
    "        when ST_A    => next_state <= ST_B;\n"
    "        when ST_B    => next_state <= ST_IDLE;\n"
    "    end case;\n"
    "end process;\n",

    // 2 - Component Instantiation
    "u_inst : component_name\n"
    "    generic map (\n"
    "        WIDTH => 8\n"
    "    )\n"
    "    port map (\n"
    "        clk  => clk,\n"
    "        rst  => rst,\n"
    "        din  => sig_in,\n"
    "        dout => sig_out\n"
    "    );\n",

    // 3 - Generate...For
    "gen_label : for i in 0 to WIDTH-1 generate\n"
    "    -- replicated logic\n"
    "end generate gen_label;\n",

    // 4 - Function body
    "function fn_name(arg : std_logic_vector) return std_logic is\n"
    "    variable result : std_logic := '0';\n"
    "begin\n"
    "    -- function body\n"
    "    return result;\n"
    "end function fn_name;\n",

    // 5 - Package
    "package pkg_name is\n"
    "    -- type declarations\n"
    "    -- constant declarations\n"
    "    -- function prototypes\n"
    "end package pkg_name;\n\n"
    "package body pkg_name is\n"
    "    -- function implementations\n"
    "end package body pkg_name;\n"
};

static const wxString s_verilogSnippets[] =
{
    // 6 - Always @(posedge clk)
    "always @(posedge clk or posedge rst) begin\n"
    "    if (rst) begin\n"
    "        // synchronous reset\n"
    "    end else begin\n"
    "        // sequential logic\n"
    "    end\n"
    "end\n",

    // 7 - Case State Machine
    "localparam ST_IDLE = 2'd0, ST_A = 2'd1, ST_B = 2'd2;\n"
    "reg [1:0] state, next_state;\n\n"
    "always @(posedge clk or posedge rst)\n"
    "    if (rst) state <= ST_IDLE;\n"
    "    else     state <= next_state;\n\n"
    "always @(*) begin\n"
    "    next_state = state;\n"
    "    case (state)\n"
    "        ST_IDLE: next_state = ST_A;\n"
    "        ST_A:    next_state = ST_B;\n"
    "        ST_B:    next_state = ST_IDLE;\n"
    "        default: next_state = ST_IDLE;\n"
    "    endcase\n"
    "end\n",

    // 8 - Task
    "task task_name;\n"
    "    input [7:0] arg_in;\n"
    "    output reg  arg_out;\n"
    "    begin\n"
    "        // task body\n"
    "        arg_out = arg_in;\n"
    "    end\n"
    "endtask\n",

    // 9 - Generate...For
    "genvar i;\n"
    "generate\n"
    "    for (i = 0; i < WIDTH; i = i + 1) begin : gen_label\n"
    "        // replicated logic\n"
    "    end\n"
    "endgenerate\n"
};

void MainWindow::OnInsertSnippet(wxCommandEvent& event)
{
    int idx = event.GetId() - ID_SnippetFirst;
    wxString text;
    if (idx >= 0 && idx <= 5)
        text = s_vhdlSnippets[idx];
    else if (idx >= 6 && idx <= 9)
        text = s_verilogSnippets[idx - 6];
    else
        return;

    workspaceNotebook->SetSelection(0);
    logicEditor->InsertSnippet(text);
}
