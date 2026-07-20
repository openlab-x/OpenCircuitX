#include "vhdl_entity_parser.h"

namespace VHDLEntityParser
{

static wxString StripComment(const wxString& s)
{
    int pos = s.Find("--");
    return (pos != wxNOT_FOUND) ? s.Left(pos) : s;
}

static PortDir DirFromString(const wxString& lo)
{
    if (lo == "inout")  return PortDir::InOut;
    if (lo == "out")    return PortDir::Out;
    if (lo == "buffer") return PortDir::Buffer;
    return PortDir::In;
}

VHDLEntity ParseText(const wxString& text)
{
    VHDLEntity result;

    // Flatten all lines into a single string with comments stripped.
    wxArrayString rawLines = wxSplit(text, '\n');
    wxString flat;
    for (size_t i = 0; i < rawLines.GetCount(); ++i)
    {
        wxString ln = StripComment(rawLines[i]).Trim(true).Trim(false);
        if (!ln.IsEmpty())
            flat += " " + ln;
    }

    wxString flatLo = flat.Lower();

    // Find "entity <name> is"
    int ePos = flatLo.Find("entity ");
    if (ePos == wxNOT_FOUND) return result;

    int nameStart = ePos + 7;
    while (nameStart < (int)flat.Len() && flat[nameStart] == ' ') nameStart++;
    int nameEnd = nameStart;
    while (nameEnd < (int)flat.Len() &&
           flat[nameEnd] != ' ' && flat[nameEnd] != '\t' && flat[nameEnd] != '\n')
        nameEnd++;
    result.name = flat.Mid(nameStart, nameEnd - nameStart);

    // Find "port" then its opening '('
    // Use std-style find() because wxString::Find() has no start-position overload.
    size_t portPosS = flatLo.find("port", (size_t)ePos);
    if (portPosS == wxString::npos) { result.valid = !result.name.IsEmpty(); return result; }
    int portPos = (int)portPosS;

    size_t parenOpenS = flatLo.find('(', (size_t)portPos);
    if (parenOpenS == wxString::npos) { result.valid = !result.name.IsEmpty(); return result; }
    int parenOpen = (int)parenOpenS;

    // Find the matching closing ')' tracking depth for nested types like std_logic_vector(...)
    int depth = 1;
    int pos   = parenOpen + 1;
    while (pos < (int)flat.Len() && depth > 0)
    {
        if (flat[pos] == '(') depth++;
        else if (flat[pos] == ')') depth--;
        if (depth > 0) pos++;
    }
    int parenClose = pos;

    // Extract port block and split on ';'
    wxString portBlock = flat.Mid(parenOpen + 1, parenClose - parenOpen - 1);
    wxArrayString decls = wxSplit(portBlock, ';');

    for (size_t i = 0; i < decls.GetCount(); ++i)
    {
        wxString decl = decls[i].Trim(true).Trim(false);
        if (decl.IsEmpty()) continue;

        int colon = decl.Find(':');
        if (colon == wxNOT_FOUND) continue;

        wxString namesPart = decl.Left(colon).Trim(true).Trim(false);
        wxString rest      = decl.Mid(colon + 1).Trim(true).Trim(false);

        // First word is the direction
        int sp = rest.Find(' ');
        wxString dirWord, typeName;
        if (sp == wxNOT_FOUND)
        {
            dirWord  = rest;
            typeName = wxEmptyString;
        }
        else
        {
            dirWord  = rest.Left(sp).Trim(true).Trim(false);
            typeName = rest.Mid(sp).Trim(true).Trim(false);
        }
        PortDir dir = DirFromString(dirWord.Lower());

        // Each name before ':'
        wxArrayString names = wxSplit(namesPart, ',');
        for (auto& nm : names)
        {
            nm = nm.Trim(true).Trim(false);
            if (nm.IsEmpty()) continue;
            VHDLPort port;
            port.name     = nm;
            port.dir      = dir;
            port.typeName = typeName;
            result.ports.push_back(port);
        }
    }

    result.valid = !result.name.IsEmpty();
    return result;
}

} // namespace VHDLEntityParser
