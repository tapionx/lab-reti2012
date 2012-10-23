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
	lista temp;

	char remote_ip[40];

	uint32_t progressive_id = 0;

    fd_set rfds;
    fd_set errfds;
    struct timeval timeout;
    int retsel;
    int fdmax;

	struct timeval curtime;
	time_t s_left;

    lista to_ack;
    to_ack.next = NULL;


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

		/* Decidiamo quanto tempo aspettare prima di risvegliarci
		 * dalla select(). Se trovo pacchetti scaduti li mando di nuovo
		 */

		do{
			memset(&temp, 0, sizeof(packet));
			if(to_ack.next == NULL){
				s_left = 999;  /* VA BENE COSI? */
			} else {
				if(gettimeofday(&(curtime), NULL)){
					printf ("gettimeofday() failed, Err: %d \"%s\"\n",
							errno,
							strerror(errno)
						   );
					exit(1);
				}

				if( (s_left = (TIMEOUT - ((curtime.tv_sec) - (to_ack.next->sentime.tv_sec)))) <= 0){
					temp = pop(&(to_ack));
					nwrite = sendto( udp_sock,
													 (char*)&temp,
													 MAXSIZE,
													 0,
													 (struct sockaddr*)&to,
													 (socklen_t )sizeof(struct sockaddr_in)
												 );
					aggiungi(&to_ack, temp.p);
					printf("rispedito %s\n", temp.p.body);
					stampalista(&to_ack);
					fflush(stdout);
				}
			}
		} while(s_left <= 0);

		printf("%d\n", (int)s_left);

		/* Wait up to five seconds. */
        timeout.tv_sec = s_left;
        timeout.tv_usec = 0;
				printf("--SELECT()--\n");
        retsel = select(fdmax, &rfds, NULL, NULL, &timeout);
        /* Don't rely on the value of tv now! */

        if (retsel == -1){
           perror("select()");
        }
		else if (retsel) {
			memset(&buf, 0, sizeof(packet));
			if(FD_ISSET(tcp_sock, &rfds)){
				printf("--> TCP\n");
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

				nwrite = sendto( udp_sock,
								 (char*)&buf,
								 HEADERSIZE + nread,
								 0,
								 (struct sockaddr*)&to,
								 (socklen_t )sizeof(struct sockaddr_in)
							   );
				/* printf("sendto(): %d byte\n\n", nwrite); */

				if (nwrite == -1){
					printf ("sendto() failed, Err: %d \"%s\"\n",
							errno,
							strerror(errno)
							);
					exit(1);
				}

				if(nwrite < HEADERSIZE+nread){
					printf("TODO: sendall()\n");
					exit(1);
				}

				if (nread == -1){
				 printf ("read() failed, Err: %d \"%s\"\n",
						 errno,
						 strerror(errno)
						 );
				 exit(1);
				}

				aggiungi(&to_ack, buf);
				stampalista(&to_ack);
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
				/*printf("%d byte: %d %c %s\n", nread, buf.id, buf.tipo, buf.body);*/
				printf("--> ACK %d\n", buf.id);
				rimuovi(&to_ack, buf.id);
				stampalista(&to_ack);
			}
		} else {
			/*printf("--> Timeout\n");*/
			fflush(stdout);
        }
	}

	return(0);
}
