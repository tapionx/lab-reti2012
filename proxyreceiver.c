#include <stdio.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "utils.h"

#define BUFSIZE 50

int main(int argc, char *argv[]){

	int udp_sock, tcp_sock, nread, nwrite, len, local_port, remote_port;

	struct sockaddr_in from;

	char buf[BUFSIZE];

	char remote_ip[40];

    if(argc < 4){
        printf("usage: LOCAL_PORT REMOTE_IP REMOTE_PORT\n");
        exit(1);
    }

    local_port = atoi(argv[1]);
    strncpy(remote_ip, argv[2], 39);
    remote_port = atoi(argv[3]);

	udp_sock = UDP_sock(local_port);

	tcp_sock = TCP_connection_send(remote_ip, remote_port);

	name_socket(&from, htonl(INADDR_ANY), 0);

	len = sizeof(struct sockaddr_in);

	while( (nread = recvfrom( udp_sock,
						  buf,
						  BUFSIZE,
						  0,
						  (struct sockaddr*) &from,
						  (socklen_t*)&len
						)) > 0) {

		printf("%s -> %d\n", buf, nread);

		nwrite = write( tcp_sock,
						buf,
						nread
					   );

		printf("write(): %d byte\n", nwrite);

	}

	return(0);
}
