# Changelog

---

## [1.0.0]

First release. Digital Design is fully supported; Analog and Mixed-Signal are planned for future major versions.

### Added

**HDL Editor**
- VHDL, Verilog, and SystemVerilog syntax highlighting, plus PCF (iCE40) and LPF (ECP5) constraint file highlighting
- Multi-tab editor with close/reopen, modified indicator, file templates for new `.vhd`/`.v`/`.sv` files
- Smart auto-indent, code folding, line numbers, word wrap, whitespace view, zoom, move/duplicate line, select all occurrences, jump to matching brace, sort lines, toggle line comment
- Bookmarks, inline find/replace, Find in Files and Replace in Files, Quick Open fuzzy file picker
- Auto-save timer and session restore (tabs reopen on next launch)
- Live VHDL syntax check on save (`ghdl -s`), multi-file Verilog compile, inline error squiggles, error double-click to jump to file:line:column
- Signal value tooltip on hover when a VCD is loaded

**Code Intelligence**
- Live outline panel for VHDL and Verilog/SV (entities, ports, signals, processes, modules)
- Auto-complete from project symbols, Go to Definition (F12), Symbol Search (Ctrl+Shift+O)

**Simulation & Debug**
- GHDL for VHDL-2008/1993/2019 (compile/run/debug), Icarus Verilog for `.v`/`.sv`, Verilator for lint and fast simulation
- Run Config Bar, auto-generated `.vcd` per run, Output/Errors panel, Watch panel, breakpoints, configurable stop-time
- Smart testbench generator that auto-detects `clk`/`rst` ports

**Waveform Viewer**
- Built-in VCD parser (no GTKWave required), with duplicate-scope signal deduplication
- Cursor and reference-line time deltas, zoom/fit, named markers, truth-table view, signal filtering and value search
- Export to PNG or CSV, save/load waveform sessions as `.ocxwave`

**Circuit Canvas**
- Drag-and-drop logic gates (AND, OR, NOT, NAND, NOR, XOR, XNOR, D Flip-Flop, MUX, adders, Clock, Tri-state Buffer, DEMUX) with placement hotkeys
- Wire drawing with Manhattan routing, live forward simulation on pin toggle, canvas notes, alignment tools, clipboard
- Visual animation with play/pause/step/speed control, VCD recording from the canvas
- Export to VHDL, Verilog, or PNG; save/load layouts as `.ocxschem`

**RTL View**
- Parses VHDL and renders a full combinational schematic with no external tools required
- Topological column layout, ANSI/IEEE gate shapes, automatic wire routing, netlist tree sidebar synced to the schematic
- Zoom, pan, click-to-select with wire highlighting, export to PNG

**FPGA Toolchain**
- Yosys synthesis (VHDL via ghdl-yosys-plugin, Verilog natively), iCE40 and ECP5 place-and-route/packing flows
- Support for iCEBreaker, iCE40-HX8K, TinyFPGA BX, UPduino v3, ColorLight i5, OrangeCrab, and ULX3S boards
- Resource usage report (LUT/FF/BRAM), no single-vendor toolchain required

**IDE & Project**
- Welcome screen with Digital/Analog/Mixed-Signal domain cards and recent projects list
- Three themes (Dark, Midnight, Light), custom dark title bar on Windows 11
- Human-readable `.ocxproj` INI project format, New Project dialog with 9 templates, project explorer
- Searchable keyboard shortcuts dialog, branded splash screen
- In-app update checker with startup check, menu bar badge, and Help menu label

**Plugin System**
- Drop-in `.dll`/`.so`/`.dylib` loading from a `plugins/` folder, with a small documented C API (`plugin_api.h`)

**Distribution**
- Windows installer (NSIS) with file associations for `.ocxproj`/`.ocxschem`
- Documentation site with a light/dark theme toggle
