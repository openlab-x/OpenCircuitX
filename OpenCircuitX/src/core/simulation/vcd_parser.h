#pragma once
#include <wx/string.h>
#include <wx/arrstr.h>
#include <vector>

// One signal definition from the VCD $var block
struct WfSignal
{
    wxString id;     // VCD identifier code (e.g. "!", "#", "ab")
    wxString name;   // human-readable name
    wxString scope;  // parent scope / module name
    int      width;  // bit width (1 = scalar, >1 = bus)
};

// A single value change event
struct WfChange
{
    long long time;   // simulation time in VCD time units
    wxString  value;  // "0","1","x","z" (scalar) or "b0101..." (bus)
};

// One complete signal track: definition + all its changes
struct WfTrack
{
    WfSignal              signal;
    std::vector<WfChange> changes;

    // Returns the signal value at time t (last change at or before t).
    wxString ValueAt(long long t) const;
};

// Parsed contents of one .vcd file
struct VcdData
{
    wxString             timescale; // e.g. "1ns", "10ps"
    std::vector<WfTrack> tracks;
    long long            endTime;   // last timestamp seen
};

class VcdParser
{
public:
    // Parse the file at filePath.
    // Returns true on success.  Call GetData() for results.
    bool Parse(const wxString& filePath);

    const VcdData& GetData() const { return m_data; }

private:
    VcdData m_data;

    void ParseTokens(const wxArrayString& tokens);
};
