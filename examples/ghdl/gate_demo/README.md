# gate_demo (GHDL / VHDL)

A 2-input, 7-gate combinational design (AND/OR/XOR/NAND/NOR/XNOR/NOT) with a matching testbench. This is the project used for every VHDL/HDL Editor/Waveform/RTL View screenshot in the main [README](../../../README.md).

## Requirements

GHDL on your PATH, or configured under Tools > Settings.

## Try it

1. **Open Project** and select `gate_demo.ocxproj`.
2. Open `gate_demo.vhd` in the HDL Editor, or press **F6** to run the testbench and jump straight to the Waveform Viewer.
3. Switch to the **RTL View** tab to see the parsed schematic.

## Files

- `gate_demo.vhd` - the design
- `tb_gate_demo.vhd` - testbench, steps through all 4 input combinations at 20 ns each
- `gate_demo.ocxproj` - project file
