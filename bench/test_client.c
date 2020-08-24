/*
 * Test client for jtag_vpi.
 *
 * Copyright (C) 2012 Franck Jullien, <elec4fun@gmail.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>

/* Configuration */
#define SERVER_ADDRESS			"127.0.0.1"
#define TEST_BUFFER_LEN			16
#define SERVER_PORT			5555

/* TAP defines */
#define OR1K_TAP_INST_DEBUG		0x8

/* ADV_DEBUG defines */
#define DBG_MODULE_SELECT_REG_SIZE	2
#define DBG_MAX_MODULES			4
#define ADBG_CRC_POLY			0xedb88320
#define DBG_WB_CMD_BWRITE32		0x3
#define DBG_WB_CMD_BREAD32		0x7

#define DC_WISHBONE			0
#define DC_CPU0				1
#define DC_CPU1				2
#define DC_JSP				3

/* JTAG VPI defines */
#define	XFERT_MAX_SIZE			512

#define CMD_RESET			0
#define CMD_TMS_SEQ			1
#define CMD_SCAN_CHAIN			2
#define CMD_SCAN_CHAIN_FLIP_TMS		3
#define CMD_STOP_SIMU			4

/* Test client */
#define NO_TAP_SHIFT			0
#define TAP_SHIFT			1
#define STATUS_BYTES			1
#define CRC_LEN				4

int server_port = SERVER_PORT;

int sockfd = 0;
struct sockaddr_in serv_addr;

struct vpi_cmd {
	int cmd;
	unsigned char buffer_out[XFERT_MAX_SIZE];
	unsigned char buffer_in[XFERT_MAX_SIZE];
	int length;
	int nb_bits;
};

static int jtag_vpi_send_cmd(struct vpi_cmd * vpi)
{
	return write(sockfd, vpi, sizeof(struct vpi_cmd));
}

static int jtag_vpi_receive_cmd(struct vpi_cmd * vpi)
{
	return read(sockfd, vpi, sizeof(struct vpi_cmd));
}

/**
 * jtag_vpi_stop_simu - Stop the simulation
 */
static void jtag_vpi_stop_simu(void)
{
	struct vpi_cmd vpi;

	vpi.cmd = CMD_STOP_SIMU;
	vpi.length = 0;
	jtag_vpi_send_cmd(&vpi);
}

/**
 * jtag_vpi_reset - ask to reset the JTAG device
 */
static void jtag_vpi_reset(void)
{
	struct vpi_cmd vpi;

	vpi.cmd = CMD_RESET;
	vpi.length = 0;
	jtag_vpi_send_cmd(&vpi);
}

/**
 * jtag_vpi_tms_seq - ask a TMS sequence transition to JTAG
 * @bits: TMS bits to be written (bit0, bit1 .. bitN)
 * @nb_bits: number of TMS bits (between 1 and 8)
 *
 * Write a serie of TMS transitions, where each transition consists in :
 *  - writing out TCK=0, TMS=<new_state>, TDI=<???>
 *  - writing out TCK=1, TMS=<new_state>, TDI=<???> which triggers the transition
 * The function ensures that at the end of the sequence, the clock (TCK) is put
 * low.
 */
static void jtag_vpi_tms_seq(const uint8_t *bits, int nb_bits)
{
	struct vpi_cmd vpi;
	int nb_bytes;

	nb_bytes = (nb_bits / 8) + !!(nb_bits % 8);

	vpi.cmd = CMD_TMS_SEQ;
	memcpy(vpi.buffer_out, bits, nb_bytes);
	vpi.length = nb_bytes;
	vpi.nb_bits = nb_bits;
	printf("jtag_vpi_tms_seq  : (bits=%02x..., nb_bits=%d)\n", bits[0], nb_bits);
	jtag_vpi_send_cmd(&vpi);
}

/**
 * jtag_vpi_clock_tms - clock a TMS transition
 * @tms: the TMS to be sent
 *
 * Triggers a TMS transition (ie. one JTAG TAP state move).
 */
static void jtag_vpi_clock_tms(int tms)
{
	const uint8_t tms_0 = 0;
	const uint8_t tms_1 = 1;

	jtag_vpi_tms_seq(tms ? &tms_1 : &tms_0, 1);
}

static void jtag_vpi_queue_tdi_xfer(uint8_t *bits, int nb_bits, int tap_shift)
{
	struct vpi_cmd vpi;
	int nb_bytes;

	nb_bytes = (nb_bits / 8) + !!(nb_bits % 8);

	vpi.cmd = tap_shift ? CMD_SCAN_CHAIN_FLIP_TMS : CMD_SCAN_CHAIN;

	if (bits)
		memcpy(vpi.buffer_out, bits, nb_bytes);
	else
		memset(vpi.buffer_out, 0xff, nb_bytes);

	vpi.length = nb_bytes;
	vpi.nb_bits = nb_bits;

	printf("jtag_vpi_queue_tdi: (bits=%02x..., nb_bits=%d)\n", bits[0], nb_bits);
	jtag_vpi_send_cmd(&vpi);
	jtag_vpi_receive_cmd(&vpi);

	if (bits)
		memcpy(bits, vpi.buffer_in, nb_bytes);
}

/* Put a value in the IR register */
int jtag_ir_scan(uint8_t *bits, int nb_bits)
{
	uint8_t moves;
	/* Move to Shift-IR */
	moves = 0x3;
	jtag_vpi_tms_seq(&moves, 4);
	/* Enable the debug unit and move to Exit1-IR */
	jtag_vpi_queue_tdi_xfer(bits, nb_bits, TAP_SHIFT);
	/* Move to IDLE */
	moves = 0x1;
	jtag_vpi_tms_seq(&moves, 2);
}

/* Put a value in the DR register */
int jtag_dr_scan(uint8_t *bits, int nb_bits)
{
	uint8_t moves;
	/* Move to Shift-DR */
	moves = 0x1;
	jtag_vpi_tms_seq(&moves, 3);
	/* Enable the debug unit and move to Exit1-DR */
	jtag_vpi_queue_tdi_xfer(bits, nb_bits, TAP_SHIFT);
	/* Move to IDLE */
	moves = 0x1;
	jtag_vpi_tms_seq(&moves, 2);
}

static int adbg_select_module(int chain)
{
	/* MSB of the data out must be set to 1, indicating a module
	 * select command
	 */
	uint8_t data = chain | (1 << DBG_MODULE_SELECT_REG_SIZE);

	jtag_dr_scan(&data, (DBG_MODULE_SELECT_REG_SIZE + 1));

	return 0;
}

static int adbg_burst_command(uint32_t opcode,
			      uint32_t address, int length_words)
{
	uint32_t data[2];

	/* Set up the data */
	data[0] = length_words | (address << 16);
	/* MSB must be 0 to access modules */
	data[1] = ((address >> 16) | ((opcode & 0xf) << 16)) & ~(0x1 << 20);

	jtag_dr_scan((uint8_t *)&data, 53);

	return 0;
}

static uint32_t adbg_compute_crc(uint32_t crc, uint32_t data_in,
				 int length_bits)
{
	int i;

	for (i = 0; i < length_bits; i++) {

		uint32_t d, c;

		d = ((data_in >> i) & 0x1) ? 0xffffffff : 0;
		c = (crc & 0x1) ? 0xffffffff : 0;
		crc = crc >> 1;
		crc = crc ^ ((d ^ c) & ADBG_CRC_POLY);
	}

	return crc;
}

static int find_status_bit(void *_buf, int len)
{
	int i = 0;
	int count = 0;
	int ret = -1;
	uint8_t *buf = _buf;

	while (!(buf[i] & (1 << count++)) && (i < len)) {
		if (count == 8) {
			count = 0;
			i++;
		}
	}

	if (i < len)
		ret = (i * 8) + count;

	return ret;
}

void buffer_shr(void *_buf, unsigned buf_len, unsigned count)
{
	unsigned i;
	unsigned char *buf = _buf;
	unsigned bytes_to_remove;
	unsigned shift;

	bytes_to_remove = count / 8;
	shift = count - (bytes_to_remove * 8);

	for (i = 0; i < (buf_len - 1); i++)
		buf[i] = (buf[i] >> shift) | ((buf[i+1] << (8 - shift)) & 0xff);

	buf[(buf_len - 1)] = buf[(buf_len - 1)] >> shift;

	if (bytes_to_remove) {
		memmove(buf, &buf[bytes_to_remove], buf_len - bytes_to_remove);
		memset(&buf[buf_len - bytes_to_remove], 0, bytes_to_remove);
	}
}

/* Set up and execute a burst write to a contiguous set of addresses */
static int adbg_wb_burst_write(const void *data, int word_count, unsigned long start_address)
{
	uint8_t moves;
	uint8_t bits;
	uint8_t *out_buffer = malloc((word_count + 4) * 4);

	adbg_burst_command(DBG_WB_CMD_BWRITE32, start_address, word_count);

	/* Move to Shift-IR */
	moves = 0x1;
	jtag_vpi_tms_seq(&moves, 3);

	/* Write a start bit so it knows when to start counting */
	bits = 1;
	jtag_vpi_queue_tdi_xfer(&bits, 1, NO_TAP_SHIFT);

	int i;
	uint32_t datawords;
	uint32_t crc_calc = 0xffffffff;
	for (i = 0; i < word_count; i++) {
		datawords = ((const uint32_t *)data)[i];
		crc_calc = adbg_compute_crc(crc_calc, datawords, 32);
	}

	memcpy(out_buffer, data, (word_count * 4));
	memcpy(&out_buffer[(word_count * 4)], &crc_calc, 4);

	jtag_vpi_queue_tdi_xfer(out_buffer, (word_count + 4) * 32, TAP_SHIFT);

	/* Move to IDLE */
	moves = 0x1;
	jtag_vpi_tms_seq(&moves, 2);

	return 0;
}

static int adbg_wb_burst_read(int word_count, uint32_t start_address, void *data)
{
	uint8_t *in_buffer = malloc((word_count * 4) + CRC_LEN + STATUS_BYTES);
	int nb_bits = ((word_count * 4) + CRC_LEN + STATUS_BYTES) * 8;
	int i;

	adbg_burst_command(DBG_WB_CMD_BREAD32, start_address, word_count);
	jtag_dr_scan(in_buffer, nb_bits);

	/* Look for the start bit in the first (STATUS_BYTES * 8) bits */
	int shift = find_status_bit(in_buffer, STATUS_BYTES);

	buffer_shr(in_buffer, (word_count * 4) + CRC_LEN + STATUS_BYTES, shift);

	uint32_t crc_read;
	memcpy(data, in_buffer, word_count * 4);
	memcpy(&crc_read, &in_buffer[word_count * 4], 4);

	uint32_t crc_calc = 0xffffffff;
	for (i = 0; i < (word_count * 4); i++)
		crc_calc = adbg_compute_crc(crc_calc, ((uint8_t *)data)[i], 8);

	if (crc_calc != crc_read) {
		printf("CRC ERROR! Computed 0x%08x, read CRC 0x%08x\n", crc_calc, crc_read);
	} else
		printf("CRC OK!\n");

	free(in_buffer);

	return 0;
}

static int jtag_vpi_init(void)
{
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Error : Could not create socket \n");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(server_port);

	if (inet_pton(AF_INET, SERVER_ADDRESS, &serv_addr.sin_addr) <= 0) {
		printf("\n inet_pton error occured\n");
		return -1;
	}

	printf("Connection to %s:%u ->", SERVER_ADDRESS, server_port);
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf(" %s\n", strerror(errno));
		return -1;
	}

	printf("succeed\n");

	return 0;
}

int main(int argc, char *argv[])
{
	uint8_t bits;
	uint32_t out_buffer[TEST_BUFFER_LEN];
	uint32_t in_buffer[TEST_BUFFER_LEN];
	int i;
	int error = 0;

	for (i= 0; i < TEST_BUFFER_LEN; i++)
		out_buffer[i] = rand();

	if (jtag_vpi_init())
		return -1;

	jtag_vpi_reset();

	/* Enable the debug unit */
	bits = OR1K_TAP_INST_DEBUG;
	jtag_ir_scan(&bits, 4);

	/* Select the wishbone module */
	adbg_select_module(DC_WISHBONE);
	/* Write the buffer at address 0 */
	adbg_wb_burst_write(out_buffer, TEST_BUFFER_LEN, 0);

	/* Set the read buffer to 0 */
	memset(in_buffer, 0, sizeof(in_buffer));
	/* Read the buffer at address 0 */
	adbg_wb_burst_read(TEST_BUFFER_LEN, 0, in_buffer);

	for (i = 0; i < TEST_BUFFER_LEN; i++)
		if (in_buffer[i] != out_buffer[i]) {
			printf("Error ! -> in_buffer[%d]=0x%08x,"
			       " out_buffer[%d]=0x%08x\n", i, in_buffer[i],
			       i, out_buffer[i]);
			error = 1;
		}

	if (!error)
		printf("Test succeed !\n");

	/* Stop the simulation */
	jtag_vpi_stop_simu();
	return 0;
}
