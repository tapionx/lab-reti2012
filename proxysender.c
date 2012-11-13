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
	int local_port_tcp = 0;
	int local_port_udp = 0;
	int rit_port[3];
	int rit_turno = 0;

	struct sockaddr_in to, from;

	packet buf_p;
	lista  buf_l; /* SI PUO USARE SOLO BUF? */

	char remote_ip[40];

	uint32_t progressive_id = 0;

	fd_set rfds;
	fd_set errfds;
	int retsel;
	int fdmax;

	struct timeval timeout, curtime, towait;

  lista to_ack;
  to_ack.next = NULL;

	timeout.tv_sec  = TIMEOUT;
	timeout.tv_usec = MSTIMEOUT;

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

		if( tcp_sock == -1 && nlist == 0)
			exit(1);

		/* incrementiamo il turno delle porte del ritardatore */
		rit_turno = (rit_turno + 1) % 3;
		name_socket(&to, inet_addr(remote_ip), rit_port[rit_turno]);

		/* Watch stdin (fd 0) to see when it has input. */
		FD_ZERO(&rfds);
		if(tcp_sock != -1)
			FD_SET(tcp_sock, &rfds);
		FD_SET(udp_sock, &rfds);

		FD_ZERO(&errfds);
		FD_SET(tcp_sock, &errfds);

		/* azzero il buffer */
		memset(&buf_l, 0, sizeof(lista));
		memset(&buf_p, 0, sizeof(packet));

		/* Decidiamo quanto tempo aspettare prima di risvegliarci
		 * dalla select(). Se trovo pacchetti scaduti li mando di nuovo
		 */
		if(to_ack.next == NULL){
			/* METTERE NULL */
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
		printf("\r%d  ", nlist);
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

			/*mi ha svegliato un socktcp?*/
			if(FD_ISSET(tcp_sock, &rfds)){
				/*
				 * TCP ----------------------------------------------
				 */
				nread = readn(tcp_sock, (char*)buf_p.body, BODYSIZE, NULL );

				if(nread == 0){
					printf("La connessione TCP e' stata chiusa\n");
					/* FAI QUALCOSA */
					close(tcp_sock);
					tcp_sock = -1;
					/*exit(1);*/
				} else {

					/*Creo l'header del pacchetto e invio il pacchetto come UDP*/
					progressive_id++;
					buf_p.id = htonl(progressive_id);
					buf_p.tipo = 'B';

					writen( udp_sock, (char*)&buf_p, HEADERSIZE + nread, &to);

					buf_p.id = ntohl(buf_p.id);
					aggiungi(&to_ack, buf_p, nread+HEADERSIZE);
					/*stampalista(&to_ack);*/
				}
			}
			if(FD_ISSET(udp_sock, &rfds)){
				/*
				 * UDP -----------------------------------------
				 */
				nread = readn( udp_sock, (char*)&buf_p, MAXSIZE, &from);

				/* se ho ricevuto un ACK */
				if(buf_p.tipo == 'B'){
					buf_p.id = ntohl(buf_p.id);
					buf_l = rimuovi(&to_ack, buf_p.id);
				}
				/* se ho ricevuto un ICMP */
				if(buf_p.tipo == 'I'){
					buf_l = rimuovi(&to_ack, ((ICMP*)&buf_p)->idpck);
					if(buf_l.p.tipo != 'E'){
						buf_l.p.id = htonl(buf_l.p.id);

						writen( udp_sock, (char*)&buf_l.p, buf_l.size, &to);

						buf_l.p.id = ntohl(buf_l.p.id);
						aggiungi(&to_ack, buf_l.p, buf_l.size);
						fflush(stdout);
					}
				}
			}
		} else {
			/*
			 * TIMEOUT -------------------------
			 */
			buf_l = pop(&(to_ack));
			buf_l.p.id = htonl(buf_l.p.id);
			writen( udp_sock, (char*)&buf_l.p, buf_l.size, &to);

			buf_l.p.id = ntohl(buf_l.p.id);
			aggiungi(&to_ack, buf_l.p, buf_l.size);
			fflush(stdout);
    }
	}
	return(0);
}
