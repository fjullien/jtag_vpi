jtag_vpi
========

TCP/IP controlled VPI JTAG Interface.

    +---------------     +----------------+      +----------------+      +------------------+      +----------+
    +              +     +                +      +                +      +                  +      +          +
    + OpenOCD core + --> + JAG VIP driver + <==> + JAG VIP server + <--> + JTAG VPI verilog + <--> + JTAG TAP +
    +              +     +                +      +                +      +                  +      +          +
    +---------------     +----------------+      +----------------+      +------------------+      +----------+
                             jtag_vpi.c              jtag_vpi.c               jtag_vpi.v             any tap...
    --------------------------------------- TCP  ------------------  VPI --------------------------------------
    ---------------------------------------      --------------------------------------------------------------
                  OpenOCD                                                VPI + Verilog RTL

A testbench is provided and can be run with:

    cd sim/run
    make sim

This simulation requires icarus verilog.
Result output is a waveform in VCD format.
