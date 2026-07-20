#include "vcd_parser.h"
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include <map>

//--
// WfTrack helpers
//--
wxString WfTrack::ValueAt(long long t) const
{
    wxString val = signal.width > 1 ? "bx" : "x";
    for (const WfChange& c : changes)
    {
        if (c.time <= t)
            val = c.value;
        else
            break;
    }
    return val;
}

//--
// VcdParser::Parse
//--
bool VcdParser::Parse(const wxString& filePath)
{
    m_data = VcdData();

    wxTextFile file;
    if (!file.Open(filePath))
        return false;

    // Collect all whitespace-separated tokens from the whole file
    wxArrayString tokens;
    for (wxString line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine())
    {
        wxStringTokenizer tok(line, " \t\r\n");
        while (tok.HasMoreTokens())
            tokens.Add(tok.GetNextToken());
    }

    ParseTokens(tokens);
    return true;
}

//--
// Token-level parser
//--
// Read tokens until "$end" and join them (used for multi-word blocks)
static wxString ReadUntilEnd(const wxArrayString& tok, int& i)
{
    wxString result;
    for (++i; i < (int)tok.size(); ++i)
    {
        if (tok[i] == "$end")
            break;
        if (!result.IsEmpty())
            result += " ";
        result += tok[i];
    }
    return result;
}

void VcdParser::ParseTokens(const wxArrayString& tok)
{
    long long currentTime  = 0;
    wxString  currentScope;

    // id → track index: built during $var parsing, used for value-change lookup.
    // When GHDL emits both the testbench-level signal and the instantiated
    // component's port (same name, different id), the second id is mapped to
    // the first track so changes still land in the right place and no duplicate
    // row appears.
    std::map<wxString, int> idToTrack;

    // name → track index: used for deduplication across scopes.
    std::map<wxString, int> nameToTrack;

    for (int i = 0; i < (int)tok.size(); ++i)
    {
        const wxString& t = tok[i];


        // ---- Header keywords ------------------------------------------------

        if (t == "$timescale")
        {
            m_data.timescale = ReadUntilEnd(tok, i);
            continue;
        }

        if (t == "$scope")
        {
            // $scope module <name> $end
            ++i; // skip type ("module")
            if (i + 1 < (int)tok.size())
            {
                ++i;
                currentScope = tok[i]; // scope name
            }
            // skip to $end
            while (i < (int)tok.size() && tok[i] != "$end")
                ++i;
            continue;
        }

        if (t == "$upscope")
        {
            while (i < (int)tok.size() && tok[i] != "$end")
                ++i;
            currentScope = wxEmptyString;
            continue;
        }

        if (t == "$var")
        {
            // $var <type> <width> <id> <name> [$end | <opt_range> $end]
            WfSignal sig;
            ++i; // type (wire, reg, ...)
            ++i; if (i < (int)tok.size()) tok[i].ToInt(&sig.width);
            ++i; if (i < (int)tok.size()) sig.id   = tok[i];
            ++i; if (i < (int)tok.size()) sig.name = tok[i];
            sig.scope = currentScope;

            // Skip optional bit-range and $end
            while (i < (int)tok.size() && tok[i] != "$end")
                ++i;

            auto it = nameToTrack.find(sig.name);
            if (it != nameToTrack.end())
            {
                // Same signal name already exists (e.g. GHDL duplicate UUT ports).
                // Map this id to the existing track - don't create a new row.
                idToTrack[sig.id] = it->second;
            }
            else
            {
                int idx = (int)m_data.tracks.size();
                WfTrack track;
                track.signal = sig;
                m_data.tracks.push_back(track);
                idToTrack[sig.id]    = idx;
                nameToTrack[sig.name] = idx;
            }
            continue;
        }

        if (t == "$enddefinitions")
        {
            while (i < (int)tok.size() && tok[i] != "$end")
                ++i;
            continue;
        }

        if (t == "$comment" || t == "$date" || t == "$version")
        {
            while (i < (int)tok.size() && tok[i] != "$end")
                ++i;
            continue;
        }

        if (t == "$dumpvars" || t == "$dumpon" || t == "$dumpoff" || t == "$dumpall")
            continue; // values follow inline; handled below

        if (t == "$end")
            continue;

        // ---- Simulation commands -------------------------------------------

        // Timestamp:  #<decimal>
        if (t.StartsWith("#"))
        {
            wxString timeStr = t.Mid(1);
            timeStr.ToLongLong(&currentTime);
            if (currentTime > m_data.endTime)
                m_data.endTime = currentTime;
            continue;
        }

        // Scalar value change:  0/1/x/z (and GHDL std_logic: U/W/L/H/-)
        // followed immediately by id  (e.g. "1!", "U!", "L!")
        if (t.Length() >= 2 &&
            (t[0] == '0' || t[0] == '1' ||
             t[0] == 'x' || t[0] == 'X' ||
             t[0] == 'z' || t[0] == 'Z' ||
             t[0] == 'u' || t[0] == 'U' ||   // uninitialized → x
             t[0] == 'w' || t[0] == 'W' ||   // weak unknown  → x
             t[0] == 'l' || t[0] == 'L' ||   // weak low      → 0
             t[0] == 'h' || t[0] == 'H' ||   // weak high     → 1
             t[0] == '-'))                    // don't care    → x
        {
            // Normalize to canonical 0/1/x/z
            wxChar   ch = t[0];
            wxString val;
            if      (ch == '0' || ch == 'l' || ch == 'L') val = "0";
            else if (ch == '1' || ch == 'h' || ch == 'H') val = "1";
            else                                           val = "x";

            wxString id = t.Mid(1);
            auto it = idToTrack.find(id);
            if (it != idToTrack.end())
            {
                WfChange c;
                c.time  = currentTime;
                c.value = val;
                m_data.tracks[it->second].changes.push_back(c);
            }
            continue;
        }

        // Vector/bus value change:  b<value> <id>   (two tokens)
        if ((t[0] == 'b' || t[0] == 'B' || t[0] == 'r' || t[0] == 'R') &&
            t.Length() > 1)
        {
            wxString val = t.Lower(); // "b0101..."
            ++i;
            if (i < (int)tok.size())
            {
                wxString id = tok[i];
                auto it = idToTrack.find(id);
                if (it != idToTrack.end())
                {
                    WfChange c;
                    c.time  = currentTime;
                    c.value = val;
                    m_data.tracks[it->second].changes.push_back(c);
                }
            }
            continue;
        }
    }

    // Post-parse: remove duplicate (time, value) entries that arise when alias
    // ids (GHDL UUT sub-scope ports) emit the same change as the primary id.
    for (auto& track : m_data.tracks)
    {
        auto& ch = track.changes;
        auto newEnd = std::unique(ch.begin(), ch.end(),
            [](const WfChange& a, const WfChange& b)
            {
                return a.time == b.time && a.value == b.value;
            });
        ch.erase(newEnd, ch.end());
    }
}
