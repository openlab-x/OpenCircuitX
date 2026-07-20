#include "circuit_canvas.h"
#include "ui/canvas/circuit_canvas_private.h"
#include "ui/shell/app_theme.h"
#include <algorithm>
#include <cmath>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/textdlg.h>
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <climits>
#include <map>
#include <wx/file.h>

// Save / Load (.ocxschem)
// Format (v2):
//   GATE <id> <type_int> <x> <y> <in0> <in1> <in2> <in3> <out> <out2> <label>
//   WIRE <fromId> <fromPin> <toId> <toPin>
//--
bool CircuitCanvas::SaveCanvas(const wxString& filePath) const
{
    wxFileOutputStream fos(filePath);
    if (!fos.IsOk()) return false;

    wxTextOutputStream out(fos);
    out << "# OpenCircuitX Canvas v2\n";

    for (const Gate& g : gates)
    {
        out << wxString::Format(
            "GATE %d %d %d %d %d %d %d %d %d %d %s\n",
            g.id, (int)g.type,
            g.pos.x, g.pos.y,
            (int)g.inputState[0], (int)g.inputState[1],
            (int)g.inputState[2], (int)g.inputState[3],
            (int)g.outputState, (int)g.outputState2,
            g.label.IsEmpty() ? "-" : g.label.utf8_string().c_str());
    }

    for (const Wire& w : wires)
    {
        out << wxString::Format("WIRE %d %d %d %d\n",
            w.fromGateId, w.fromPinIndex, w.toGateId, w.toPinIndex);
    }

    for (const CanvasAnnotation& a : m_annotations)
    {
        // Text goes at the end - may contain spaces; the loader joins remaining tokens.
        wxString safeText = a.text.IsEmpty() ? "-" : a.text;
        out << wxString::Format("ANNOT %d %d %d %s\n",
            a.id, a.pos.x, a.pos.y, safeText);
    }

    return true;
}

bool CircuitCanvas::LoadCanvas(const wxString& filePath)
{
    wxTextFile tf;
    if (!tf.Open(filePath)) return false;

    gates.clear();
    wires.clear();
    m_annotations.clear();
    nextGateId    = 1;
    m_nextAnnotId = 1;

    for (wxString line = tf.GetFirstLine(); !tf.Eof(); line = tf.GetNextLine())
    {
        line = line.Trim();
        if (line.IsEmpty() || line.StartsWith("#")) continue;

        wxArrayString parts;
        wxStringTokenizer tok(line, " \t");
        while (tok.HasMoreTokens())
            parts.Add(tok.GetNextToken());
        if (parts.IsEmpty()) continue;

        if (parts[0] == "GATE" && parts.GetCount() >= 10)
        {
            Gate g;
            long v;
            parts[1].ToLong(&v); g.id           = (int)v;
            parts[2].ToLong(&v); g.type          = static_cast<GateType>(v);
            parts[3].ToLong(&v); g.pos.x         = (int)v;
            parts[4].ToLong(&v); g.pos.y         = (int)v;
            parts[5].ToLong(&v); g.inputState[0] = (v != 0);
            parts[6].ToLong(&v); g.inputState[1] = (v != 0);
            parts[7].ToLong(&v); g.inputState[2] = (v != 0);
            parts[8].ToLong(&v); g.inputState[3] = (v != 0);
            parts[9].ToLong(&v); g.outputState   = (v != 0);
            // v2 fields
            if (parts.GetCount() >= 11) {
                parts[10].ToLong(&v); g.outputState2 = (v != 0);
            } else {
                g.outputState2 = false;
            }
            if (parts.GetCount() >= 12 && parts[11] != "-")
                g.label = parts[11];
            gates.push_back(g);
            if (g.id >= nextGateId) nextGateId = g.id + 1;
        }
        else if (parts[0] == "WIRE" && parts.GetCount() >= 5)
        {
            // v2: WIRE fromId fromPin toId toPin
            Wire w;
            long v;
            parts[1].ToLong(&v); w.fromGateId   = (int)v;
            parts[2].ToLong(&v); w.fromPinIndex  = (int)v;
            parts[3].ToLong(&v); w.toGateId      = (int)v;
            parts[4].ToLong(&v); w.toPinIndex    = (int)v;
            wires.push_back(w);
        }
        else if (parts[0] == "WIRE" && parts.GetCount() == 4)
        {
            // v1 backwards compat: WIRE fromId toId pinIdx
            Wire w;
            long v;
            parts[1].ToLong(&v); w.fromGateId   = (int)v;
            w.fromPinIndex = 0;
            parts[2].ToLong(&v); w.toGateId     = (int)v;
            parts[3].ToLong(&v); w.toPinIndex   = (int)v;
            wires.push_back(w);
        }
        else if (parts[0] == "ANNOT" && parts.GetCount() >= 4)
        {
            CanvasAnnotation a;
            long v;
            parts[1].ToLong(&v); a.id    = (int)v;
            parts[2].ToLong(&v); a.pos.x = (int)v;
            parts[3].ToLong(&v); a.pos.y = (int)v;
            // Text is everything after the first 4 tokens (space-separated)
            if (parts.GetCount() >= 5)
            {
                wxString text;
                for (size_t i = 4; i < parts.GetCount(); ++i)
                    text += (i == 4 ? "" : " ") + parts[i];
                if (text != "-") a.text = text;
            }
            m_annotations.push_back(a);
            if (a.id >= m_nextAnnotId) m_nextAnnotId = a.id + 1;
        }
    }

    SimulateOnce();
    Refresh();
    return true;
}

//--
// VHDL export
//--
wxString CircuitCanvas::ExportToVHDL() const
{
    if (gates.empty())
        return "-- No gates on canvas.\n";

    wxString out;
    out += "library IEEE;\n";
    out += "use IEEE.STD_LOGIC_1164.ALL;\n\n";

    // Collect INPUT / OUTPUT ports
    wxString portLines;
    bool     hasPort = false;
    for (const Gate& g : gates)
    {
        if (g.type == GateType::INPUT)
        {
            wxString name = g.label.IsEmpty()
                ? wxString::Format("in_g%d", g.id) : g.label;
            portLines += "        " + name + " : in  std_logic;\n";
            hasPort = true;
        }
        else if (g.type == GateType::OUTPUT)
        {
            wxString name = g.label.IsEmpty()
                ? wxString::Format("out_g%d", g.id) : g.label;
            portLines += "        " + name + " : out std_logic;\n";
            hasPort = true;
        }
    }
    if (!portLines.IsEmpty())
    {
        // Remove trailing ;\n from last port and replace with \n
        portLines = portLines.Left(portLines.Length() - 2) + "\n";
    }

    out += "entity circuit is\n";
    if (hasPort)
        out += "    port (\n" + portLines + "    );\n";
    else
        out += "    -- No INPUT/OUTPUT ports defined\n";
    out += "end circuit;\n\n";
    out += "architecture structural of circuit is\n\n";

    // Internal signals
    for (const Gate& g : gates)
    {
        if (g.type == GateType::INPUT || g.type == GateType::OUTPUT)
            continue;
        out += wxString::Format(
            "    signal g%d_out : std_logic := '0';\n", g.id);
        if (g.GetOutputCount() >= 2)
            out += wxString::Format(
                "    signal g%d_out2 : std_logic := '0';\n", g.id);
        for (int i = 0; i < g.GetInputCount(); ++i)
            out += wxString::Format(
                "    signal g%d_in%d : std_logic := '0';\n", g.id, i);
    }

    out += "\nbegin\n\n";

    // Wire assignments
    for (const Wire& w : wires)
    {
        const Gate* from = FindGate(w.fromGateId);
        const Gate* to   = FindGate(w.toGateId);
        if (!from || !to) continue;

        wxString src;
        if (from->type == GateType::INPUT)
            src = from->label.IsEmpty()
                ? wxString::Format("in_g%d", from->id) : from->label;
        else
            src = (w.fromPinIndex == 0)
                ? wxString::Format("g%d_out",  from->id)
                : wxString::Format("g%d_out2", from->id);

        wxString dst;
        if (to->type == GateType::OUTPUT)
            dst = to->label.IsEmpty()
                ? wxString::Format("out_g%d", to->id) : to->label;
        else
            dst = wxString::Format("g%d_in%d", to->id, w.toPinIndex);

        out += "    " + dst + " <= " + src + ";\n";
    }
    out += "\n";

    // Gate logic
    for (const Gate& g : gates)
    {
        if (g.type == GateType::INPUT || g.type == GateType::OUTPUT)
            continue;

        wxString a  = wxString::Format("g%d_in0", g.id);
        wxString b  = wxString::Format("g%d_in1", g.id);
        wxString ci = wxString::Format("g%d_in2", g.id);
        wxString o  = wxString::Format("g%d_out",  g.id);
        wxString o2 = wxString::Format("g%d_out2", g.id);

        switch (g.type)
        {
        case GateType::AND:  out += "    " + o + " <= " + a + " and " + b + ";\n"; break;
        case GateType::OR:   out += "    " + o + " <= " + a + " or "  + b + ";\n"; break;
        case GateType::NOT:  out += "    " + o + " <= not " + a + ";\n"; break;
        case GateType::NAND: out += "    " + o + " <= not (" + a + " and " + b + ");\n"; break;
        case GateType::NOR:  out += "    " + o + " <= not (" + a + " or "  + b + ");\n"; break;
        case GateType::XOR:  out += "    " + o + " <= " + a + " xor " + b + ";\n"; break;
        case GateType::XNOR: out += "    " + o + " <= not (" + a + " xor " + b + ");\n"; break;
        case GateType::MUX:
            out += "    " + o + " <= " + b + " when " + ci + " = '1' else " + a + ";\n"; break;
        case GateType::HALFADDER:
            out += "    " + o  + " <= " + a + " xor " + b + ";         -- Sum\n";
            out += "    " + o2 + " <= " + a + " and " + b + ";         -- Carry\n"; break;
        case GateType::FULLADDER:
            out += "    " + o  + " <= " + a + " xor " + b + " xor " + ci + ";   -- Sum\n";
            out += "    " + o2 + " <= (" + a + " and " + b + ") or (" + ci
                        + " and (" + a + " xor " + b + "));  -- Carry\n"; break;
        case GateType::DFLIPFLOP:
        {
            wxString clk = wxString::Format("g%d_in1", g.id);
            out += "    process(" + clk + ") begin\n";
            out += "        if rising_edge(" + clk + ") then\n";
            out += "            " + o  + " <= " + a + ";\n";
            out += "            " + o2 + " <= not " + a + ";\n";
            out += "        end if;\n";
            out += "    end process;\n";
            break;
        }
        case GateType::SRLATCH:
        {
            out += "    process(" + a + ", " + b + ") begin\n";
            out += "        if " + a + " = '1' then\n";
            out += "            " + o  + " <= '1';\n";
            out += "            " + o2 + " <= '0';\n";
            out += "        elsif " + b + " = '1' then\n";
            out += "            " + o  + " <= '0';\n";
            out += "            " + o2 + " <= '1';\n";
            out += "        end if;\n";
            out += "    end process;\n";
            break;
        }
        case GateType::JKFLIPFLOP:
        {
            wxString clk = wxString::Format("g%d_in2", g.id);
            wxString j   = wxString::Format("g%d_in0", g.id);
            wxString k   = wxString::Format("g%d_in1", g.id);
            out += "    process(" + clk + ") begin\n";
            out += "        if rising_edge(" + clk + ") then\n";
            out += "            if " + j + " = '1' and " + k + " = '0' then\n";
            out += "                " + o + " <= '1'; " + o2 + " <= '0';\n";
            out += "            elsif " + j + " = '0' and " + k + " = '1' then\n";
            out += "                " + o + " <= '0'; " + o2 + " <= '1';\n";
            out += "            elsif " + j + " = '1' and " + k + " = '1' then\n";
            out += "                " + o + " <= not " + o + ";\n";
            out += "                " + o2 + " <= not " + o2 + ";\n";
            out += "            end if;\n";
            out += "        end if;\n";
            out += "    end process;\n";
            break;
        }
        case GateType::TFLIPFLOP:
        {
            wxString clk = wxString::Format("g%d_in1", g.id);
            wxString t   = wxString::Format("g%d_in0", g.id);
            out += "    process(" + clk + ") begin\n";
            out += "        if rising_edge(" + clk + ") then\n";
            out += "            if " + t + " = '1' then\n";
            out += "                " + o  + " <= not " + o  + ";\n";
            out += "                " + o2 + " <= not " + o2 + ";\n";
            out += "            end if;\n";
            out += "        end if;\n";
            out += "    end process;\n";
            break;
        }
        case GateType::CLOCK:
            out += "    -- CLK g" + wxString::Format("%d", g.id)
                + " (connect clock port to " + o + ")\n";
            out += "    " + o + " <= '0';\n"; break;
        case GateType::TRISTATE:
            out += "    " + o + " <= " + a + " when " + b + " = '1' else 'Z';\n"; break;
        case GateType::DEMUX:
            out += "    " + o  + " <= " + a + " when " + b + " = '0' else '0';  -- Y0\n";
            out += "    " + o2 + " <= " + a + " when " + b + " = '1' else '0';  -- Y1\n"; break;
        default: break;
        }
    }

    out += "\nend structural;\n";
    return out;
}

//--
// Verilog export
//--
wxString CircuitCanvas::ExportToVerilog() const
{
    if (gates.empty())
        return "// No gates on canvas.\n";

    wxString out;
    out += "module circuit (\n";

    // Ports
    wxString portList;
    for (const Gate& g : gates)
    {
        if (g.type == GateType::INPUT)
        {
            wxString name = g.label.IsEmpty()
                ? wxString::Format("in_g%d", g.id) : g.label;
            portList += "    input  wire " + name + ",\n";
        }
        else if (g.type == GateType::OUTPUT)
        {
            wxString name = g.label.IsEmpty()
                ? wxString::Format("out_g%d", g.id) : g.label;
            portList += "    output reg  " + name + ",\n";
        }
    }
    if (!portList.IsEmpty())
        portList = portList.Left(portList.Length() - 2) + "\n"; // strip last comma
    else
        portList = "    // no ports\n";
    out += portList + ");\n\n";

    // Internal wires
    for (const Gate& g : gates)
    {
        if (g.type == GateType::INPUT || g.type == GateType::OUTPUT) continue;
        out += wxString::Format("wire g%d_out;\n", g.id);
        if (g.GetOutputCount() >= 2)
            out += wxString::Format("wire g%d_out2;\n", g.id);
        for (int i = 0; i < g.GetInputCount(); ++i)
            out += wxString::Format("wire g%d_in%d;\n", g.id, i);
    }
    out += "\n";

    // Assignments
    for (const Wire& w : wires)
    {
        const Gate* from = FindGate(w.fromGateId);
        const Gate* to   = FindGate(w.toGateId);
        if (!from || !to) continue;

        wxString src;
        if (from->type == GateType::INPUT)
            src = from->label.IsEmpty() ? wxString::Format("in_g%d", from->id) : from->label;
        else
            src = (w.fromPinIndex == 0)
                ? wxString::Format("g%d_out",  from->id)
                : wxString::Format("g%d_out2", from->id);

        wxString dst;
        if (to->type == GateType::OUTPUT)
            dst = to->label.IsEmpty() ? wxString::Format("out_g%d", to->id) : to->label;
        else
            dst = wxString::Format("g%d_in%d", to->id, w.toPinIndex);

        out += "assign " + dst + " = " + src + ";\n";
    }
    out += "\n";

    // Gate logic
    for (const Gate& g : gates)
    {
        if (g.type == GateType::INPUT || g.type == GateType::OUTPUT) continue;

        wxString a  = wxString::Format("g%d_in0", g.id);
        wxString b  = wxString::Format("g%d_in1", g.id);
        wxString ci = wxString::Format("g%d_in2", g.id);
        wxString o  = wxString::Format("g%d_out",  g.id);
        wxString o2 = wxString::Format("g%d_out2", g.id);

        switch (g.type)
        {
        case GateType::AND:  out += "assign " + o + " = " + a + " & " + b + ";\n"; break;
        case GateType::OR:   out += "assign " + o + " = " + a + " | " + b + ";\n"; break;
        case GateType::NOT:  out += "assign " + o + " = ~" + a + ";\n"; break;
        case GateType::NAND: out += "assign " + o + " = ~(" + a + " & " + b + ");\n"; break;
        case GateType::NOR:  out += "assign " + o + " = ~(" + a + " | " + b + ");\n"; break;
        case GateType::XOR:  out += "assign " + o + " = " + a + " ^ " + b + ";\n"; break;
        case GateType::XNOR: out += "assign " + o + " = ~(" + a + " ^ " + b + ");\n"; break;
        case GateType::MUX:
            out += "assign " + o + " = " + ci + " ? " + b + " : " + a + ";\n"; break;
        case GateType::HALFADDER:
            out += "assign " + o  + " = " + a + " ^ " + b + ";  // Sum\n";
            out += "assign " + o2 + " = " + a + " & " + b + ";  // Carry\n"; break;
        case GateType::FULLADDER:
            out += "assign " + o  + " = " + a + " ^ " + b + " ^ " + ci + ";  // Sum\n";
            out += "assign " + o2 + " = (" + a + " & " + b + ") | (" + ci
                        + " & (" + a + " ^ " + b + "));  // Carry\n"; break;
        case GateType::DFLIPFLOP:
        {
            wxString clk = wxString::Format("g%d_in1", g.id);
            out += "always @(posedge " + clk + ") begin\n";
            out += "    " + o  + " <= " + a + ";\n";
            out += "    " + o2 + " <= ~" + a + ";\n";
            out += "end\n";
            break;
        }
        case GateType::SRLATCH:
        {
            out += "always @(*) begin\n";
            out += "    if (" + a + ") begin " + o + " = 1'b1; " + o2 + " = 1'b0; end\n";
            out += "    else if (" + b + ") begin " + o + " = 1'b0; " + o2 + " = 1'b1; end\n";
            out += "end\n";
            break;
        }
        case GateType::JKFLIPFLOP:
        {
            wxString clk = wxString::Format("g%d_in2", g.id);
            wxString j   = wxString::Format("g%d_in0", g.id);
            wxString k   = wxString::Format("g%d_in1", g.id);
            out += "always @(posedge " + clk + ") begin\n";
            out += "    if (" + j + " && !" + k + ") begin " + o + " <= 1'b1; " + o2 + " <= 1'b0; end\n";
            out += "    else if (!" + j + " && " + k + ") begin " + o + " <= 1'b0; " + o2 + " <= 1'b1; end\n";
            out += "    else if (" + j + " && " + k + ") begin " + o + " <= ~" + o + "; " + o2 + " <= ~" + o2 + "; end\n";
            out += "end\n";
            break;
        }
        case GateType::TFLIPFLOP:
        {
            wxString clk = wxString::Format("g%d_in1", g.id);
            wxString t   = wxString::Format("g%d_in0", g.id);
            out += "always @(posedge " + clk + ") begin\n";
            out += "    if (" + t + ") begin " + o + " <= ~" + o + "; " + o2 + " <= ~" + o2 + "; end\n";
            out += "end\n";
            break;
        }
        case GateType::CLOCK:
            out += "// CLK g" + wxString::Format("%d", g.id) + "\n"; break;
        case GateType::TRISTATE:
            out += "assign " + o + " = " + b + " ? " + a + " : 1'bz;  // tri-state\n"; break;
        case GateType::DEMUX:
            out += "assign " + o  + " = ~" + b + " ? " + a + " : 1'b0;  // Y0\n";
            out += "assign " + o2 + " =  " + b + " ? " + a + " : 1'b0;  // Y1\n"; break;
        default: break;
        }
    }

    out += "\nendmodule\n";
    return out;
}
