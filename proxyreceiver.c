/*
 * 	proxyreceiver.c : usando datagram UDP simula una connessione TCP
 * 	attraverso un programma ritardatore che scarta i pacchetti in modo
 * 	casuale, simulando una rete reale
 */

/* file di header */
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
#include <sys/select.h>

/* file di header del progetto */
#include "utils.h"

int main(int argc, char *argv[]){

	/* variabile globale che contiene il numero di elementi nella lista
	 * viene tenuta aggiornata dalle funzioni di manipolazione delle
	 * liste */
	extern int nlist;

	int arrivata_terminazione = 0;

	int udp_sock, tcp_sock, nread, local_port, remote_port;

	struct sockaddr_in from, to;

	lista  buf_l;
	packet buf;
	char remote_ip[40];
	char ritardatore_ip[40];

	/* una lista ordinata che contiene i pacchetti da spedire al
	   receiver */
	lista to_send;

	uint32_t id_to_wait = 1;

	fd_set rfds;
	struct timeval tv;
	int retval;

	/* recupero parametri */
	if(argc > 1)
		strcpy(remote_ip, argv[1]);
	else
		strcpy(remote_ip, "127.0.0.1");

	if(argc > 2)
		strcpy(ritardatore_ip, argv[2]);
	else
		strcpy(ritardatore_ip, "127.0.0.1");

	if(argc > 3)
		local_port = atoi(argv[3]);
	else
		local_port = 63000;

	if(argc > 4)
		remote_port = atoi(argv[4]);
	else
		remote_port = 64000;

	printf("receiverIP: %s ritardatoreIP %s proxyreceiverPORT: %d receiverPORT: %d\n",
    	   remote_ip,
    	   ritardatore_ip,
		   local_port,
		   remote_port
		  );

	/* inizializzo i socket */

	udp_sock = UDP_sock(local_port);

	tcp_sock = TCP_connection_send(remote_ip, remote_port);

	name_socket(&from, htonl(INADDR_ANY), 0);

	while(1) {

		if(arrivata_terminazione && nlist == 0){
			FD_ZERO(&rfds);
            FD_SET(udp_sock, &rfds);
            tv.tv_sec = 5;
			tv.tv_usec = 0;
			retval = select(udp_sock+1, &rfds, NULL, NULL, &tv);
			if (retval == -1)
               perror("select()");
			else if (retval == 0){
				close(udp_sock);
				close(tcp_sock);
				printf("\nTimeout scaduto: terminazione.\n");
				exit(EXIT_SUCCESS);
			}
		}

		printf("\r%d  ", nlist);
		fflush(stdout);

		nread = readn(udp_sock, (char*)&buf, MAXSIZE, &from);

		if(buf.tipo == 'B'){

			if(ntohl(buf.id) == 0){
				if(arrivata_terminazione == 0){
					arrivata_terminazione = 1;
					printf("\nArrivata terminazione\n");
				} else {
					printf("\nArrivato ACK, terminazione.\n");
					close(udp_sock);
					close(tcp_sock);
					exit(EXIT_SUCCESS);
				}
			}

			/* Imposto la destinazione dell'ACK
			 * inviandolo sullo stesso canale dal quale è arrivato
			 * l'udp perchè probabilmente non è in BURST
			 */
			name_socket(&to, inet_addr(ritardatore_ip), ntohs(from.sin_port));

			/* invio ACK */
			writen(udp_sock, (char*)&buf, HEADERSIZE+1, &to);

		/*---------------------------------------*/

			if(ntohl(buf.id) >= id_to_wait){
				buf.id = ntohl(buf.id);
				aggiungi_in_ordine(&to_send, buf, nread-HEADERSIZE);
				printf("\r%d  ", nlist);
				fflush(stdout);
			}

			while( to_send.next != NULL && to_send.next->p.id == id_to_wait){

				memset(&buf_l, 0, sizeof(lista));
				memset(&buf_l.p, 0, sizeof(packet));
				buf_l = pop(&to_send);
				writen( tcp_sock,
				    	buf_l.p.body,
						buf_l.size,
						NULL
					   );
				id_to_wait++;

				printf("\r%d  ", nlist);
				fflush(stdout);
			}
		}
		if(buf.tipo == 'I') {
			buf.id = ((ICMP*)&buf)->idpck;
			buf.tipo = 'B';
			/* buf.body = 0; */
			writen(udp_sock, (char*)&buf, HEADERSIZE+1, &to);
			/* printf("ICMP! %d\n", ntohl(buf.id) ); */
		}
	fflush(stdout);
	}

	return(0);
}
