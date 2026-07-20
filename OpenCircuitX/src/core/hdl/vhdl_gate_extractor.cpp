#include "vhdl_gate_extractor.h"

namespace VHDLGateExtractor
{

static GateType OpToType(const wxString& op)
{
    if (op == "xnor") return GateType::XNOR;
    if (op == "nand") return GateType::NAND;
    if (op == "nor")  return GateType::NOR;
    if (op == "xor")  return GateType::XOR;
    if (op == "and")  return GateType::AND;
    if (op == "or")   return GateType::OR;
    return GateType::AND;
}

RTLNetlist Extract(const wxString& vhdlText, const VHDLEntity& entity)
{
    RTLNetlist result;
    if (!entity.valid) return result;

    for (const auto& p : entity.ports)
    {
        if (p.dir == PortDir::In || p.dir == PortDir::InOut)
            result.inputPorts.Add(p.name.Lower());
        else
            result.outputPorts.Add(p.name.Lower());
    }

    // Flatten lines, strip comments
    wxArrayString rawLines = wxSplit(vhdlText, '\n');
    wxString flat;
    for (size_t i = 0; i < rawLines.GetCount(); ++i)
    {
        wxString ln = rawLines[i];
        int cm = ln.Find("--");
        if (cm != wxNOT_FOUND) ln = ln.Left(cm);
        ln.Trim(true).Trim(false);
        if (!ln.IsEmpty()) flat += " " + ln;
    }
    wxString lo = flat.Lower();

    // Find architecture body start (after "begin")
    size_t archPos = lo.find("architecture ");
    if (archPos == wxString::npos) { result.valid = true; return result; }

    size_t beginPos = lo.find(" begin", archPos);
    if (beginPos == wxString::npos) { result.valid = true; return result; }

    wxString body = lo.Mid((int)beginPos + 6);

    // Split on ';' → concurrent statements
    wxArrayString stmts = wxSplit(body, ';');

    static const wxString binaryOps[] = { "xnor", "nand", "nor", "xor", "and", "or" };

    for (size_t si = 0; si < stmts.GetCount(); ++si)
    {
        wxString stmt = stmts[si].Trim(true).Trim(false);
        if (stmt.IsEmpty()) continue;

        int leq = stmt.Find("<=");
        if (leq == wxNOT_FOUND) continue;

        wxString lhs = stmt.Left(leq).Trim(true).Trim(false);
        wxString rhs = stmt.Mid(leq + 2).Trim(true).Trim(false);
        if (lhs.IsEmpty() || lhs.Contains(" ") || rhs.IsEmpty()) continue;

        // Try binary operators (longest match first to catch xnor before nor/or, etc.)
        bool found = false;
        for (const wxString& op : binaryOps)
        {
            wxString needle = " " + op + " ";
            if (rhs.Find(needle) == wxNOT_FOUND) continue;

            // Collect ALL operands by splitting on this operator repeatedly.
            // e.g. "a and b and c and d" → [a, b, c, d] → 4-input AND gate.
            wxArrayString parts;
            wxString rem = rhs;
            while (!rem.IsEmpty())
            {
                int idx = rem.Find(needle);
                if (idx == wxNOT_FOUND)
                {
                    // Last segment - take first identifier only
                    // (guards against trailing different-op expressions)
                    wxString tok = rem.Trim(true).Trim(false);
                    int sp = tok.Find(' ');
                    if (sp != wxNOT_FOUND) tok = tok.Left(sp);
                    if (!tok.IsEmpty()) parts.Add(tok);
                    break;
                }
                wxString tok = rem.Left(idx).Trim(true).Trim(false);
                if (!tok.IsEmpty()) parts.Add(tok);
                rem = rem.Mid(idx + (int)needle.Len());
            }

            if (parts.GetCount() < 2) continue;

            RTLGateInst g;
            g.gateType     = OpToType(op);
            g.outputSignal = lhs;
            g.inputSignals = parts;
            result.gates.push_back(g);
            found = true;
            break;
        }

        if (!found && rhs.StartsWith("not "))
        {
            wxString in0 = rhs.Mid(4).Trim(true).Trim(false);
            int sp = in0.Find(' ');
            if (sp != wxNOT_FOUND) in0 = in0.Left(sp);
            if (!in0.IsEmpty())
            {
                RTLGateInst g;
                g.gateType     = GateType::NOT;
                g.outputSignal = lhs;
                g.inputSignals.Add(in0);
                result.gates.push_back(g);
            }
        }
    }

    result.valid = true;
    return result;
}

} // namespace VHDLGateExtractor
