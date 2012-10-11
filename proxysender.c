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

	int udp_sock, tcp_sock, nread, nwrite, local_port, remote_port;

	struct sockaddr_in to;

	char buf[BUFSIZE];

	char remote_ip[40];

    if(argc < 4){
        printf("usage: LOCAL_PORT REMOTE_IP REMOTE_PORT\n");
        exit(1);
    }

    local_port = atoi(argv[1]);
    strncpy(remote_ip, argv[2], 39);
    remote_port = atoi(argv[3]);

	udp_sock = UDP_sock(0);

	tcp_sock = TCP_connection_recv(local_port);

	name_socket(&to, inet_addr(remote_ip), remote_port);

	while( (nread = read(tcp_sock, buf, BUFSIZE )) > 0){
		printf ("read(): %d byte\n%s\n", nread,buf);
		nwrite = sendto( udp_sock,
						 buf,
						 nread,
						 0,
						 (struct sockaddr*)&to,
						 (socklen_t )sizeof(struct sockaddr_in)
					   );
		printf("sendto(): %d byte\n", nwrite);
	}

    printf("File trasferito correttamente\n");

	return(0);
}
