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

#include <sys/select.h>

#include "utils.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char *argv[]){

	int udp_sock,
		tcp_sock,
		nread,
		nwrite,
		local_port_tcp,
		local_port_udp;

	/*int rit_port_1, rit_port_2, rit_port_3;*/

	int len = sizeof(struct sockaddr_in*);

	struct sockaddr_in to, from;

	packet buf;

	char remote_ip[40];

	uint32_t progressive_id = 0;

    fd_set rfds;
    fd_set errfds;
    struct timeval timeout;
    int retsel;
    int fdmax;

    lista to_ack;
    to_ack.next = NULL;
    to_ack.value = NULL;

    if(argc < 2){
        printf("usage: IP_RITARDATORE\n");
        exit(1);
    }

    strncpy(remote_ip, argv[1], 39);

	local_port_tcp = 59000;
	local_port_udp = 60000;

	/*rit_port_1 = 61000;*/
	/*rit_port_2 = 61001;*/
	/*rit_port_3 = 61002;*/

	udp_sock = UDP_sock(local_port_udp);

	tcp_sock = TCP_connection_recv(local_port_tcp);

	fdmax = (udp_sock > tcp_sock)? udp_sock+1 : tcp_sock+1;

	name_socket(&to, inet_addr(remote_ip), 63000);

	while(1){

        /* Watch stdin (fd 0) to see when it has input. */
        FD_ZERO(&rfds);
        FD_SET(tcp_sock, &rfds);
        FD_SET(udp_sock, &rfds);

		FD_ZERO(&errfds);
		FD_SET(tcp_sock, &errfds);

        /* Wait up to five seconds. */
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        retsel = select(fdmax, &rfds, NULL, NULL, &timeout);
        /* Don't rely on the value of tv now! */

        if (retsel == -1){
           perror("select()");
        }
		else if (retsel) {
			printf("Data is available now.\n");

			if(FD_ISSET(tcp_sock, &rfds)){

				nread = read(tcp_sock, (char*)buf.body, BODYSIZE );

				/*printf("%s\n", buf.body);*/

				if(nread == 0){
					printf("La connessione TCP e' stata chiusa\n");
					/* FAI QUALCOSA */
					exit(1);
				}

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

				if (nwrite == -1){
					printf ("sendto() failed, Err: %d \"%s\"\n",
							errno,
							strerror(errno)
							);
					exit(1);
				}

				if (nread == -1){
				 printf ("read() failed, Err: %d \"%s\"\n",
						 errno,
						 strerror(errno)
						 );
				 exit(1);
				}

				aggiungi(&sentinella, buf.id);
			}
			if(FD_ISSET(udp_sock, &rfds)){

				nread = recvfrom( udp_sock,
								  (char*)&buf,
								  MAXSIZE,
								  0,
								  (struct sockaddr*) &from,
								  (socklen_t*)&len
								);

				if (nread == -1){
					 printf ("recvfrom() failed, Err: %d \"%s\"\n",
							 errno,
							 strerror(errno)
							 );
					 exit(1);
				}
				printf("ACK\n%s\n", buf.body);
			}
		} else {
			printf("No data within five seconds.\n");
        }
	}

	return(0);
}
