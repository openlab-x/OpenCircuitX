# Examples

One small design per supported toolchain, so each simulation/synthesis path can be tried and reproduced on its own rather than only through the main README's screenshots.

| Folder | Toolchain | Demonstrates |
|--------|-----------|---------------|
| [`ghdl/gate_demo`](ghdl/gate_demo) | GHDL (VHDL) | HDL Editor, Run Simulation, Waveform Viewer, RTL View - the project behind every screenshot in the main [README](../README.md) |
| [`icarus/and_gate`](icarus/and_gate) | Icarus Verilog | Same workflow as `gate_demo`, through the Verilog/Icarus path instead of VHDL/GHDL |
| [`yosys/and_gate`](yosys/and_gate) | Yosys + Icarus | FPGA synthesis (F9) and simulation (F6) side by side in one project |

Each folder has its own `.ocxproj` and its own README with exact steps. Open the `.ocxproj` directly with **Open Project**, don't open the loose `.v`/`.vhd` files on their own - the project file is what tells OpenCircuitX which files belong together and what the top-level entity is.

## Why they're split by toolchain

VHDL and Verilog take different paths through the app end to end (different compilers, different waveform-dump mechanics, different RTL View support), so a single mixed example would hide which parts of the app are exercising what. GHDL and Icarus are for simulation; Yosys is for synthesis, a different step in the flow with no waveform involved at all.

## Requirements

All three tools ship together in [OSS CAD Suite](https://github.com/YosysHQ/oss-cad-suite-build) - one download covers every example here. Individually: GHDL, Icarus Verilog (`iverilog`/`vvp`), and Yosys, each configurable under Tools > Settings if not already on your PATH.
