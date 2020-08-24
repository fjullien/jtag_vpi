/*
 * Verilator test bench for jtagServer
 *
 * Copyright (C) 2020 Franck Jullien, <franck.jullien@gmail.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdlib.h>
#include "Vjtag_soc.h"
#include "verilated.h"
#include "../../jtagServer.h"
#include <verilated_vcd_c.h>

vluint64_t tick = 0;

int main(int argc, char **argv)
{
	Verilated::commandArgs(argc, argv);
	Verilated::traceEverOn(true);

	VerilatedVcdC	*m_trace = new VerilatedVcdC;

	Vjtag_soc *jtag_soc = new Vjtag_soc;

	VerilatorJtagServer* jtag = new VerilatorJtagServer(10);
	jtag->init_jtag_server(5555, false);

	jtag_soc->trace(m_trace, 99);

	m_trace->open("sim.vcd");

	jtag_soc->sys_reset = 1;
	jtag_soc->sys_clock = 0;

	// Tick the clock until we are done
	while (jtag->stop_simu == false) {

		jtag_soc->sys_clock = !jtag_soc->sys_clock;
		jtag_soc->eval();
		
		if (tick > 500)
			jtag->doJTAG(tick, &jtag_soc->tms_pad_i, &jtag_soc->tdi_pad_i, &jtag_soc->tck_pad_i, jtag_soc->tdo_pad_o);

		if (tick > 100)
			jtag_soc->sys_reset = 0;

		m_trace->dump(tick);

		tick++;
	}

	m_trace->close();

	printf("Simulation finished !\n");
}
