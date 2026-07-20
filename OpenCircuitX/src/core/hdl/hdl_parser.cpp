#include "hdl_parser.h"

std::vector<OutlineItem> HdlParser::Parse(const wxString& code, const wxString& ext)
{
    std::vector<OutlineItem> items;

    bool isVHDL    = (ext == "vhd" || ext == "vhdl");
    bool isVerilog = (ext == "v"   || ext == "sv");

    if (!isVHDL && !isVerilog)
        return items;

    auto trimmed = [](wxString s) -> wxString {
        return s.Trim(true).Trim(false);
    };
    auto lower = [](wxString s) -> wxString {
        return s.Lower();
    };
    auto stripComment = [&](wxString line) -> wxString {
        int pos = line.Find("--");
        if (isVHDL && pos != wxNOT_FOUND)
            line = line.Left(pos);
        pos = line.Find("//");
        if (isVerilog && pos != wxNOT_FOUND)
            line = line.Left(pos);
        return line;
    };

    wxArrayString lines = wxSplit(code, '\n');

    // ---- VHDL parser -------------------------------------------------------
    if (isVHDL)
    {
        enum class St { NONE, IN_ENTITY, IN_PORT, IN_ARCH };
        St state = St::NONE;
        wxString currentParent;

        for (int i = 0; i < (int)lines.size(); ++i)
        {
            int lineNo = i + 1;
            wxString raw  = stripComment(lines[i]);
            wxString line = trimmed(raw);
            wxString lo   = lower(line);

            if (state == St::NONE || state == St::IN_ENTITY ||
                state == St::IN_PORT || state == St::IN_ARCH)
            {
                // entity <name> is
                if (lo.StartsWith("entity "))
                {
                    wxString rest = trimmed(line.Mid(7));
                    if (rest.Lower().EndsWith(" is"))
                        rest = trimmed(rest.Left(rest.Len() - 3));
                    else if (rest.Lower().EndsWith("\tis"))
                        rest = trimmed(rest.Left(rest.Len() - 3));
                    wxString name = rest.BeforeFirst(' ');
                    if (name.IsEmpty()) name = rest;
                    if (!name.IsEmpty())
                    {
                        OutlineItem it;
                        it.name = name; it.kind = "entity"; it.line = lineNo;
                        items.push_back(it);
                        currentParent = name;
                        state = St::IN_ENTITY;
                    }
                    continue;
                }

                // architecture <name> of <entity> is
                if (lo.StartsWith("architecture "))
                {
                    wxString rest = trimmed(line.Mid(13));
                    wxString name = rest.BeforeFirst(' ');
                    if (name.IsEmpty()) name = rest;
                    if (!name.IsEmpty())
                    {
                        OutlineItem it;
                        it.name = name; it.kind = "arch"; it.line = lineNo;
                        items.push_back(it);
                        currentParent = name;
                        state = St::IN_ARCH;
                    }
                    continue;
                }
            }

            if (state == St::IN_ENTITY)
            {
                if (lo.StartsWith("port") && lo.Contains("("))
                {
                    state = St::IN_PORT;
                    continue;
                }
                if (lo.StartsWith("end"))
                    state = St::NONE;
            }

            if (state == St::IN_PORT)
            {
                if (lo.StartsWith(")") || lo.StartsWith(");"))
                {
                    state = St::IN_ENTITY;
                    continue;
                }
                if (line.Contains(":"))
                {
                    wxString namesPart = trimmed(line.BeforeFirst(':'));
                    wxString rest      = trimmed(line.AfterFirst(':'));
                    wxString restLo    = lower(rest);

                    wxString kind = "port";
                    if      (restLo.StartsWith("in "))     kind = "port_in";
                    else if (restLo.StartsWith("out "))    kind = "port_out";
                    else if (restLo.StartsWith("inout "))  kind = "port_inout";
                    else if (restLo.StartsWith("buffer ")) kind = "port_out";

                    wxArrayString names = wxSplit(namesPart, ',');
                    for (const wxString& nm : names)
                    {
                        wxString n = trimmed(nm);
                        if (!n.IsEmpty())
                        {
                            OutlineItem it;
                            it.name = n; it.kind = kind; it.line = lineNo;
                            items.push_back(it);
                        }
                    }
                }
                continue;
            }

            if (state == St::IN_ARCH)
            {
                // signal <name> [, ...] : <type>
                if (lo.StartsWith("signal "))
                {
                    wxString rest      = trimmed(line.Mid(7));
                    wxString namesPart = rest.BeforeFirst(':');
                    wxArrayString names = wxSplit(namesPart, ',');
                    for (const wxString& nm : names)
                    {
                        wxString n = trimmed(nm);
                        if (!n.IsEmpty())
                        {
                            OutlineItem it;
                            it.name = n; it.kind = "signal"; it.line = lineNo;
                            items.push_back(it);
                        }
                    }
                    continue;
                }

                // process
                if (lo.StartsWith("process"))
                {
                    wxString label = wxString::Format("process@%d", lineNo);
                    if (i > 0)
                    {
                        wxString prev = trimmed(stripComment(lines[i - 1]));
                        if (prev.EndsWith(":"))
                        {
                            wxString lbl = trimmed(prev.Left(prev.Len() - 1));
                            if (!lbl.IsEmpty()) label = lbl;
                        }
                    }
                    if (lo.Contains(":") && lo.Find(':') < lo.Find("process"))
                    {
                        wxString lbl = trimmed(line.BeforeFirst(':'));
                        if (!lbl.IsEmpty()) label = lbl;
                    }
                    OutlineItem it;
                    it.name = label; it.kind = "process"; it.line = lineNo;
                    items.push_back(it);
                    continue;
                }

                if (lo.StartsWith("end ") || lo == "end")
                    state = St::NONE;
            }
        }
    }

    // ---- Verilog / SystemVerilog parser ------------------------------------
    if (isVerilog)
    {
        enum class St { NONE, IN_MODULE };
        St state = St::NONE;

        for (int i = 0; i < (int)lines.size(); ++i)
        {
            int lineNo = i + 1;
            wxString raw  = stripComment(lines[i]);
            wxString line = trimmed(raw);
            wxString lo   = lower(line);

            if (lo.StartsWith("module "))
            {
                wxString rest = trimmed(line.Mid(7));
                wxString name = rest.BeforeFirst(' ');
                if (name.IsEmpty()) name = rest.BeforeFirst('(');
                if (name.IsEmpty()) name = rest.BeforeFirst(';');
                if (name.IsEmpty()) name = rest;
                name = trimmed(name);
                if (!name.IsEmpty())
                {
                    OutlineItem it;
                    it.name = name; it.kind = "module"; it.line = lineNo;
                    items.push_back(it);
                    state = St::IN_MODULE;
                }
                continue;
            }

            if (state == St::IN_MODULE)
            {
                auto parsePort = [&](const wxString& kw, const wxString& kind)
                {
                    if (!lo.StartsWith(kw)) return;
                    wxString rest = trimmed(line.Mid((int)kw.Len()));
                    rest = rest.BeforeFirst(';');
                    rest = rest.BeforeFirst(',');
                    while (rest.Contains("["))
                    {
                        int a = rest.Find('['), b = rest.Find(']');
                        if (b != wxNOT_FOUND && b > a)
                            rest = trimmed(rest.Left(a)) + " " + trimmed(rest.Mid(b + 1));
                        else break;
                    }
                    for (const auto& kw2 : { "wire", "reg", "logic", "signed", "unsigned",
                                              "integer", "real", "time" })
                    {
                        wxString t = wxString::FromAscii(kw2);
                        if (lower(rest).StartsWith(t + " ") || lower(rest).StartsWith(t + "\t"))
                            rest = trimmed(rest.Mid(t.Len()));
                    }
                    wxArrayString names = wxSplit(rest, ',');
                    for (const wxString& nm : names)
                    {
                        wxString n = trimmed(nm);
                        if (!n.IsEmpty() && n != ";")
                        {
                            OutlineItem it;
                            it.name = n; it.kind = kind; it.line = lineNo;
                            items.push_back(it);
                        }
                    }
                };
                parsePort("input ",  "input");
                parsePort("output ", "output");
                parsePort("inout ",  "inout");

                if (lo.StartsWith("wire ") || lo.StartsWith("reg "))
                {
                    wxString kw2  = lo.StartsWith("wire ") ? "wire" : "reg";
                    wxString rest = trimmed(line.Mid((int)kw2.Len() + 1));
                    rest = rest.BeforeFirst(';');
                    while (rest.Contains("["))
                    {
                        int a = rest.Find('['), b = rest.Find(']');
                        if (b != wxNOT_FOUND && b > a)
                            rest = trimmed(rest.Left(a)) + " " + trimmed(rest.Mid(b + 1));
                        else break;
                    }
                    wxArrayString names = wxSplit(rest, ',');
                    for (const wxString& nm : names)
                    {
                        wxString n = trimmed(nm);
                        if (!n.IsEmpty())
                        {
                            OutlineItem it;
                            it.name = n; it.kind = kw2; it.line = lineNo;
                            items.push_back(it);
                        }
                    }
                    continue;
                }

                if (lo.StartsWith("always") || lo.StartsWith("initial"))
                {
                    wxString kindStr = lo.StartsWith("always") ? "always" : "initial";
                    OutlineItem it;
                    it.name = wxString::Format("%s@%d", kindStr, lineNo);
                    it.kind = kindStr; it.line = lineNo;
                    items.push_back(it);
                    continue;
                }

                if (lo.StartsWith("assign "))
                {
                    wxString rest = trimmed(line.Mid(7));
                    wxString name = trimmed(rest.BeforeFirst('='));
                    if (!name.IsEmpty())
                    {
                        OutlineItem it;
                        it.name = name; it.kind = "assign"; it.line = lineNo;
                        items.push_back(it);
                    }
                    continue;
                }

                if (lo.StartsWith("endmodule"))
                    state = St::NONE;
            }
        }
    }

    return items;
}
