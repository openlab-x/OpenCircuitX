#pragma once

static const char* VHDL_KEYWORDS =
    "abs access after alias all and architecture array assert attribute "
    "begin block body buffer bus case component configuration constant "
    "disconnect downto else elsif end entity exit file for function "
    "generate generic group guarded if impure in inertial inout is "
    "label library linkage literal loop map mod nand new next nor not "
    "null of on open or others out package port postponed procedure "
    "process protected pure range record register reject rem report "
    "return rol ror select severity signal shared sla sll sra srl "
    "subtype then to transport type unaffected units until use variable "
    "wait when while with xnor xor";

static const char* VERILOG_KEYWORDS =
    "always and assign begin buf bufif0 bufif1 case casex casez cmos "
    "deassign default defparam disable edge else end endcase endfunction "
    "endmodule endprimitive endspecify endtable endtask event for force "
    "forever fork function highz0 highz1 if ifnone initial inout input "
    "integer join large macromodule medium module nand negedge nmos nor "
    "not notif0 notif1 or output parameter pmos posedge primitive pull0 "
    "pull1 pulldown pullup rcmos real realtime reg release repeat rnmos "
    "rpmos rtran rtranif0 rtranif1 scalared small specify specparam "
    "strong0 strong1 supply0 supply1 table task time tran tranif0 "
    "tranif1 tri tri0 tri1 triand trior trireg vectored wait wand weak0 "
    "weak1 while wire wor xnor xor";
