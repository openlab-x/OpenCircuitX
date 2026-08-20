`timescale 1ns / 1ps

module tb_and_gate;

    reg a;
    reg b;
    wire y;

    initial begin
        a = 0;
        b = 0;
    end

    // Waveform output - Icarus has no --vcd flag, the testbench has to ask
    // for it directly. Required for the Waveform tab to load anything.
    initial begin
        $dumpfile("tb_and_gate.vcd");
        $dumpvars(0, tb_and_gate);
    end

    and_gate DUT (
        .a(a),
        .b(b),
        .y(y)
    );

    initial begin
        // Walk all four input combinations, 20 ns each
        a = 0; b = 0; #20;
        a = 0; b = 1; #20;
        a = 1; b = 0; #20;
        a = 1; b = 1; #20;
        #20;
        $finish;
    end

endmodule

