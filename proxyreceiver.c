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

	len = sizeof(struct sockaddr_in);

	while( (nread = recvfrom( udp_sock,
							  (char*)&buf,
							  MAXSIZE,
							  0,
							  (struct sockaddr*) &from,
							  (socklen_t*)&len
							)) > 0) {

		/* CONTROLLARE ERRORI! */
		if(nread == -1)
			printf("ERRORE\n");

		if(buf.tipo == 'B'){

			/* Imposto la destinazione dell'ACK
			 * inviandolo sullo stesso canale dal quale è arrivato
			 * l'udp perchè probabilmente non è in BURST
			 */

			name_socket(&to, from.sin_addr.s_addr, ntohs(from.sin_port));

			/* invio ACK */
			nwrite = sendto( udp_sock,
							 (char*)&buf,
							 nread,
							 0,
							 (struct sockaddr*)&to,
							 (socklen_t )sizeof(struct sockaddr_in)
						   );

			if (nwrite == -1){
				printf ("sendto() failed, Err: %d \"%s\"\n",
						errno,
						strerror(errno)
					    );
				exit(1);
			}

			if(nwrite < nread){
				printf("TODO: sendall()\n");
				exit(1);
			}

		/*---------------------------------------*/

			printf("aspetto  id: %d\n", id_to_wait);
			printf("ricevuto id: %d\n", ntohl(buf.id));


			if(ntohl(buf.id) >= id_to_wait){
				buf.id = ntohl(buf.id);
				aggiungi_in_ordine(&to_send, buf);
				printf("aggiunto id: %d\n", buf.id);
			}

			while( to_send.next != NULL && to_send.next->p.id == id_to_wait){

				memset(&buf, 0, sizeof(buf));
				buf = pop(&to_send);
				nwrite = write( tcp_sock,
								buf.body,
								nread-HEADERSIZE
							   );
				if (nwrite == -1){
					printf ("write() failed, Err: %d \"%s\"\n",
							errno,
							strerror(errno)
							);
					exit(1);
				}
				if(nwrite < nread-HEADERSIZE){
					printf("TODO: sendall()\n");
					exit(1);
				}

				printf("inviato  id: %d %d byte\n", buf.id, nwrite);
				printf("%s\n", buf.body);
				id_to_wait++;
			}
		}
		if(buf.tipo == 'I') {
			printf("ICMP! %d\n", ntohl(buf.id) );
		}

	printf("----------------\n");
	}
	if (nread == -1){
		 printf ("recvfrom() failed, Err: %d \"%s\"\n",
				 errno,
				 strerror(errno)
				 );
         exit(1);
	} else {
		printf ("Trasferimento completato\n");
	}

	return(0);
}
