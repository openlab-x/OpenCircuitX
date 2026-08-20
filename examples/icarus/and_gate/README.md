# and_gate (Icarus Verilog)

A single 2-input AND gate with a matching testbench. Deliberately minimal - this is the Verilog equivalent of `ghdl/gate_demo`, and exercises the same Run Simulation / Waveform Viewer workflow through Icarus instead of GHDL.

## Requirements

`iverilog` and `vvp` on your PATH, or configured under Tools > Settings. Both ship with [OSS CAD Suite](https://github.com/YosysHQ/oss-cad-suite-build).

## Try it

1. **Open Project** and select `and_gate.ocxproj`.
2. Open `tb_and_gate.v`, then press **F6** (Run Simulation).
3. The Waveform tab opens automatically. `y` should go high only in the last 20 ns window, when both `a` and `b` are 1.

## A note on the testbench

`tb_and_gate.v`'s stimulus block walks all four input combinations explicitly instead of leaving a `// TODO: add test stimulus here` stub. A freshly *generated* testbench (Tools > Generate Testbench) will have that stub, and running it as-is produces a flat, uninteresting waveform - not a bug, just nothing to look at yet. Fill in real stimulus before expecting to see anything.

## Why RTL View shows nothing here

RTL View currently parses VHDL only. Opening `and_gate.v` there will show an explanation rather than a schematic - there's no workaround, RTL View doesn't read synthesis output either (see `yosys/and_gate`'s README for why).

## Files

- `and_gate.v` - the design
- `tb_and_gate.v` - testbench with real stimulus and a `$dumpfile`/`$dumpvars` block (required for Icarus to produce a `.vcd` at all - it has no `--vcd` flag like GHDL does)
- `and_gate.ocxproj` - project file
