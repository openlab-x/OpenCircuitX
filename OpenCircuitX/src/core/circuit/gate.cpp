#include "gate.h"

wxString Gate::GetLabel() const
{
    if ((type == GateType::INPUT || type == GateType::OUTPUT) && !label.IsEmpty())
        return label;
    switch (type)
    {
    case GateType::AND:       return "AND";
    case GateType::OR:        return "OR";
    case GateType::NOT:       return "NOT";
    case GateType::NAND:      return "NAND";
    case GateType::NOR:       return "NOR";
    case GateType::XOR:       return "XOR";
    case GateType::XNOR:      return "XNOR";
    case GateType::DFLIPFLOP:  return "DFF";
    case GateType::SRLATCH:    return "SR";
    case GateType::JKFLIPFLOP: return "JK-FF";
    case GateType::TFLIPFLOP:  return "T-FF";
    case GateType::MUX:        return "MUX";
    case GateType::HALFADDER: return "HA";
    case GateType::FULLADDER: return "FA";
    case GateType::CLOCK:     return "CLK";
    case GateType::INPUT:     return "IN";
    case GateType::TRISTATE:  return "TRI";
    case GateType::DEMUX:     return "DEMUX";
    case GateType::OUTPUT:    return "OUT";
    default:                  return "?";
    }
}

int Gate::GetInputCount() const
{
    switch (type)
    {
    case GateType::NOT:       return 1;
    case GateType::DFLIPFLOP:  return 2;  // D(0), CLK(1)
    case GateType::SRLATCH:    return 2;  // S(0), R(1)
    case GateType::JKFLIPFLOP: return 3;  // J(0), K(1), CLK(2)
    case GateType::TFLIPFLOP:  return 2;  // T(0), CLK(1)
    case GateType::MUX:        return 3;  // A(0), B(1), SEL(2)
    case GateType::FULLADDER: return 3;  // A(0), B(1), Cin(2)
    case GateType::CLOCK:     return 0;
    case GateType::INPUT:     return 0;
    case GateType::OUTPUT:    return 1;
    case GateType::TRISTATE:  return 2;  // A(0), EN(1)
    case GateType::DEMUX:     return 2;  // A(0), SEL(1)
    default:                  return 2;
    }
}

int Gate::GetOutputCount() const
{
    switch (type)
    {
    case GateType::DFLIPFLOP:
    case GateType::SRLATCH:
    case GateType::JKFLIPFLOP:
    case GateType::TFLIPFLOP:
    case GateType::HALFADDER:
    case GateType::FULLADDER: return 2;
    case GateType::OUTPUT:    return 0;
    case GateType::TRISTATE:  return 1;
    case GateType::DEMUX:     return 2;  // Y0(0), Y1(1)
    default:                  return 1;
    }
}

int Gate::GetHeight() const
{
    switch (type)
    {
    case GateType::DFLIPFLOP:  return 72;  // D, CLK inputs; Q, !Q outputs
    case GateType::SRLATCH:    return 60;  // S, R inputs; Q, !Q outputs
    case GateType::JKFLIPFLOP: return 72;  // J, K, CLK inputs; Q, !Q outputs
    case GateType::TFLIPFLOP:  return 60;  // T, CLK inputs; Q, !Q outputs
    case GateType::MUX:        return 72;  // A, B, SEL inputs
    case GateType::FULLADDER: return 72;  // A, B, Cin inputs; Sum, Carry outputs
    case GateType::HALFADDER: return 60;  // A, B inputs; Sum, Carry outputs
    case GateType::TRISTATE:  return 50;
    case GateType::DEMUX:     return 60;
    default:                  return 50;
    }
}

wxRect Gate::GetBounds() const
{
    return wxRect(pos.x, pos.y, GATE_W, GetHeight());
}

wxPoint Gate::GetInputPinPos(int index) const
{
    int count = GetInputCount();
    int h     = GetHeight();
    if (count == 0)
        return wxPoint(pos.x, pos.y + h / 2);  // unused
    if (count == 1)
        return wxPoint(pos.x, pos.y + h / 2);
    if (count == 2)
        return (index == 0)
            ? wxPoint(pos.x, pos.y + h / 3)
            : wxPoint(pos.x, pos.y + 2 * h / 3);
    // 3 inputs: evenly at 1/4, 1/2, 3/4
    int offsets[3] = { h / 4, h / 2, 3 * h / 4 };
    return wxPoint(pos.x, pos.y + offsets[index]);
}

wxPoint Gate::GetOutputPinPos() const
{
    return GetOutputPinPos(0);
}

wxPoint Gate::GetOutputPinPos(int i) const
{
    int h    = GetHeight();
    int outC = GetOutputCount();
    if (outC == 1)
        return wxPoint(pos.x + GATE_W, pos.y + h / 2);
    // 2 outputs: top and bottom thirds
    return (i == 0)
        ? wxPoint(pos.x + GATE_W, pos.y + h / 3)
        : wxPoint(pos.x + GATE_W, pos.y + 2 * h / 3);
}

wxString Gate::GetInputLabel(int i) const
{
    switch (type)
    {
    case GateType::DFLIPFLOP:  return i == 0 ? "D"   : "CLK";
    case GateType::SRLATCH:    return i == 0 ? "S"   : "R";
    case GateType::JKFLIPFLOP: return i == 0 ? "J"   : (i == 1 ? "K" : "CLK");
    case GateType::TFLIPFLOP:  return i == 0 ? "T"   : "CLK";
    case GateType::MUX:        return i == 0 ? "A"   : (i == 1 ? "B" : "SEL");
    case GateType::FULLADDER: return i == 0 ? "A" : (i == 1 ? "B" : "Cin");
    case GateType::HALFADDER: return i == 0 ? "A" : "B";
    case GateType::NOT:       return "A";
    case GateType::AND:
    case GateType::OR:
    case GateType::NAND:
    case GateType::NOR:
    case GateType::XOR:
    case GateType::XNOR:      return i == 0 ? "A" : "B";
    case GateType::OUTPUT:    return "in";
    case GateType::TRISTATE:  return i == 0 ? "A"   : "EN";
    case GateType::DEMUX:     return i == 0 ? "A"   : "SEL";
    default:                  return "";
    }
}

wxString Gate::GetOutputLabel(int i) const
{
    switch (type)
    {
    case GateType::DFLIPFLOP:
    case GateType::SRLATCH:
    case GateType::JKFLIPFLOP:
    case GateType::TFLIPFLOP:  return i == 0 ? "Q" : "Q'";
    case GateType::HALFADDER:
    case GateType::FULLADDER: return i == 0 ? "Sum" : "Cy";
    case GateType::CLOCK:     return "CLK";
    case GateType::INPUT:     return "out";
    case GateType::TRISTATE:  return "Z";
    case GateType::DEMUX:     return i == 0 ? "Y0" : "Y1";
    default:                  return "";
    }
}

bool Gate::Evaluate() const
{
    bool a   = inputState[0];
    bool b   = inputState[1];
    bool cin = inputState[2];

    switch (type)
    {
    case GateType::AND:       return a && b;
    case GateType::OR:        return a || b;
    case GateType::NOT:       return !a;
    case GateType::NAND:      return !(a && b);
    case GateType::NOR:       return !(a || b);
    case GateType::XOR:       return a ^ b;
    case GateType::XNOR:      return !(a ^ b);
    case GateType::DFLIPFLOP:  return a;             // Q follows D (clocked behaviour via Tick)
    case GateType::SRLATCH:    return a ? true : (b ? false : outputState); // S priority SR latch
    case GateType::JKFLIPFLOP: return outputState;  // tick-driven (TickClocks handles J/K logic)
    case GateType::TFLIPFLOP:  return outputState;  // tick-driven (TickClocks handles T logic)
    case GateType::MUX:       return cin ? b : a;   // SEL=inputState[2]
    case GateType::HALFADDER: return a ^ b;         // Sum
    case GateType::FULLADDER: return a ^ b ^ cin;   // Sum
    case GateType::CLOCK:     return outputState;   // toggled externally by TickClocks
    case GateType::INPUT:     return outputState;   // toggled by user click
    case GateType::OUTPUT:    return a;             // pass-through for display
    case GateType::TRISTATE:  return b ? a : false; // Z = EN ? A : 0
    case GateType::DEMUX:     return !b && a;       // Y0 = !SEL & A
    default:                  return false;
    }
}

bool Gate::EvaluateSecondary() const
{
    bool a   = inputState[0];
    bool b   = inputState[1];
    bool cin = inputState[2];

    switch (type)
    {
    case GateType::DFLIPFLOP:
    case GateType::SRLATCH:
    case GateType::JKFLIPFLOP:
    case GateType::TFLIPFLOP:  return !outputState;  // !Q (Q-bar)
    case GateType::HALFADDER: return a && b;                // Carry
    case GateType::FULLADDER: return (a && b) || (cin && (a ^ b)); // Carry
    case GateType::DEMUX:     return b && a;  // Y1 = SEL & A
    default:                  return false;
    }
}
