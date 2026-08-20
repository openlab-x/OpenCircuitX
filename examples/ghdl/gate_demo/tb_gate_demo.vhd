library ieee;
use ieee.std_logic_1164.all;

entity tb_gate_demo is
end tb_gate_demo;

architecture sim of tb_gate_demo is

    component gate_demo
        port (
            a      : in  std_logic;
            b      : in  std_logic;
            y_and  : out std_logic;
            y_or   : out std_logic;
            y_xor  : out std_logic;
            y_nand : out std_logic;
            y_nor  : out std_logic;
            y_xnor : out std_logic;
            y_not  : out std_logic
        );
    end component;

    signal a, b : std_logic;
    signal y_and, y_or, y_xor, y_nand, y_nor, y_xnor, y_not : std_logic;

begin

    DUT: gate_demo
        port map (
            a => a, b => b,
            y_and => y_and, y_or => y_or, y_xor => y_xor,
            y_nand => y_nand, y_nor => y_nor, y_xnor => y_xnor,
            y_not => y_not
        );

    -- Step through all four input combinations, 20 ns each
    stim_proc: process
    begin
        a <= '0'; b <= '0'; wait for 20 ns;
        a <= '0'; b <= '1'; wait for 20 ns;
        a <= '1'; b <= '0'; wait for 20 ns;
        a <= '1'; b <= '1'; wait for 20 ns;
        wait;
    end process;

end sim;





