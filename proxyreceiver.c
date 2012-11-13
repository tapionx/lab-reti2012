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

	int udp_sock, tcp_sock, nread, local_port, remote_port;

	int quanti_in_attesa = 0;

	struct sockaddr_in from, to;

	lista  buf_l;
	packet buf;
	char remote_ip[40];

	/* una lista ordinata che contiene i pacchetti da spedire al
	   receiver */
	lista to_send;

	uint32_t id_to_wait = 1;

	/* recupero parametri */
	if(argc > 1)
		strcpy(remote_ip, argv[1]);
	else
		strcpy(remote_ip, "127.0.0.1");

	if(argc > 2)
		local_port = atoi(argv[2]);
	else
		local_port = 63000;

	if(argc > 3)
		remote_port = atoi(argv[3]);
	else
		remote_port = 64000;

	printf("remoteIP: %s localPORT: %d remotePORT: %d\n",
    	   remote_ip,
		   local_port,
		   remote_port
		  );

	/* inizializzo i socket */

	udp_sock = UDP_sock(local_port);

	tcp_sock = TCP_connection_send(remote_ip, remote_port);

	name_socket(&from, htonl(INADDR_ANY), 0);

	while(1) {

		nread = readn(udp_sock, (char*)&buf, MAXSIZE, &from);

		if(buf.tipo == 'B'){

			/* Imposto la destinazione dell'ACK
			 * inviandolo sullo stesso canale dal quale è arrivato
			 * l'udp perchè probabilmente non è in BURST
			 */
			name_socket(&to, from.sin_addr.s_addr, ntohs(from.sin_port));

			/* invio ACK */
			writen(udp_sock, (char*)&buf, HEADERSIZE+1, &to);

		/*---------------------------------------*/

			if(ntohl(buf.id) >= id_to_wait){
				buf.id = ntohl(buf.id);
				aggiungi_in_ordine(&to_send, buf, nread-HEADERSIZE);
				quanti_in_attesa++;
				printf("\r%d  ", quanti_in_attesa);
				fflush(stdout);
			}

			while( to_send.next != NULL && to_send.next->p.id == id_to_wait){

				memset(&buf_l, 0, sizeof(lista));
				memset(&buf_l.p, 0, sizeof(packet));
				buf_l = pop(&to_send);
				quanti_in_attesa--;
				writen( tcp_sock,
				    	buf_l.p.body,
						buf_l.size,
						NULL
					   );
				id_to_wait++;

				printf("\r%d  ", quanti_in_attesa);
				fflush(stdout);
			}
		}
		if(buf.tipo == 'I') {
			printf("ICMP! %d\n", ntohl(buf.id) );
		}

	}

	return(0);
}
