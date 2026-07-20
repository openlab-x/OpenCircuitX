#pragma once
#include <wx/wx.h>
#include "core/circuit/gate.h"
#include "core/hdl/vhdl_entity_parser.h"
#include <vector>

struct RTLGateInst
{
    GateType      gateType;
    wxString      outputSignal;
    wxArrayString inputSignals;   // 1 for NOT, 2 for binary ops
};

struct RTLNetlist
{
    bool                     valid = false;
    wxArrayString            inputPorts;   // entity in / inout ports (lowercase)
    wxArrayString            outputPorts;  // entity out / buffer ports (lowercase)
    std::vector<RTLGateInst> gates;
};

namespace VHDLGateExtractor
{
    RTLNetlist Extract(const wxString& vhdlText, const VHDLEntity& entity);
}
