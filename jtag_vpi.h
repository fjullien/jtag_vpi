#ifndef __JTAG_VPI_H__
#define __JTAG_VPI_H__

#define XFERT_MAX_SIZE		512

#define DONE			0
#define IN_PROGRESS		1

#define CMD_RESET		0
#define CMD_TMS_SEQ		1
#define CMD_SCAN_CHAIN		2
#define CMD_SCAN_CHAIN_FLIP_TMS	3
#define CMD_STOP_SIMU		4

#define CHECK_CMD		0
#define TAP_RESET		1
#define GOTO_IDLE		2
#define DO_TMS_SEQ		3
#define SCAN_CHAIN		4
#define FINISHED		5

struct jtag_cmd {
	int cmd;
	unsigned char buffer_out[XFERT_MAX_SIZE];
	unsigned char buffer_in[XFERT_MAX_SIZE];
	int length;
	int nb_bits;
};

class VerilatorJTAG {
public:
	VerilatorJTAG(uint64_t period);
	~VerilatorJTAG();

	int doJTAG(uint64_t t, int *tms, int *tdi, int *tck, int tdo);
	int init_jtag_server(int port);

private:
	int check_for_command(struct jtag_cmd *vpi);
	int send_result_to_server(struct jtag_cmd *packet);

	int gen_clk(uint64_t t, int nb_period, int *tck, int tdo, int *captured_tdo, int restart);
	int reset_tap(uint64_t t, int *tms, int *tck);
	int goto_run_test_idle(uint64_t t, int *tms, int *tck);
	int do_tms_seq(uint64_t t, int length, int nb_bits, unsigned char *buffer, int *tms, int *tck);
	int do_scan_chain(uint64_t t, int length, int nb_bits, unsigned char *buffer_out,
			  unsigned char *buffer_in, int *tms, int *tck, int *tdi, int tdo, int flip_tms);

	int listenfd;
	int connfd;
	struct jtag_cmd packet;

	int jtag_state;
	int cmd_in_progress;
	int tms_flip;
	uint64_t tck_period;
};

#endif
