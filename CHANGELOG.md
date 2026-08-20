# Changelog

---

## [1.1.0] - 2026-08-20

Linux and macOS builds work for the first time, and the Verilog simulation workflow now behaves the same way the VHDL one always did.

### Added

- Linux and macOS support. The update checker previously included Windows-only headers unconditionally, so the project could not be compiled on either platform. Non-Windows builds now fetch the remote version through libcurl, with the response bounded to the size of the receiving buffer. **libcurl is now a build dependency on Linux and macOS.** Thanks to @dylanbautista for the original patch, and to @johnblommers and @jamescraigziegler for confirming the failure independently on Ubuntu. (#1, #2, #3)
- Generated Verilog testbenches now include a `$dumpfile`/`$dumpvars` block, so an Icarus run actually produces a waveform. Icarus has no equivalent of GHDL's `--vcd` flag, so without those calls no VCD is written at all.
- RTL View now explains itself when it cannot render a file, instead of showing an empty panel. It parses VHDL only; a Verilog file previously produced a blank view with no message.
- A Verilog run that produces no VCD now prints the exact lines to add to a hand-written testbench.

### Fixed

- CMake now guards the curl dependency with `NOT WIN32` instead of `APPLE OR LINUX`, which was never true on CMake versions before 3.25.
- RTL View now refreshes when you switch to its tab. A stale tab index meant the refresh fired on the Circuit Canvas tab instead, leaving RTL View showing whatever it last had, or nothing at all.
- Saving a non-VHDL file now updates RTL View too, so it can explain why it can't render the file rather than staying blank.
- The Settings dialog's Simulation and FPGA tabs now scroll instead of overlapping their own rows on Linux. The dialog is a fixed size tuned for Windows' more compact widget metrics; GTK renders the same content taller and had nowhere to put the overflow.
- The app window now has a real icon on Linux (dock, taskbar, window decoration, app search) instead of a blank/generic one. It was only ever loaded via a Windows-only resource mechanism, with no fallback for platforms that don't have it. Ships as a PNG on Linux/macOS rather than the Windows `.ico`, matching the actual Linux/freedesktop icon-theme standard.
- GHDL, Icarus, and Verilator no longer fail to launch on Linux when pointed at a standalone release that bundles its own runtime libraries in a `lib/` folder next to `bin/` (same class of fix already applied to Yosys). Note: GHDL specifically ships two backend variants on Linux - use the **mcode** backend, not **gcc backend**, which additionally requires the system's matching GNAT/GCC runtime to already be installed via `apt` and isn't fixed by this change.
- The README's build instructions for all three platforms ended after compiling, with no instruction to actually run the result. Added a "Run" step to each, and pointed the Simulation backends table at OSS CAD Suite, which bundles Icarus, Yosys, both nextpnr variants, icepack, and ecppack in one download - the actual tool used to verify the Linux build this release. Also fixed the openFPGALoader link, which pointed at the repo root instead of its releases page like every other tool in that table.
- The README's Linux and macOS build instructions were missing `libcurl`, now a required build dependency since Linux/macOS support was added earlier in this same release - following the old instructions exactly would fail at the `cmake` step. Also updated "Tested On" to reflect that Linux has now actually been run end-to-end, not just compiled.
- Fixed a crash on first launch of the HDL Editor on Linux (GTK), caused by a colour value that's valid to pass on Windows but not on GTK.
- Browsing for a toolchain executable in Settings on Linux/macOS no longer defaults to a filter that hides every real match. The `*.exe` filter is Windows-only now; other platforms default straight to "All files".
- The About dialog's "Powered by" line no longer shows garbled middle dots ("Â·") on Windows - same root cause as the Settings dialog hints below: the string literal was decoded through the system codepage instead of UTF-8. Replaced with plain ASCII.
- The About dialog's heart symbol now shows on Windows too, not just Linux. It was a supplementary-plane emoji that needs a colour-emoji font Windows doesn't reliably provide for static text; replaced with a plain heart symbol in the same orange.
- "Powered by" now also credits Icarus and Yosys alongside GHDL, which were missing despite being equally core to the app's simulation and synthesis features.
- The Waveform Viewer's ΔT cursor-delta label no longer risks the same garbled-text issue as the About dialog fixes above (found by sweeping for the same pattern, not by anyone seeing it broken).
- The Settings dialog no longer shows garbled text in three hints. Real arrow and en-dash characters were corrupted at runtime because string literals get decoded through the system codepage, not UTF-8; replaced with plain ASCII.
- RTL View's message on a Verilog file no longer suggests running FPGA synthesis as a way to see something here - it doesn't read synthesis output, so that advice led nowhere.
- The FPGA synthesis resource report ("No synthesis statistics available.") now actually populates. The parser expected the cell count to follow the cell name; real Yosys output puts the count first, so every resource line silently failed to parse before.
- Yosys (and nextpnr, icepack/ecppack, openFPGALoader) no longer fail with a `.dll was not found` error, or a misleading "not found" message, when Settings points directly at an OSS CAD Suite `bin\` executable rather than launching through the suite's own environment script. The tool's `lib\` directory is now added to its PATH automatically.
- Run Simulation on a Verilog file now compiles the whole project, not just the file in the active tab. Running a testbench previously failed with `Unknown module type` unless Build had been pressed first in the same session.
- The same source file listed twice in a project, differing only in path separators, is no longer passed to the compiler twice. This produced a duplicate module declaration in Verilog and re-declared design units in VHDL.
- The Waveform Viewer now opens automatically after a Verilog simulation, the same way it already did for VHDL. The Verilog path previously finished without ever loading the waveform.
- Generated Verilog testbenches now wire up their ports when the module declares them on a single line, for example `module and_gate(input a, input b, output y);`. Previously the port list was dropped, producing a testbench with no signals and an empty instantiation that ran but left every input floating.
- Corrected a duplicated word in the Run Config bar's "Stop Time" tooltip.
- Corrected a misspelling in the Circuit Canvas "Record VCD" button tooltip.

---

## [1.0.0] - 2026-07-21

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
