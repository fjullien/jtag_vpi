#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include "jtag_common.h"

#define RSP_SERVER_PORT	5555

int listenfd = 0;
int connfd = 0;

int init_jtag_server(int port)
{
	struct sockaddr_in serv_addr;
	int flags;

	printf("Listening on port %d\n", port);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(listenfd, 10);

	printf("Waiting for client connection...");
	connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
	printf("ok\n");

	flags = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);

	return 0;
}

// See if there's anything on the FIFO for us

int check_for_command(struct jtag_cmd *vpi) {
	int nb;
	// Get the command from TCP server
	if(!connfd)
	  init_jtag_server(RSP_SERVER_PORT);
	nb = read(connfd, vpi, sizeof(struct jtag_cmd));
	if (((nb < 0) && (errno == EAGAIN)) || (nb == 0)) {
		// Nothing in the fifo this time, let's return
		return 1;
	} else {
		if (nb < 0) {
			// some sort of error
			perror("check_for_command");
			exit(1);
		}
	}
	return 0;
}

int send_result_to_server(struct jtag_cmd *vpi) {
	ssize_t n;
	n = write(connfd, vpi, sizeof(struct jtag_cmd));
	if (n < (ssize_t)sizeof(struct jtag_cmd))
	  return -1;
	return 0;
}

void jtag_finish(void) {
  	if(connfd)
		printf("Closing RSP server\n");
	close(connfd);
	close(listenfd);
}
