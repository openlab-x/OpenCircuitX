#pragma once
#include <wx/wx.h>
#include <vector>

//--
// OutlineItem - one symbol entry produced by the HDL parser
//--
struct OutlineItem
{
    wxString name;
    wxString kind;   // "entity","arch","port_in","port_out","port_inout","signal",
                     // "process","module","input","output","inout","wire","reg",
                     // "always","initial","assign"
    int      line;   // 1-based source line number
};

//--
// HdlParser - static VHDL / Verilog / SystemVerilog symbol parser
//--
namespace HdlParser
{
    // Parse code and return a flat list of symbols.
    // ext must be lowercase: "vhd", "vhdl", "v", "sv"
    std::vector<OutlineItem> Parse(const wxString& code, const wxString& ext);
}
