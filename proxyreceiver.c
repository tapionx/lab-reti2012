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
#include <sys/select.h>
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
	uint16_t porte_ritardatore[3];
	in_addr_t ip_ritardatore;

	/* una lista ordinata che contiene i pacchetti da spedire al
	   receiver */
	lista to_send;

	uint32_t id_to_wait = 1;

	fd_set rfds;
	struct timeval tv;
	int retval;
	
	struct in_addr indirizzo_ritardatore, indirizzo_receiver;

	/* recupero parametri da riga di comando
 	 * nessun parametro è obbligatorio, infatti se un parametro non
	 * viene fornito, viene utilizzato il valore di default */

	/* IP del ritardatore, default localhost */
	if(argc > 1)
		strcpy(ritardatore_ip, argv[1]);
	else
		strcpy(ritardatore_ip, "127.0.0.1");
	/* IP del receiver, default localhost */
	if(argc > 2)
		strcpy(remote_ip, argv[2]);
	else
		strcpy(remote_ip, "127.0.0.1");
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
	/* prima porta del ritardatore latoreceiver, serve solo per
	 * filtrare i pacchetti estranei */
	if(argc > 5)
		porte_ritardatore[0] = htons(atoi(argv[5]));
	else
		porte_ritardatore[0] = htons(62000);

	porte_ritardatore[1] = htons(ntohs(porte_ritardatore[0])+1);
	porte_ritardatore[2] = htons(ntohs(porte_ritardatore[1])+1);

	/* copia dell'ip del ritardatore in endianess di rete per
	 * migliori performance durante il filtraggio */
	/* stampa dei parametri utilizzati */
	indirizzo_ritardatore = DNSquery(ritardatore_ip);
	ip_ritardatore = indirizzo_ritardatore.s_addr;
	indirizzo_receiver = DNSquery(remote_ip);

    printf("IP ritardatore: %s\n", inet_ntoa(indirizzo_ritardatore));
	printf("IP receiver: %s\n", inet_ntoa(indirizzo_receiver));
    printf("porta proxyreceiver: %d\n", local_port);
    printf("porta receiver: %d\n", remote_port);
    printf("prima porta ritardatore: %d\n", ntohs(porte_ritardatore[0]));

	/* inizializzazione del socket UDP */
	udp_sock = UDP_sock(local_port);
	/* inizializzazione del socket TCP, viene ritornato il socket di
	 * connessione, non quello generico */
	tcp_sock = TCP_connection_send(indirizzo_receiver, remote_port);

	name_socket(&from, htonl(INADDR_ANY), 0);

	/* il programma consiste in un loop infinito interrotto da
	 * determinati datagram UDP */
	while(1) {
		/* se è stato ricevuto il datagram di terminazione attendo
		 * 6 secondi per ulteriori pacchetti, nel caso in cui il
		 * proxysender non avesse ricevuto l'ACK di terminazione.
		 * Se durante CLOSETIMEOUT secondi ricevo l'ACK finale termino, 
		 * oppure se passano CLOSETIMEOUT secondi senza ricevere nulla*/
		if(arrivata_terminazione && nlist == 0){
			FD_ZERO(&rfds);
            FD_SET(udp_sock, &rfds);
            tv.tv_sec = CLOSETIMEOUT;
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
		printf("\rpacchetti rimanenti: %d  ", nlist);
		fflush(stdout);

		/* leggo un datagram UDP dal Ritardatore */
		nread = readn(udp_sock, (char*)&buf, MAXSIZE, &from);

		/* se il pacchetto non proviene dal ritardatore viene scartato*/
		if( nread > HEADERSIZE &&
		    from.sin_addr.s_addr == ip_ritardatore &&
		   (from.sin_port == porte_ritardatore[0] ||
		    from.sin_port == porte_ritardatore[1] ||
		    from.sin_port == porte_ritardatore[2])){

			/* se si tratta di un pacchetto "Body" */
			if(buf.tipo == 'B'){
				/* se ha id=0 è un pacchetto di terminazione */
				if(ntohl(buf.id) == 0 && 
                   buf.body[0] == '1' && 
                   !arrivata_terminazione){
					arrivata_terminazione = 1;
					printf("\nArrivata terminazione\n");
				}
				if(ntohl(buf.id) == 0 && 
                   buf.body[0] == '2' && 
                   arrivata_terminazione){
					/* se è il secondo pacchetto di terminazione, il
					 * protocollo di chiusura è completo perchè il
					 * proxysender è stato informato della terminazione
					 * quindi viene chiuso il programma e i socket*/
					printf("\nArrivato ACK, terminazione.\n");
					close(udp_sock);
					close(tcp_sock);
					exit(EXIT_SUCCESS);
				}

				/* Imposto la destinazione dell'ACK
				 * inviandolo sullo stesso canale dal quale è arrivato
				 * l'udp perchè probabilmente non è in BURST */
				name_socket(&to, ip_ritardatore, ntohs(from.sin_port));

				/* invio ACK del pacchetto ricevuto */
				writen(udp_sock, (char*)&buf, HEADERSIZE+1, &to);

				/* solo se il pacchetto ricevuto ha un ID >= del primo ID
				 * della lista, e quindi deve essere ancora inviato,
				 * lo aggiungo alla lista dei pacchetti da inviare */
				if(ntohl(buf.id) >= id_to_wait){
					buf.id = ntohl(buf.id);
					aggiungi_in_ordine(&to_send, buf, nread-HEADERSIZE);
				}

				/* se il datagram appena arrivato ha permesso l'invio di
				 * altri pacchetti in ordine al receiver, li rimuovo dalla
				 * lista e li invio in TCP */
				while(to_send.next != NULL && 
				       to_send.next->p.id==id_to_wait){
					/* azzero i buffer */
					memset(&buf_l, 0, sizeof(lista));
					memset(&buf_l.p, 0, sizeof(packet));
					/* rimuovo il pacchetto in testa alla lista */
					buf_l = pop(&to_send);
					/* invio il pacchetto al receiver senza header*/
					writen( tcp_sock,
							buf_l.p.body,
							buf_l.size,
							NULL
						   );
					id_to_wait++;
					/* DEBUG: stampo il numero di pacchetti nella lista */
					printf("\rpacchetti rimanenti: %d  ", nlist);
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
		} else {
			printf("ricevuto pacchetto UDP estraneo, scarto.\n");
		}
	}
	/* l'esecuzione non raggiunge mai questo punto del codice */
}
