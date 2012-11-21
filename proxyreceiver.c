/*
 * 	proxyreceiver.c : usando datagram UDP simula una connessione TCP
 * 	attraverso un programma ritardatore che scarta i pacchetti in modo
 * 	casuale, simulando una rete reale
 */

/* file di header */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

/* file di header del progetto */
#include "utils.h"

int main(int argc, char *argv[]){

	/* variabile globale che contiene il numero di elementi nella lista
	 * viene tenuta aggiornata dalle funzioni di manipolazione delle
	 * liste */
	extern int nlist;

	/* variabile booleana che controlla se è arrivato il datagram
	 * di terminazione del protocollo */
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

	/* recupero parametri da riga di comando
 	 * nessun parametro è obbligatorio, infatti se un parametro non
	 * viene fornito, viene utilizzato il valore di default */

	/* IP del sender, default localhost */
	if(argc > 1)
		strcpy(remote_ip, argv[1]);
	else
		strcpy(remote_ip, "127.0.0.1");
	/* IP del ritardatore, default localhost */
	if(argc > 2)
		strcpy(ritardatore_ip, argv[2]);
	else
		strcpy(ritardatore_ip, "127.0.0.1");
	/* porta UDP del proxyreceiver, default 63000 */
	if(argc > 3)
		local_port = atoi(argv[3]);
	else
		local_port = 63000;
	/* porta TCP del receiver, default 64000 */
	if(argc > 4)
		remote_port = atoi(argv[4]);
	else
		remote_port = 64000;
	/* stampa dei parametri utilizzati */
	printf("IP receiver: %s\nIP ritardatore: %s\nlocal UDP port: %d\nsender TCP port: %d\n",
    	   remote_ip,
    	   ritardatore_ip,
		   local_port,
		   remote_port
		  );

	/* inizializzazione del socket UDP */
	udp_sock = UDP_sock(local_port);
	/* inizializzazione del socket TCP, viene ritornato il socket di
	 * connessione, non quello generico */
	tcp_sock = TCP_connection_send(remote_ip, remote_port);

	name_socket(&from, htonl(INADDR_ANY), 0);

	/* il programma consiste in un loop infinito interrotto da
	 * determinati datagram UDP */
	while(1) {
		/* se è stato ricevuto il datagram di terminazione attendo
		 * 5 secondi per ulteriori pacchetti, nel caso in cui il
		 * proxysender non avesse ricevuto l'ACK di terminazione.
		 * Se durante 5 secondi ricevo l'ACK finale termino, oppure se
		 * passano 5 secondi senza ricevere datagram */
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

		/* DEBUG: stampo quanti pacchetti ci sono nella lista dei
		 * pacchetti da inviare al receiver */
		printf("\r%d  ", nlist);
		fflush(stdout);

		/* leggo un datagram UDP dal Ritardatore */
		nread = readn(udp_sock, (char*)&buf, MAXSIZE, &from);

		/* se si tratta di un pacchetto "Body" */
		if(buf.tipo == 'B'){
			/* se ha id=0 è un pacchetto di terminazione */
			if(ntohl(buf.id) == 0){
				/* se è il primo pacchetto accendo il flag dedicato */
				if(arrivata_terminazione == 0){
					arrivata_terminazione = 1;
					printf("\nArrivata terminazione\n");
				/* se è il secondo pacchetto di terminazione, il
				 * protocollo di chiusura è completo perchè il
				 * proxysender è stato informato della terminazione
				 * quindi viene chiuso il programma e i socket*/
				} else {
					printf("\nArrivato ACK, terminazione.\n");
					close(udp_sock);
					close(tcp_sock);
					exit(EXIT_SUCCESS);
				}
			}

			/* Imposto la destinazione dell'ACK
			 * inviandolo sullo stesso canale dal quale è arrivato
			 * l'udp perchè probabilmente non è in BURST */
			name_socket(&to, inet_addr(ritardatore_ip), ntohs(from.sin_port));

			/* invio ACK del pacchetto ricevuto */
			writen(udp_sock, (char*)&buf, HEADERSIZE+1, &to);

			/* solo se il pacchetto ricevuto ha un ID >= del primo ID
			 * della lista, e quindi deve essere ancora inviato,
			 * lo aggiungo alla lista dei pacchetti da inviare */
			if(ntohl(buf.id) >= id_to_wait){
				buf.id = ntohl(buf.id);
				aggiungi_in_ordine(&to_send, buf, nread-HEADERSIZE);
				printf("\r%d  ", nlist);
				fflush(stdout);
			}

			/* se il datagram appena arrivato ha permesso l'invio di
			 * altri pacchetti in ordine al receiver, li rimuovo dalla
			 * lista e li invio in TCP */
			while( to_send.next != NULL && to_send.next->p.id == id_to_wait){
				/* azzero i buffer */
				memset(&buf_l, 0, sizeof(lista));
				memset(&buf_l.p, 0, sizeof(packet));
				/* rimuovo il pacchetto in testa alla lista */
				buf_l = pop(&to_send);
				/* invio il pacchetto in TCP al receiver, senza header*/
				writen( tcp_sock,
				    	buf_l.p.body,
						buf_l.size,
						NULL
					   );
				id_to_wait++;
				/* DEBUG: stampo il numero di pacchetti nella lista */
				printf("\r%d  ", nlist);
				fflush(stdout);
			}
		}
		/* se il datagram UDP ricevuto è di tipo ICMP
		 * significa che l'ACK appena inviato è stato scartato,
		 * quindi viene ricostruito a partire dall'id contenuto nell'
		 * ICMP e inviato nuovamente */
		if(buf.tipo == 'I') {
			buf.id = ((ICMP*)&buf)->idpck;
			buf.tipo = 'B';
			writen(udp_sock, (char*)&buf, HEADERSIZE+1, &to);
		}
	}
	/* l'esecuzione non raggiunge mai questo punto del codice */
}
