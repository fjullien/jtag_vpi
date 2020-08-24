/*
 * Test bench for VPI JTAG Interface
 *
 * Copyright (C) 2012 Franck Jullien, <franck.jullien@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation  and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of any
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

module jtag_vpi_tb;

wire		tdo_pad_o;
wire		tck_pad_i;
wire		tms_pad_i;
wire		tdi_pad_i;

wire		jtag_tap_tdo;
wire		jtag_tap_shift_dr;
wire		jtag_tap_pause_dr;
wire		jtag_tap_update_dr;
wire		jtag_tap_capture_dr;
wire		dbg_if_tdo;
wire		dbg_if_select;

wire	[31:0]	wb_adr;
wire	[31:0]	wb_dat;
wire	[3:0]	wb_sel;
wire		wb_we;
wire	[1:0]	wb_bte;
wire	[2:0]	wb_cti;
wire		wb_cyc;
wire		wb_stb;
wire		wb_ack;
wire		wb_err;
wire	[31:0]	wb_sdt;

reg		sys_clock = 0;
reg		sys_reset = 0;

initial begin
	$dumpfile("jtag_vpi.vcd");
	$dumpvars(0);
end

always
	#20 sys_clock <= ~sys_clock;

initial begin
	#100 sys_reset <= 1;
	#200 sys_reset <= 0;
end

jtag_vpi #(.DEBUG_INFO(0))
jtag_vpi0
(
	.tms(tms_pad_i),
	.tck(tck_pad_i),
	.tdi(tdi_pad_i),
	.tdo(tdo_pad_o),

	.enable(1'b1),
	.init_done(1'b1)
);

jtag_soc jtag_soc0
(
	.sys_clock			(sys_clock),
	.sys_reset			(sys_reset),
	.tdo_pad_o			(tdo_pad_o),
	.tms_pad_i			(tms_pad_i),
	.tck_pad_i			(tck_pad_i),
	.tdi_pad_i			(tdi_pad_i)
);

endmodule
