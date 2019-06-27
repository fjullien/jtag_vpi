#ifndef __JTAG_COMMON_H__
#define __JTAG_COMMON_H__

#define	XFERT_MAX_SIZE	512

struct jtag_cmd {
	uint32_t cmd;
	unsigned char buffer_out[XFERT_MAX_SIZE];
	unsigned char buffer_in[XFERT_MAX_SIZE];
	uint32_t length;
	uint32_t nb_bits;
};

int init_jtag_server(int port);
int check_for_command(struct jtag_cmd *vpi);
int send_result_to_server(struct jtag_cmd *vpi);
void jtag_finish(void);

#endif
