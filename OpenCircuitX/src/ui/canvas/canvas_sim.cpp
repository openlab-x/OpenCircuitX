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

// Animation
//--
void CircuitCanvas::CapturePrevOutputs()
{
    m_prevOutputs.clear();
    for (const Gate& g : gates)
        m_prevOutputs[g.id] = g.outputState;
}

void CircuitCanvas::FindChangedGates()
{
    m_changedGates.clear();
    for (const Gate& g : gates)
    {
        auto it = m_prevOutputs.find(g.id);
        if (it != m_prevOutputs.end() && it->second != g.outputState)
            m_changedGates.insert(g.id);
    }
}

void CircuitCanvas::StartAnimation()
{
    if (gates.empty()) return;
    m_animating = true;
    m_animTimer.Start(m_animIntervalMs);
    Refresh();
}

void CircuitCanvas::StopAnimation()
{
    m_animating = false;
    m_animTimer.Stop();
    m_changedGates.clear();
    Refresh();
}

void CircuitCanvas::StepAnimation()
{
    CapturePrevOutputs();
    TickClocks();
    FindChangedGates();
    Refresh();
}

void CircuitCanvas::SetAnimSpeed(int ms)
{
    m_animIntervalMs = ms;
    if (m_animating)
        m_animTimer.Start(ms);
}

//--
// VCD recording
//--
void CircuitCanvas::StartRecording(const wxString& outputPath)
{
    m_recordPath    = outputPath;
    m_recordSamples.clear();
    m_recordTick    = 0;
    m_recording     = true;

    // Capture initial state (tick 0) - all gates
    for (const Gate& g : gates)
        m_recordSamples.push_back({ 0, g.id, g.outputState });
}

void CircuitCanvas::StopRecording()
{
    if (!m_recording) return;
    m_recording = false;

    if (!m_recordSamples.empty() && WriteSimVCD())
    {
        if (m_simVCDCb)
            m_simVCDCb(m_recordPath);
    }
}

// Returns a short human-readable type prefix for VCD signal naming.
static wxString GateTypeName(GateType t)
{
    switch (t)
    {
        case GateType::AND:        return "AND";
        case GateType::OR:         return "OR";
        case GateType::NOT:        return "NOT";
        case GateType::NAND:       return "NAND";
        case GateType::NOR:        return "NOR";
        case GateType::XOR:        return "XOR";
        case GateType::XNOR:       return "XNOR";
        case GateType::DFLIPFLOP:  return "DFF";
        case GateType::SRLATCH:    return "SR";
        case GateType::JKFLIPFLOP: return "JKF";
        case GateType::TFLIPFLOP:  return "TFF";
        case GateType::MUX:        return "MUX";
        case GateType::HALFADDER:  return "HA";
        case GateType::FULLADDER:  return "FA";
        case GateType::CLOCK:      return "CLK";
        case GateType::INPUT:      return "IN";
        case GateType::OUTPUT:     return "OUT";
        case GateType::TRISTATE:   return "TRI";
        case GateType::DEMUX:      return "DEMUX";
        default:                   return "GATE";
    }
}

// Generates a unique printable VCD identifier for index n (0-based).
// Single char for n<94, double char for n<8836, etc.
static wxString MakeVcdId(int n)
{
    const int B = 94; // printable ASCII '!' (0x21) .. '~' (0x7E)
    wxString id;
    do {
        id = wxChar('!' + n % B) + id;
        n  = n / B - 1;
    } while (n >= 0);
    return id;
}

bool CircuitCanvas::WriteSimVCD() const
{
    if (gates.empty()) return false;

    wxFile f(m_recordPath, wxFile::write);
    if (!f.IsOpened()) return false;

    // Build ordered list of all gate ids present in samples
    std::vector<int> ids;
    {
        std::set<int> seen;
        for (const auto& s : m_recordSamples)
            if (seen.insert(s.gateId).second)
                ids.push_back(s.gateId);
    }
    if (ids.empty()) return false;

    // Assign VCD identifiers and signal names
    std::map<int, wxString> idMap;   // gateId → VCD ident string
    int identIdx = 0;

    f.Write("$timescale 1ns $end\n");
    f.Write("$scope module canvas $end\n");

    for (int gid : ids)
    {
        const Gate* g = FindGate(gid);
        if (!g) continue;

        // Name: user label if set, otherwise TypeName_ID (e.g. "AND_3")
        wxString name = g->label.IsEmpty()
            ? GateTypeName(g->type) + "_" + wxString::Format("%d", g->id)
            : g->label;
        name.Replace(" ", "_");

        wxString vcdId = MakeVcdId(identIdx++);
        f.Write(wxString::Format("$var wire 1 %s %s $end\n", vcdId, name));
        idMap[gid] = vcdId;
    }

    f.Write("$upscope $end\n");
    f.Write("$enddefinitions $end\n");
    f.Write("$dumpvars\n");

    // Group samples by tick and emit value changes
    std::map<long long, std::vector<SimSample>> byTick;
    for (const auto& s : m_recordSamples)
        byTick[s.tick].push_back(s);

    bool dumpDone = false;
    for (const auto& kv : byTick)
    {
        f.Write(wxString::Format("#%lld\n", kv.first * 10)); // 10 ns per tick
        for (const auto& s : kv.second)
        {
            auto it = idMap.find(s.gateId);
            if (it == idMap.end()) continue;
            f.Write(wxString::Format("%c%s\n",
                                     s.value ? '1' : '0', it->second));
        }
        if (!dumpDone) { f.Write("$end\n"); dumpDone = true; }
    }

    return true;
}

void CircuitCanvas::OnAnimTimer(wxTimerEvent&)
{
    CapturePrevOutputs();
    TickClocks();
    FindChangedGates();

    // VCD recording: sample every gate's output each tick
    if (m_recording)
    {
        for (const Gate& g : gates)
            m_recordSamples.push_back({ m_recordTick, g.id, g.outputState });
        ++m_recordTick;

        // Auto-stop after 200 ticks to prevent unbounded growth
        if (m_recordTick >= 200)
            StopRecording();
    }
}

//--
// Tick clocks
//--
void CircuitCanvas::TickClocks()
{
    for (Gate& g : gates)
    {
        if (g.type == GateType::CLOCK)
        {
            g.outputState = !g.outputState;
        }
        else if (g.type == GateType::JKFLIPFLOP)
        {
            bool j = g.inputState[0], k = g.inputState[1];
            if      ( j && !k) g.outputState = true;
            else if (!j &&  k) g.outputState = false;
            else if ( j &&  k) g.outputState = !g.outputState;
            // j=0,k=0: hold
        }
        else if (g.type == GateType::TFLIPFLOP)
        {
            if (g.inputState[0])   // T=1: toggle
                g.outputState = !g.outputState;
            // T=0: hold
        }
    }
    SimulateOnce();
    Refresh();
}

//--