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

	extern int nlist;

	int udp_sock = 0;
	int tcp_sock = 0;
	int nread = 0;
	int nwrite = 0;
	int local_port_tcp = 0;
	int local_port_udp = 0;
	int rit_port[3];
	int rit_turno = 0;

	int len = sizeof(struct sockaddr_in*);

	struct sockaddr_in to, from;

	packet buf;
	packet temp; /* SI PUO USARE SOLO BUF? */

	char remote_ip[40];

	uint32_t progressive_id = 0;

	fd_set rfds;
	fd_set errfds;
	int retsel;
	int fdmax;

	struct timeval timeout, curtime, towait;

  lista to_ack;
  to_ack.next = NULL;

	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;

	/* recupero parametri */
	if(argc > 1)
		strcpy(remote_ip, argv[1]);
	else
		strcpy(remote_ip,	"127.0.0.1");

	if(argc > 2)
		local_port_tcp = atoi(argv[2]);
	else
		local_port_tcp = 59000;

	if(argc > 3)
		local_port_udp = atoi(argv[3]);
	else
		local_port_udp = 60000;

	if(argc > 4)
		rit_port[0] = atoi(argv[4]);
	else
		rit_port[0] = 61000;

	if(argc > 5)
		rit_port[1] = atoi(argv[5]);
	else
		rit_port[1] = 61001;

	if(argc > 6)
		rit_port[2] = atoi(argv[6]);
	else
		rit_port[2] = 61002;

	printf("remoteIP: %s localTCP: %d localUDP: %d rit1: %d rit2: %d rit3: %d\n",
				 remote_ip,
				 local_port_tcp,
				 local_port_udp,
				 rit_port[0],
				 rit_port[1],
				 rit_port[2]
				);
	/********************************************************/

	udp_sock = UDP_sock(local_port_udp);

	tcp_sock = TCP_connection_recv(local_port_tcp);

	fdmax = (udp_sock > tcp_sock)? udp_sock+1 : tcp_sock+1;

	while(1){

		/* incrementiamo il turno delle porte del ritardatore */
		rit_turno = (rit_turno + 1) % 3;
		name_socket(&to, inet_addr(remote_ip), rit_port[rit_turno]);

		/* Watch stdin (fd 0) to see when it has input. */
		FD_ZERO(&rfds);
		FD_SET(tcp_sock, &rfds);
		FD_SET(udp_sock, &rfds);

		FD_ZERO(&errfds);
		FD_SET(tcp_sock, &errfds);

		/* azzero il buffer */
		memset(&temp, 0, sizeof(packet));

		/* Decidiamo quanto tempo aspettare prima di risvegliarci
		 * dalla select(). Se trovo pacchetti scaduti li mando di nuovo
		 */
		if(to_ack.next == NULL){
			towait.tv_sec = 999;
			towait.tv_usec = 99999999;
		} else {
			if(gettimeofday(&(curtime), NULL)){
				printf("gettimeofday() failed, Err: %d \"%s\"\n",
							 errno,
							 strerror(errno)
							);
				exit(1);
			}
			timeval_subtract(&curtime, &curtime, &(to_ack.next->sentime));
			if(timeval_subtract(&towait, &timeout, &curtime)){
				towait.tv_usec = 0;
				towait.tv_sec = 0;
			}
		}
		/* Wait time left of the pck */
		printf("\r%d", nlist);
		fflush(stdout);
		/*
		 * SELECT -------------------------------------------------
		 */
		retsel = select(fdmax, &rfds, NULL, NULL, &towait);
		/* Don't rely on the value of tv now! */

		if (retsel == -1){
			 perror("select()");
			 exit(1);
		}
		else if (retsel) {
			memset(&buf, 0, sizeof(packet));
			/*mi ha svegliato un socktcp?*/
			if(FD_ISSET(tcp_sock, &rfds)){
				/*
				 * TCP ----------------------------------------------
				 */
				nread = read(tcp_sock, (char*)buf.body, BODYSIZE );

				/*printf("%s\n", buf.body);*/

				if(nread == 0){
					printf("La connessione TCP e' stata chiusa\n");
					/* FAI QUALCOSA */
					exit(1);
				}
				/*Creo l'header del pacchetto e invio il pacchetto come UDP*/
				progressive_id++;
				buf.id = htonl(progressive_id);
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
				buf.id = ntohl(buf.id);
				aggiungi(&to_ack, buf);
				/*stampalista(&to_ack);*/
			}
			if(FD_ISSET(udp_sock, &rfds)){
				/*
				 * UDP -----------------------------------------
				 */
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
				/* se ho ricevuto un ACK */
				if(buf.tipo == 'B'){
					buf.id = ntohl(buf.id);
					temp = rimuovi(&to_ack, buf.id);
					/*stampalista(&to_ack);*/
				}
				/* se ho ricevuto un ICMP */
				if(buf.tipo == 'I'){
					temp = rimuovi(&to_ack, ((ICMP*)&buf)->idpck);
					if(temp.tipo != 'E'){
						temp.id = htonl(temp.id);
						nwrite = sendto( udp_sock,
														 (char*)&temp,
														 MAXSIZE,
														 0,
														 (struct sockaddr*)&to,
														 (socklen_t )sizeof(struct sockaddr_in)
													 );
						if (nwrite == -1){
						 printf ("read() failed, Err: %d \"%s\"\n",
								 errno,
								 strerror(errno)
								 );
						 exit(1);
						}
						temp.id = ntohl(temp.id);
						aggiungi(&to_ack, temp);
						/*stampalista(&to_ack);*/
						fflush(stdout);
					}
				}
			}
		} else {
			/*
			 * TIMEOUT -------------------------
			 */
			temp = pop(&(to_ack));
			temp.id = htonl(temp.id);
			nwrite = sendto( udp_sock,
											 (char*)&temp,
											 MAXSIZE,
											 0,
											 (struct sockaddr*)&to,
											 (socklen_t )sizeof(struct sockaddr_in)
										 );
			if (nwrite == -1){
			 printf ("read() failed, Err: %d \"%s\"\n",
					 errno,
					 strerror(errno)
					 );
			 exit(1);
			}
			temp.id = ntohl(temp.id);
			aggiungi(&to_ack, temp);
			/*stampalista(&to_ack);*/
			fflush(stdout);
    }
	}
	return(0);
}
