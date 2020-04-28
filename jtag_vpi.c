/*
 * TCP/IP controlled VPI JTAG Interface.
 * Based on Julius Baxter's work on jp_vpi.c
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

#include <arpa/inet.h>

#include <vpi_user.h>

#include "jtag_common.h"

static void terminate_simulation(void)
{
	vpi_control(vpiFinish, 1);
}

void vpi_check_for_command(char *userdata)
{
	vpiHandle systfref, args_iter, argh;
	struct t_vpi_value argval;
	struct jtag_cmd cmd;
	unsigned loaded_words = 0;
	int ret;
	(void)userdata;

	// See if there is an incoming JTAG command
	ret = check_for_command(&cmd);
	if (ret == JTAG_SERVER_TRY_LATER) {
		// No command from OpenOCD at this time
		return;
	}
	else if (ret != JTAG_SERVER_SUCCESS) {
		if (ret == JTAG_SERVER_CLIENT_DISCONNECTED) {
			// OpenOCD disconnected
			vpi_printf("Ending simulation. Reason: jtag_vpi client disconnection.\n");
		}
		else {
			// A communication error ocurred, cannot continue
			vpi_printf("Ending simulation. Reason: Error communicating with OpenOCD.\n");
		}
		terminate_simulation();
		return;
	}

	// New command from OpenOCD arrived. Push it to the RTL simulation:

	/************* vpi.cmd to VPI ******************************/

	// Obtain a handle to the argument list
	systfref = vpi_handle(vpiSysTfCall, NULL);
	// Now call iterate with the vpiArgument parameter
	args_iter = vpi_iterate(vpiArgument, systfref);
	// get a handle on the variable passed to the function
	argh = vpi_scan(args_iter);
	// now store the command value back in the sim
	argval.format = vpiIntVal;
	// Now set the command value
	vpi_get_value(argh, &argval);

	argval.value.integer = (uint32_t)cmd.cmd;

	// And vpi_put_value() it back into the sim
	vpi_put_value(argh, &argval, NULL, vpiNoDelay);

	/************* vpi.length to VPI ******************************/

	// now get a handle on the next object (memory array)
	argh = vpi_scan(args_iter);
	// now store the command value back in the sim
	argval.format = vpiIntVal;
	// Now set the command value
	vpi_get_value(argh, &argval);

	argval.value.integer = (uint32_t)cmd.length;

	// And vpi_put_value() it back into the sim
	vpi_put_value(argh, &argval, NULL, vpiNoDelay);

	/************* vpi.nb_bits to VPI ******************************/

	// now get a handle on the next object (memory array)
	argh = vpi_scan(args_iter);
	// now store the command value back in the sim
	argval.format = vpiIntVal;
	// Now set the command value
	vpi_get_value(argh, &argval);

	argval.value.integer = (uint32_t)cmd.nb_bits;

	// And vpi_put_value() it back into the sim
	vpi_put_value(argh, &argval, NULL, vpiNoDelay);

	/*****************vpi.buffer_out to VPI ********/

	// now get a handle on the next object (memory array)
	argh = vpi_scan(args_iter);
	vpiHandle array_word;

	// Loop to load the words
	while (loaded_words < cmd.length) {
		// now get a handle on the current word we want in the array that was passed to us
		array_word = vpi_handle_by_index(argh, loaded_words);

		if (array_word != NULL) {
			argval.value.integer = (uint32_t)cmd.buffer_out[loaded_words];
			// And vpi_put_value() it back into the sim
			vpi_put_value(array_word, &argval, NULL, vpiNoDelay);
		} else
			return;

		loaded_words++;
	}

	/*******************************************/

	// Cleanup and return
	vpi_free_object(args_iter);
}

void vpi_send_result_to_server(char *userdata)
{
	vpiHandle systfref, args_iter, argh;
	struct t_vpi_value argval;
	struct jtag_cmd cmd;

	int32_t length;
	int sent_words;

	vpiHandle array_word;

	(void)userdata;

	// Now setup the handles to verilog objects and check things
	// Obtain a handle to the argument list
	systfref = vpi_handle(vpiSysTfCall, NULL);

	// Now call iterate with the vpiArgument parameter
	args_iter = vpi_iterate(vpiArgument, systfref);

	// get a handle on the length variable
	argh = vpi_scan(args_iter);

	argval.format = vpiIntVal;

	// get the value for the length object
	vpi_get_value(argh, &argval);

	// now set length
	length = argval.value.integer;

	// now get a handle on the next object (memory array)
	argh = vpi_scan(args_iter);

	// check we got passed a memory (array of regs)
	
	if (!((vpi_get(vpiType, argh) == vpiMemory) || (vpi_get(vpiType, argh) == vpiRegArray))) {
		vpi_printf("jtag_vpi: ERROR: did not pass a memory/regArray to get_command_block_data\n");
		vpi_printf("jtag_vpi: ERROR: was passed type %d\n", (int)vpi_get(vpiType, argh));
		return;
	}

	// check the memory we're writing into is big enough
	if (vpi_get(vpiSize, argh) < length ) {
		vpi_printf("jtag_vpi: ERROR: buffer passed to get_command_block_data too small. size is %d words, needs to be %d\n",
		vpi_get(vpiSize, argh), length);
		return;
	}

	// Loop to load the words
	sent_words = 0;
	while (sent_words < length) {
		// Get a handle on the current word we want in the array that was passed to us
		array_word = vpi_handle_by_index(argh, sent_words);

		if (array_word != NULL) {
			vpi_get_value(array_word, &argval);
			cmd.buffer_in[sent_words] = (uint32_t) argval.value.integer;
		} else
			return;

		sent_words++;
	}

	if (send_result_to_server(&cmd) != JTAG_SERVER_SUCCESS) {
		vpi_printf("Ending simulation. Reason: Cannot send data back to OpenOCD.\n");
		terminate_simulation();
		return;
	}

	// Cleanup and return
	vpi_free_object(args_iter);
}

void print_func(char *msg)
{
	vpi_printf("%s", msg);
}

void setup_jtag_server_print(void)
{
	// Provide a callback to jtag_common.c so that messages can be printed.
	jtag_server_set_print_func(print_func);
}

void register_check_for_command(void)
{
	s_vpi_systf_data data = {
		vpiSysTask,
		0,
		"$check_for_command",
		(void *)vpi_check_for_command,
		0,
		0,
		0
	};

	vpi_register_systf(&data);
}

void register_send_result_to_server(void)
{
	s_vpi_systf_data data = {
		vpiSysTask,
		0,
		"$send_result_to_server",
		(void *)vpi_send_result_to_server,
		0,
		0,
		0
	};

	vpi_register_systf(&data);
}

void sim_reset_callback(void)
{
  // nothing to do!
}

void setup_reset_callbacks(void)
{
	// here we setup and install callbacks for
	// the setup and management of connections to
	// the simulator upon simulation start and
	// reset

	static s_vpi_time time_s = {vpiScaledRealTime, 0, 0, 0};
	static s_vpi_value value_s = {.format = vpiBinStrVal};

	static s_cb_data cb_data_s = {
		cbEndOfReset, // or start of simulation - initing socket fds etc
		(void *)sim_reset_callback,
		NULL,
		&time_s,
		&value_s,
		0,
		NULL
	};

	cb_data_s.obj = NULL;  /* trigger object */

	cb_data_s.user_data = NULL;

	// actual call to register the callback
	vpi_register_cb(&cb_data_s);
}

void sim_endofcompile_callback(void)
{

}

void setup_endofcompile_callbacks(void)
{
	// here we setup and install callbacks for
	// simulation finish

	static s_vpi_time time_s = {vpiScaledRealTime, 0, 0, 0};
	static s_vpi_value value_s = {.format = vpiBinStrVal};

	static s_cb_data cb_data_s = {
		cbEndOfCompile, // end of compile
		(void *)sim_endofcompile_callback,
		NULL,
		&time_s,
		&value_s,
		0,
		NULL
	};

	cb_data_s.obj = NULL;  /* trigger object */

	cb_data_s.user_data = NULL;

	// actual call to register the callback
	vpi_register_cb(&cb_data_s);
}

void sim_finish_callback(void)
{
	jtag_server_finish();
}

void setup_finish_callbacks(void)
{
	// here we setup and install callbacks for
	// simulation finish

	static s_vpi_time time_s = {vpiScaledRealTime, 0, 0, 0};
	static s_vpi_value value_s = {.format = vpiBinStrVal};

	static s_cb_data cb_data_s = {
		cbEndOfSimulation, // end of simulation
		(void *)sim_finish_callback,
		NULL,
		&time_s,
		&value_s,
		0,
		NULL
	};

	cb_data_s.obj = NULL;  /* trigger object */
	cb_data_s.user_data = NULL;

	// actual call to register the callback
	vpi_register_cb(&cb_data_s);
}

#ifndef VCS_VPI
// Register the new system task here
void (*vlog_startup_routines[])(void) = {
#ifdef CDS_VPI
	// this installs a callback on simulator reset - something which
	// icarus does not do, so we only do it for cadence currently
	setup_reset_callbacks,
#endif
	setup_endofcompile_callbacks,
	setup_finish_callbacks,
	register_check_for_command,
	register_send_result_to_server,
	setup_jtag_server_print,
	0  // last entry must be 0
};

// Entry point for testing development of the vpi functions
int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	return 0;
}
#endif
