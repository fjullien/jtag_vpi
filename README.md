jtag_vpi
========

TCP/IP controlled VPI JTAG Interface.

    +------------------+     +-----------------+     +------------------+      +----------+
    +                  +     +                 +     +                  +      +          +
    + Testbench client + <=> + JTAG VPI server + <-> + JTAG VPI verilog + <--> + JTAG TAP +
    +                  +     +                 +     +                  +      +          +
    +------------------+     +-----------------+     +------------------+      +----------+
        test_client.c             jtag_vpi.c               jtag_vpi.v             any tap...
    -------------------- TCP  ------------------  VPI ---------------------   --------------
    --------------------      ---------------------------------------------   --------------

A testbench is provided and can be run with:

    cd sim/run
    make sim

This simulation requires icarus verilog.
Result output is a waveform in VCD format.

jtagServer
==========

This a TCP/IP controlled JTAG running with verilator.

    +------------------+     +-----------------+     +---------------------------------+
    +                  +     +                 +     +                                 +
    + Testbench client + <=> +    jtagServer   + <-> + Verilated model with a JTAG TAP +
    +                  +     +                 +     +                                 +
    +------------------+     +-----------------+     +---------------------------------+
        test_client.c           jtagServer.cpp                a soc with any tap...
    -------------------- TCP  -------------------- Verilator -------------------------------
    --------------------      --------------------------------------------------------------

A verilator testbench is provided and can be run with:

    cd sim/run
    make sim_verilator

This simulation requires verilator.
Result output is a waveform in VCD format.
