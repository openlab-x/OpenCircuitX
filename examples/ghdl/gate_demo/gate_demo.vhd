library ieee;
use ieee.std_logic_1164.all;

entity gate_demo is
    port (
        a : in  std_logic;
        b : in  std_logic;

        y_and  : out std_logic;
        y_or   : out std_logic;
        y_xor  : out std_logic;
        y_nand : out std_logic;
        y_nor  : out std_logic;
        y_xnor : out std_logic;
        y_not  : out std_logic
    );
end entity gate_demo;

architecture rtl of gate_demo is
begin

    y_and  <= a and  b;
    y_or   <= a or   b;
    y_xor  <= a xor  b;
    y_nand <= a nand b;
    y_nor  <= a nor  b;
    y_xnor <= a xnor b;
    y_not  <= not a;

end architecture rtl;


