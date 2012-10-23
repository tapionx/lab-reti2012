/*
 * proxyreceiver.c
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
#include <sys/time.h>

#include "utils.h"


int main(int argc, char *argv[]){

	int udp_sock, tcp_sock, nread, nwrite, len, local_port, remote_port;

	struct sockaddr_in from, to;

	packet buf;
	char remote_ip[40];

    if(argc < 2){
        printf("usage: RECEIVER_IP\n");
        exit(1);
    }

	strncpy(remote_ip, argv[1], 39);

    local_port = 63000;
    remote_port = 64000;

	udp_sock = UDP_sock(local_port);

	tcp_sock = TCP_connection_send(remote_ip, remote_port);

	name_socket(&from, htonl(INADDR_ANY), 0);

	name_socket(&to, inet_addr("127.0.0.1"), 60000);

	len = sizeof(struct sockaddr_in);

	while( (nread = recvfrom( udp_sock,
							  (char*)&buf,
							  MAXSIZE,
							  0,
							  (struct sockaddr*) &from,
							  (socklen_t*)&len
							)) > 0) {

		printf("recvfrom(): %d byte\n", nread);

		printf("%d %c\n", buf.id, buf.tipo);

		nwrite = write( tcp_sock,
						buf.body,
						nread-HEADERSIZE
					   );

		printf("write(): %d byte\n\n", nwrite);

		if (nwrite == -1){
			printf ("write() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        	exit(1);
   		}

		if(nwrite < nread-HEADERSIZE){
			printf("TODO: sendall()\n");
			exit(1);
		}

   		nwrite = sendto( udp_sock,
						 (char*)&buf,
						 nread,
						 0,
						 (struct sockaddr*)&to,
						 (socklen_t )sizeof(struct sockaddr_in)
					   );

		if (nwrite == -1){
			printf ("write() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        	exit(1);
   		}

		if(nwrite < nread){
			printf("TODO: sendall()\n");
			exit(1);
		}

	}

	if (nread == -1){
		 printf ("recvfrom() failed, Err: %d \"%s\"\n",errno,strerror(errno));
         exit(1);
	} else {
		printf ("Trasferimento completato\n");
	}

	return(0);
}
