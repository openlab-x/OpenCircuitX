#pragma once
#include <wx/gdicmn.h>
#include <wx/string.h>

enum class GateType
{
    // Basic combinational
    AND, OR, NOT, NAND, NOR, XOR, XNOR,
    // Sequential
    DFLIPFLOP,  // inputs: D(0), CLK(1) - outputs: Q(0), !Q(1)
    SRLATCH,    // inputs: S(0), R(1) - outputs: Q(0), !Q(1)  (level-sensitive)
    JKFLIPFLOP, // inputs: J(0), K(1), CLK(2) - outputs: Q(0), !Q(1)  (tick-driven)
    TFLIPFLOP,  // inputs: T(0), CLK(1) - outputs: Q(0), !Q(1)  (tick-driven)
    // Complex combinational
    MUX,        // inputs: A(0), B(1), SEL(2) - output: SEL ? B : A
    HALFADDER,  // inputs: A(0), B(1) - outputs: Sum(0), Carry(1)
    FULLADDER,  // inputs: A(0), B(1), Cin(2) - outputs: Sum(0), Carry(1)
    // Source/sink
    CLOCK,      // 0 inputs, free-running clock source
    INPUT,      // top-level input port  (0 inputs, 1 output - togglable)
    OUTPUT,     // top-level output port (1 input,  0 outputs - display only)
    // Tri-state / routing
    TRISTATE,   // inputs: A(0), EN(1) - output: EN ? A : 0  (HiZ shown as 0 in sim)
    DEMUX       // inputs: A(0), SEL(1) - outputs: Y0(0)=(!SEL&A), Y1(1)=(SEL&A)
};

// Gate body width is fixed; height varies by type.
static constexpr int GATE_W = 72;

struct Gate
{
    int      id;
    GateType type;
    wxPoint  pos;          // top-left corner in canvas coordinates
    wxString label;        // user-visible name - used for INPUT/OUTPUT port names
    bool     inputState[4];
    bool     outputState;   // primary output (Q for DFF, Sum for HA/FA)
    bool     outputState2;  // secondary output (!Q for DFF, Carry for HA/FA)

    wxString GetLabel()            const;  // returns type name or user label
    int      GetInputCount()       const;
    int      GetOutputCount()      const;
    int      GetHeight()           const;
    wxRect   GetBounds()           const;
    wxPoint  GetInputPinPos(int i) const;  // absolute canvas position
    wxPoint  GetOutputPinPos()     const;  // primary output (index 0)
    wxPoint  GetOutputPinPos(int i) const; // 0=primary, 1=secondary
    wxString GetInputLabel(int i)  const;
    wxString GetOutputLabel(int i) const;
    bool     Evaluate()            const;  // primary output
    bool     EvaluateSecondary()   const;  // secondary output (gates with 2 outputs)
};

struct Wire
{
    int fromGateId;
    int fromPinIndex;   // 0=primary output, 1=secondary output
    int toGateId;
    int toPinIndex;
};
