#pragma once
#include <wx/wx.h>
#include <vector>

enum class PortDir { In, Out, InOut, Buffer };

struct VHDLPort
{
    wxString name;
    PortDir  dir;
    wxString typeName;
};

struct VHDLEntity
{
    wxString              name;
    std::vector<VHDLPort> ports;
    bool                  valid = false;
};

namespace VHDLEntityParser
{
    // Parse the first entity declaration found in raw VHDL text.
    // Returns a VHDLEntity with valid=false if no entity is found.
    VHDLEntity ParseText(const wxString& text);
}
