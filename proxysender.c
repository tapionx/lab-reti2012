/*
 * 	proxysender.c : Forward a TCP stream through UDP datagrams
 * 					without packet loss
 */

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

#define MAXSIZE 65536

#define HEADERSIZE 5

#define BODYSIZE (MAXSIZE) - (HEADERSIZE)

typedef struct packet{
	uint32_t id;
	char tipo;
	char body[BODYSIZE];
} packet;

int main(int argc, char *argv[]){

	int udp_sock, tcp_sock, nread, nwrite, local_port;

	int rit_port_1, rit_port_2, rit_port_3;

	struct sockaddr_in to;

	packet buf;

	char remote_ip[40];

	uint32_t progressive_id = 0;

    if(argc < 2){
        printf("usage: IP_RITARDATORE\n");
        exit(1);
    }

    strncpy(remote_ip, argv[1], 39);

	local_port = 59000;

	rit_port_1 = 61000;
	rit_port_2 = 61001;
	rit_port_3 = 61002;

	udp_sock = UDP_sock(0);

	tcp_sock = TCP_connection_recv(local_port);

	name_socket(&to, inet_addr(remote_ip), 63000);

	while( (nread = read(tcp_sock, (char*)buf.body, BODYSIZE )) > 0){
		printf ("read(): %d byte\n", nread);
		/*printf("%s\n", buf.body);*/


		progressive_id++;
		buf.id = progressive_id;
		buf.tipo = 'B';

		printf("%d\n", progressive_id);

		nwrite = sendto( udp_sock,
						 (char*)&buf,
						 HEADERSIZE + nread,
						 0,
						 (struct sockaddr*)&to,
						 (socklen_t )sizeof(struct sockaddr_in)
					   );
		printf("sendto(): %d byte\n\n", nwrite);
	}

    printf("File trasferito correttamente\n");

	return(0);
}
