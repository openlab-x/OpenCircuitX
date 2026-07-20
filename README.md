# OpenCircuitX

<div align="center">
  <img src="OpenCircuitX/res/app.ico" width="96"/>

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/License-MIT-green)
![Version](https://img.shields.io/badge/Version-1.0.0-gray)
![GUI](https://img.shields.io/badge/GUI-wxWidgets%203.2-blueviolet)

</div>

## About

OpenCircuitX is a free, open-source EDA (Electronic Design Automation) platform for VHDL and Verilog: the hardware description languages (HDL) used to design digital logic. It is developed by OpenLabX and unifies an IDE-quality HDL editor, a visual circuit canvas, an RTL schematic viewer, a waveform viewer, and a full FPGA toolchain in one window, for hardware engineers, students, and FPGA hobbyists alike.

See [Why OpenCircuitX?](#why-opencircuitx) below for how it compares to Active-HDL, ModelSim, Vivado, and Quartus.

## Download

- [x] **Windows 10/11 (x64)** - [Download the latest installer](https://github.com/openlab-x/OpenCircuitX/releases/latest/download/OpenCircuitX-Setup.exe). That link always points at the newest release; see [Requirements](#requirements) for details.
- [ ] **Linux** - no pre-built package yet, an AppImage is planned. [Build from source](#building) for now.
- [ ] **macOS** - no pre-built package yet, a DMG is planned. [Build from source](#building) for now.

## Table of Contents

- [About](#about)
- [Download](#download)
- [Why OpenCircuitX](#why-opencircuitx)
- [Design Domains](#design-domains)
- [Features](#features)
  - [HDL Editor](#hdl-editor)
  - [Code Intelligence](#code-intelligence)
  - [Simulation & Debug](#simulation--debug)
  - [Waveform Viewer](#waveform-viewer)
  - [Circuit Canvas](#circuit-canvas)
  - [RTL View](#rtl-view)
  - [FPGA Toolchain](#fpga-toolchain)
  - [IDE & Project](#ide--project)
  - [Plugin System](#plugin-system)
- [Screenshots](#screenshots)
- [Try It Yourself](#try-it-yourself)
- [Requirements](#requirements)
- [Tested On](#tested-on)
- [Building](#building)
- [GHDL Setup](#ghdl-setup)
- [FPGA Toolchain Setup](#fpga-toolchain-setup)
- [Keyboard Shortcuts](#keyboard-shortcuts)
- [Project File Format](#project-file-format)
- [File Formats](#file-formats)
- [Known Issues](#known-issues)
- [Changelog](#changelog)
- [Contributing](#contributing)
- [License](#license)
- [Built With](#built-with)
- [Contact](#contact)

---

## Why OpenCircuitX?

| Tool | Problem |
|---|---|
| Active-HDL | Windows-only, expensive, outdated UI |
| ModelSim | Terrible UI, expensive, scripting-heavy |
| Vivado | 50 GB install, locked to AMD/Xilinx |
| Quartus | Locked to Intel, large and complex |
| GHDL | No GUI, no IDE, command-line only |
| **OpenCircuitX** | Modern, keyboard-first UI; runs offline with no telemetry; free and open-source; no vendor lock-in |

**Scope note:** OpenCircuitX targets FPGA design (iCE40, ECP5), not ASIC tapeout. That's not a limitation to apologize for, it's the space where open-source EDA already works: foundries only qualify PDKs against Cadence/Synopsys/Siemens, so no open-source ASIC flow can sign off a real tapeout. FPGAs don't have that gatekeeper, Project IceStorm and Project Trellis already produce real bitstreams for real silicon.

---

## Design Domains

OpenCircuitX is built to grow from digital-only into a full EDA platform. When you launch the app, a welcome screen lets you pick your design domain:

| Domain | Status | Description |
|---|---|---|
| **Digital Design** | Available | Logic gates, HDL (VHDL/Verilog), simulation, waveforms, FPGA synthesis |
| **Analog Design** | Planned | Electronic components (R, C, L, transistors), schematic capture, SPICE-style simulation |
| **Mixed-Signal** | Planned | Unified digital + analog co-simulation, ADC/DAC modeling, shared environment |

---

## Features

### HDL Editor
- VHDL, Verilog, SystemVerilog syntax highlighting
- PCF (iCE40) and LPF (ECP5) constraint file highlighting
- Multi-tab editor - open multiple files, each with a close button
- Tab right-click context menu - Close, Close Other Tabs, Reveal in Explorer, Copy Path
- Modified indicator (`*`) in tab title; Ctrl+S to save; Ctrl+Shift+T to reopen last closed tab
- File templates - new `.vhd`/`.v`/`.sv` files open with boilerplate
- Smart auto-indent - respects `begin`/`end`, `{`/`}`, `process`, `always`, etc.
- Code folding toggle (View menu)
- Line number toggle (View menu)
- Word wrap toggle and show-whitespace toggle (View menu)
- Editor zoom - Ctrl+`=` / Ctrl+`-` / Ctrl+`0`
- Move line up/down - Alt+Up / Alt+Down
- Duplicate line - Ctrl+D
- Select all occurrences - Ctrl+Shift+L
- Jump to matching brace - Ctrl+`]`
- Sort selected lines alphabetically
- Toggle line comment - Ctrl+`/` (VHDL: `--`, Verilog: `//`)
- Bookmarks - Ctrl+F2 toggle, F2 next, Shift+F2 previous
- Inline find bar - Ctrl+F; Find+Replace bar - Ctrl+H; Next/Prev, wraps
- Find in Files - results in a navigable Find Results panel (double-click to jump)
- Replace in Files - Ctrl+Shift+H, walks all project source files
- Quick Open - Ctrl+P, fuzzy file picker across project sources
- Auto-save timer (every 3 minutes, configurable)
- Session restore - open tabs saved on exit and restored on next launch
- Error double-click - jumps to file:line:column from the Errors tab
- Inline error squiggles - red underline on exact error lines after a failed build
- Live VHDL syntax check on save - runs `ghdl -s` after every Ctrl+S; clears on success
- Multi-file Verilog compile - all `.v`/`.sv` project files passed to Icarus at once
- **Signal value tooltip** - hover an identifier for 600 ms; if a VCD is loaded, shows the signal's value at the current waveform cursor time

### Code Intelligence
- Outline panel in the left pane, updates live on every keystroke
  - VHDL: entity, architecture, port (in/out/inout), signal, process
  - Verilog/SV: module, input/output/inout, wire, reg, always/initial, assign
  - Double-click any node to jump to the exact line
- Auto-complete pulls symbol names straight from the outline and merges them with language keywords
- Go to Definition (F12) jumps to the first matching symbol across all project files
- Symbol Search (Ctrl+Shift+O) - live-filter dialog, arrow-key navigation, Enter to jump

### Simulation & Debug
- GHDL for VHDL-2008: compile (`F5`), run (`F6`), debug (`F7`)
- Icarus Verilog for `.v`/`.sv`: compile + run (`F5`/`F6`)
- Verilator for linting (`F8`) and fast simulation (`Shift+F8`)
- Run Config Bar - always-visible entity field, stop time field, VHDL standard selector (2008/1993/2019); changes persist per project
- Every simulation run auto-generates a `.vcd` and switches you straight to the Waveform tab
- Output panel splits Output and Errors into separate tabs; errors show in red and the tab opens automatically on failure
- Watch panel - add named signals and watch their values update live as you move the waveform cursor
- Breakpoints - click the gutter to place/remove red circle markers; `GetBreakpoints()` returns the line list for GHDL `--break-at` if you're scripting around it
- Stop-time is configurable in Tools > Settings and gets appended as `--stop-time=<value>` on the GHDL run
- Smart testbench generator auto-detects `clk`/`rst` ports and writes a clock process + stimulus that's directly simulatable by GHDL

### Waveform Viewer
- Built-in VCD parser, no GTKWave needed
- Signals appear once even when GHDL emits both testbench and UUT ports for the same net (duplicate-scope deduplication)
- 1-bit signals render as a green step waveform with a filled HIGH region and `1`/`0` labels on each segment
- Bus signals render as a blue parallelogram with a hex value label per segment
- Time axis auto-scales its tick interval and draws vertical grid lines
- Click to place the cursor - shows time in the toolbar; step it with `<< Step` / `Step >>`
- Right-click → "Set Reference Here" drops a green dotted reference line; the toolbar shows `dT = <delta><unit>` between cursor and reference
- Zoom In / Out / Fit to window, or mouse-wheel zoom anchored to the pointer; Fit re-runs on window resize
- Signal name panel shows the value sequence under each name (e.g. `0 1 0 1` / `00 FF 3C`)
- "Table" button toggles a truth-table view - every timestamp as a row, colored 0/1 cells, click a row to jump the cursor there (and vice versa)
- Filter signals by name, or find the next/previous occurrence of a given value on any signal
- Jump the cursor straight to the nearest value change on the selected signal (previous/next transition)
- Drag to reorder signals; right-click to remove one or add it to Watch
- Right-click to add named vertical amber markers - rename or remove them the same way
- Export the waveform as PNG (with the signal names column) or CSV
- Save/load a waveform session - VCD path, zoom, cursor, filter, unit, markers - as `.ocxwave`

### Circuit Canvas
- Drag-and-drop logic gates: AND, OR, NOT, NAND, NOR, XOR, XNOR, D Flip-Flop, MUX 2:1, Half Adder, Full Adder, Clock, Tri-state Buffer, DEMUX 1:2
- Placement hotkeys for every gate (`A` AND, `O` OR, `N` NOT, `X` XOR, `G` NAND, `R` NOR, `P` XNOR, `I` INPUT, `U` OUTPUT, `C` CLOCK, `D` D-FlipFlop, `M` MUX, `H` Half Adder, `F` Full Adder); Shift+Click drops multiple copies
- Snap-to-grid (20 px) on a scrollable 2000×2000 canvas
- Wire drawing: click an output pin, then an input pin, Manhattan routing fills in the path
- Click input pins to toggle high/low; a live 3-pass forward simulation updates pin colors to match state
- Right-click → "Add Note Here..." for canvas annotations - drag to reposition, double-click to edit, saved in `.ocxschem`
- Clipboard: Copy / Paste / Select All / Delete Selected (Edit > Canvas submenu)
- Alignment tools: Align Left, Right, Top, Bottom, Center H, Center V
- Visual animation with Play/Pause/Step controls, adjustable speed (1–10), wire pulse effects, and gate glow on transition
- "Record VCD" captures every gate's output each animation tick, auto-stopping at 200 ticks and loading straight into the Waveform tab
- Undo/Redo (Ctrl+Z/Y) routes to the canvas on the canvas tab, or to the HDL editor's Scintilla undo stack on the editor tab
- Export to VHDL structural description, opening directly in the editor tab
- Export to Verilog via a file dialog, with the option to open it in the editor
- Export the canvas itself as a PNG
- Save/load canvas layouts as plain-text `.ocxschem` files

### RTL View
Parses the active VHDL file and renders a full combinational schematic - no external tools required.

- Left-to-right layout: primary inputs on the far left, primary outputs on the far right, logic gates bucketed into columns by level (computed via iterative topological relaxation)
- ANSI/IEEE gate shapes (AND, OR, NOT, NAND, NOR, XOR, XNOR) rendered via the same `ANSIGateRenderer` used in the circuit canvas
- H-V-H wire routing with channel allocation, so wires never overlap; fan-out junction dots drawn automatically
- Multi-input chains like `a and b and c` parse as a single 3-input AND gate, not two separate gates
- Netlist tree sidebar (Inputs / Outputs / Signals / Gates) stays in sync with the schematic in both directions - click an item in the tree or a gate in the schematic and the other follows
- Click a gate to select it: gold border and label, plus colour-highlighted wires - output in bright green, inputs in orange, everything else dimmed blue
- Zoom with Ctrl+mouse wheel (15%–400%), or the toolbar − / + / Fit buttons
- Right-drag to pan without triggering a context menu
- Export the schematic to PNG at 1:1 zoom
- Scroll position and gate selection survive tab switches - coming back after an edit lands you where you left off

### FPGA Toolchain
Universal, no vendor lock: works with any supported board, no single-vendor toolchain required.

- Yosys synthesis - VHDL via ghdl-yosys-plugin, Verilog natively; writes a `.ys` script to avoid shell quoting issues
- iCE40 flow: nextpnr-ice40 → icepack → openFPGALoader
- ECP5 flow: nextpnr-ecp5 → ecppack → openFPGALoader
- Supported boards:
  - iCEBreaker (UP5K), iCE40-HX8K, TinyFPGA BX, UPduino v3
  - ColorLight i5 (ECP5-25F), OrangeCrab r0.2 (ECP5-25F), ULX3S 85F (ECP5-85F)
- Resource report - LUT/FF/BRAM Used/Total/% table in Output tab
- PCF (iCE40) and LPF (ECP5) constraint files - syntax-highlighted in editor

### IDE & Project
- Welcome screen on startup when no project is open - three domain cards (Digital / Analog / Mixed-Signal) let you pick a design domain; Analog and Mixed-Signal carry "Coming Soon" badges; recent projects list for one-click re-open
- Three themes - Dark (VS Code Dark+), Midnight (GitHub Dark), Light (VS Code Light) - switchable at runtime
- Custom dark title bar on Windows 11
- `.ocxproj` project files use a human-readable INI format that's friendly to version control
- New Project dialog with 9 templates: VHDL Entity, VHDL Package, VHDL Testbench, Verilog Module, SystemVerilog Module, D Flip-Flop, 7-segment decoder, Full Adder, JK Flip-Flop
- Project explorer with right-click create file/folder, rename, delete
- Toolbar and project tree use Google Material Design filled icons (via wxMaterialDesignArtProvider), with semantic colors - green Run, orange Debug, amber Build, gold folders, VS-blue source files - that update automatically on theme switch
- File type icons: folder (gold), VHDL/Verilog source (blue), hardware project (teal), generic file
- Open tabs and up to 9 recent projects are restored across sessions
- Searchable keyboard shortcuts dialog (Help > Keyboard Shortcuts, or Ctrl+Shift+?) with 40+ entries
- Branded splash screen on launch, auto-dismissing after 2.5s

### Plugin System
- Drop a `.dll` / `.so` / `.dylib` into the `plugins/` folder next to the executable and it loads automatically
- Plugins get `LogMessage`, `LogError`, and `AddToolsMenuItem` callbacks to work with
- Four required exports: `ocx_plugin_name`, `ocx_plugin_version`, `ocx_plugin_init`, `ocx_plugin_shutdown`
- Full C interface documented in `src/core/plugin/plugin_api.h`

---

## Screenshots

<p align="center">
  <img src="screenshots/01_splash.png" width="600" alt="OpenCircuitX splash screen" />
</p>

<p align="center">
  <img src="screenshots/03_hdl_editor.png" width="800" alt="HDL Editor with syntax highlighting and outline panel" /><br />
  <em>HDL Editor: VHDL/Verilog syntax highlighting, live outline panel, build output</em>
</p>

<p align="center">
  <img src="screenshots/04_circuit_canvas.png" width="800" alt="Circuit Canvas with a live simulation" /><br />
  <em>Circuit Canvas: drag-and-drop gates with live simulation feedback on the wires</em>
</p>

<p align="center">
  <img src="screenshots/05_waveform_viewer.png" width="800" alt="Waveform Viewer with signal value table" /><br />
  <em>Waveform Viewer: VCD traces plus a readable Signal Value Table</em>
</p>

<p align="center">
  <img src="screenshots/06_rtl_view.png" width="800" alt="RTL View with a gate selected" /><br />
  <em>RTL View: gate-level schematic parsed directly from VHDL, no external tools required</em>
</p>

<p align="center">
  <img src="screenshots/07_rtl_view_exported_png.png" width="700" alt="RTL View schematic exported to PNG" /><br />
  <em>RTL View's Export PNG output, an entity diagram and gate-level schematic at 1:1 zoom</em>
</p>

See the [documentation site](docs/index.html) for a full walkthrough with more screenshots.

---

## Try It Yourself

The screenshots above are all the same example project: [`examples/gate_demo`](examples/gate_demo), a small 2-input, 7-gate VHDL design (AND/OR/XOR/NAND/NOR/XNOR/NOT) with a matching testbench. It's included in this repository so you can open the exact same project and reproduce every screenshot yourself:

1. Clone or download this repository.
2. In OpenCircuitX, **Open Project** and select `examples/gate_demo/gate_demo.ocxproj`.
3. Open `gate_demo.vhd` in the HDL Editor, or press **F6** to run the testbench and jump straight to the Waveform Viewer.
4. Switch to the **RTL View** tab to see the gate-level schematic, or **Circuit Canvas** to build the same logic by hand.

---

## Requirements

| Platform | Requirement |
|---|---|
| Windows 10/11 | Visual Studio 2022 (v143), wxWidgets 3.2.10 |
| Linux | GCC/Clang, CMake 3.20+, wxWidgets 3.x dev package |
| macOS | Clang, CMake 3.20+, wxWidgets 3.x (Homebrew) |

Simulation backends (optional - set paths in Tools > Settings):

| Tool | Purpose |
|---|---|
| [GHDL](https://github.com/ghdl/ghdl/releases) | VHDL compile + simulation |
| [Icarus Verilog](https://github.com/steveicarus/iverilog) | Verilog compile + simulation |
| [Verilator](https://www.veripool.org/verilator) | Verilog lint + fast simulation |
| [Yosys](https://github.com/YosysHQ/yosys) (via [Project IceStorm](https://clifford.at/icestorm)) | FPGA synthesis |
| [nextpnr-ice40](https://github.com/YosysHQ/nextpnr) | iCE40 place & route |
| [nextpnr-ecp5](https://github.com/YosysHQ/nextpnr) | ECP5 place & route |
| icepack (bundled with [Project IceStorm](https://clifford.at/icestorm)) | iCE40 bitstream packing |
| ecppack (bundled with [Project Trellis](https://github.com/YosysHQ/prjtrellis)) | ECP5 bitstream packing |
| [openFPGALoader](https://github.com/trabucayre/openFPGALoader) | Board programming (USB) |
| [ghdl-yosys-plugin](https://github.com/ghdl/ghdl-yosys-plugin) | VHDL → Yosys bridge (for FPGA synthesis) |

All of these are covered in more detail in [GHDL Setup](#ghdl-setup) and [FPGA Toolchain Setup](#fpga-toolchain-setup) below.

---

## Tested On

**Actually tested:** Windows 11 (x64), Visual Studio 2022, wxWidgets 3.2.10. The HDL editor, Circuit Canvas, waveform viewer, and simulation backends (GHDL, Icarus Verilog, Verilator) have all been exercised on this setup.

**Not yet tested:**
- Linux and macOS builds. The CMake build targets both, but hasn't been run end-to-end on either platform yet.
- Programming real FPGA hardware. Synthesis and bitstream generation (Yosys, nextpnr, icepack/ecppack) work, but no board listed under FPGA Toolchain has actually been programmed and verified, none of that hardware has been purchased yet.

If you try either of these and run into something, please open an issue, that feedback is genuinely useful.

---

## Building

### Windows (Visual Studio 2022)

<details>
<summary>Why Visual Studio 2022, not older or newer?</summary>

- **Not an older version (2019 and before):** Visual Studio 2022 has full C++20 support and partial C++23 support. Visual Studio 2019 only stabilized C++20 very late, after a significant backport effort, and never got full C++23 support at all.
- **Not the newer Visual Studio 2026:** it's a very recent release still working through launch issues. Microsoft's own C++ team blog reports 416 C++ compiler bugs fixed in just the first six months after release, and documented parser/compiler issues (range-for expressions inside template functions with lambdas, internal compiler errors with member function pointers and constexpr references) were still outstanding at launch. Visual Studio 2022 is mature, at version 17.14, with years of fixes behind it.

</details>

**1. Install wxWidgets**

Download wxWidgets 3.2.10 from the [official downloads page](https://www.wxwidgets.org/downloads/) or directly from [GitHub releases](https://github.com/wxWidgets/wxWidgets/releases/tag/v3.2.10). Extract it to `C:\wxWidgets` or any path you like, then open its own solution in Visual Studio and build both the `Release | x64` and `Debug | x64` static library configurations (`vc_x64_lib`) before opening OpenCircuitX's solution.

**2. Set WXWIN environment variable**

```
setx WXWIN "C:\wxWidgets"
```

Restart Visual Studio after setting this.

**3. Open and build**

Open `OpenCircuitX.sln`, select `x64 | Release`, build.

---

### Linux

```bash
sudo apt install libwxgtk3.2-dev   # Debian/Ubuntu
# or
sudo dnf install wxGTK-devel       # Fedora

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### macOS

```bash
brew install wxwidgets

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

---

## GHDL Setup

1. Download GHDL from [github.com/ghdl/ghdl/releases](https://github.com/ghdl/ghdl/releases)
2. Extract to e.g. `C:\ghdl`
3. In OpenCircuitX: **Tools > Settings > Simulation tab**
4. Set GHDL path to `C:\ghdl\bin\ghdl.exe`

---

## FPGA Toolchain Setup

Install the open-source toolchain for your target family:

- **iCE40**: [Project IceStorm](https://clifford.at/icestorm) - provides `yosys`, `nextpnr-ice40`, `icepack`
- **ECP5**: [Project Trellis](https://github.com/YosysHQ/prjtrellis) - provides `nextpnr-ecp5`, `ecppack`
- **Board programming**: [openFPGALoader](https://github.com/trabucayre/openFPGALoader)
- **VHDL synthesis**: [ghdl-yosys-plugin](https://github.com/ghdl/ghdl-yosys-plugin)

Then set all paths in **Tools > Settings > FPGA tab**.

---

## Keyboard Shortcuts

The app has its own searchable shortcuts dialog (Help > Keyboard Shortcuts, or `Ctrl+Shift+?`) with 40+ entries covering the editor, canvas, navigation, and build/simulation. The full reference also lives on the [documentation site](docs/index.html).

---

## Project File Format

`.ocxproj` files use a readable INI format:

```ini
[OpenCircuitX]
Version=1.0.0
ProjectName=counter
Author=
License=MIT

[Language]
Primary=VHDL
VHDLStandard=2008

[Files]
Count=2
File0=counter.vhd
File1=tb_counter.vhd

[Simulation]
TopEntity=tb_counter
RunTime=1000ns

[Editor]
OpenTabs=counter.vhd,tb_counter.vhd
```

---

## File Formats

| Extension | Purpose |
|---|---|
| `.ocxproj` | Project file |
| `.ocxschem` | Circuit canvas schematic |
| `.ocxsim` | Simulation save file |
| `.ocxwave` | Waveform session |
| `.ocxconst` | Timing / pin constraints |
| `.vhd` / `.vhdl` | VHDL source |
| `.v` / `.sv` | Verilog / SystemVerilog |
| `.pcf` | iCE40 physical constraints |
| `.lpf` | ECP5 Lattice preference file |
| `.vcd` | Value Change Dump (waveforms) |

---

## Known Issues

- The Gate Scheme Renderer currently supports ANSI shapes only; IEC/IEEE and DIN-style symbols are planned.
- The plugin loader (`.dll`/`.so`/`.dylib`) has no in-app UI yet to enable or disable a loaded plugin.
- No automated test suite yet; each release is verified with a manual pass on a Release x64 build.

---

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for release notes.

---

## Contributing

We welcome contributions.

1. Give the project a star.
2. Fork the repository.
3. Create a branch for your feature or fix.
4. Make your changes, following the code style in the [Contributing Guide](docs/contributing.html).
5. Open a pull request against `main`, describing what you built and why.

See [CONTRIBUTING.md](CONTRIBUTING.md) for the full guide.

---

## License

MIT License - Copyright (c) 2026 OpenLabX

---

## Built With

- C++ 17
- [wxWidgets 3.2.10](https://wxwidgets.org) - cross-platform GUI
- [wxMaterialDesignArtProvider](https://github.com/perazz/wxMaterialDesignArtProvider) - Material Design SVG icons
- [GHDL](https://github.com/ghdl/ghdl) - VHDL simulator
- [Icarus Verilog](https://github.com/steveicarus/iverilog) - Verilog simulator
- [Verilator](https://www.veripool.org/verilator) - fast Verilog sim
- [Yosys](https://github.com/YosysHQ/yosys) - open synthesis
- [nextpnr](https://github.com/YosysHQ/nextpnr) - place & route
- [openFPGALoader](https://github.com/trabucayre/openFPGALoader) - board programming

---

## Contact

In pursuit of innovation,
**OpenLabX Team**

- **Website**: [https://openlabx.com](https://openlabx.com)
- **Email**: contact@openlabx.com

**Follow Us:**

<div align="center">

| <a href="https://www.instagram.com/openlabx_official/" target="_blank"><strong>Instagram</strong></a> | <a href="https://x.com/openlabx" target="_blank"><strong>X (formerly Twitter)</strong></a> | <a href="https://www.facebook.com/openlabx/" target="_blank"><strong>Facebook</strong></a> | <a href="https://www.youtube.com/@OpenLabX" target="_blank"><strong>YouTube</strong></a> | <a href="https://github.com/openlab-x" target="_blank"><strong>GitHub</strong></a> |
|---|---|---|---|---|

</div>
