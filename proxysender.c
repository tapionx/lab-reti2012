/*
 * 	proxysender.c : usando datagram UDP simula una connessione TCP
 * 	attraverso un programma ritardatore che scarta i pacchetti in modo
 * 	casuale, simulando una rete reale
 */

/* File di header */
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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* File di header del progetto */
#include "utils.h"

int main(int argc, char *argv[]){

	/* numero di elementi nella lista,
	 * variabile globale aggiornata dalle funzioni delle liste */
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
	lista  buf_l;

	char remote_ip[16];

	uint32_t progressive_id = 0;

	/* per la select */
	fd_set rfds;
	int retsel;
	int fdmax;

	/* per gestire il timeout dei pacchetti */
	struct timeval timeout, curtime, towait;

	/* elemento sentinella della lista dei pacchetti che
	 * devono ricevere ACK */
  lista to_ack;
  /* la lista viene inizializzata come vuota */
  to_ack.next = NULL;

	/* il timeout viene definito nel file di header "utils.h" */
	timeout.tv_sec  = TIMEOUT;
	timeout.tv_usec = MSTIMEOUT;

	/* recupero dei parametri da linea di comando.
	 * nessun parametro è obbligatorio, infatti se un parametro non
	 * viene fornito, viene utilizzato il valore di default */

	/* IP del Ritardatore, default localhost */
	if(argc > 1)
		strcpy(remote_ip, argv[1]);
	else
		strcpy(remote_ip,	"127.0.0.1");

	/* porta in ascolto del proxysender, default 59000 */
	if(argc > 2)
		local_port_tcp = atoi(argv[2]);
	else
		local_port_tcp = 59000;

	/* porta in uscita del proxysender, default 60000 */
	if(argc > 3)
		local_port_udp = atoi(argv[3]);
	else
		local_port_udp = 60000;

	/* prima porta del ritardatore */
	if(argc > 4)
		rit_port[0] = atoi(argv[4]);
	else
		rit_port[0] = 61000;

		rit_port[1] = rit_port[0]+1;
		rit_port[2] = rit_port[1]+1;

	printf("IP ritardatore: %s \
					local TCP port: %d \
					local UDP port: %d \
					porta ritardatore: %d \n",
				 remote_ip,
				 local_port_tcp,
				 local_port_udp,
				 rit_port[0]
				);
	/********************************************************/

	/* inizializzazione del socket UDP (utils.c) */
	udp_sock = UDP_sock(local_port_udp);

	/* inizializzazione del socket TCP (utils.c)
	 * il valore restituito è il socket di connessione, non quello
	 * iniziale generico */
	tcp_sock = TCP_connection_recv(local_port_tcp);

	/* calcolo NFDS: indica il range dei files descriptors a cui siamo interessati per la select() */
	fdmax = (udp_sock > tcp_sock)? udp_sock+1 : tcp_sock+1;

	/* il programma consiste in un ciclo infinito */
	while(1){

		/* se la connessione TCP è stata chiusa e sono stati inviati tutti
		 * i pacchetti, invio un segnale di terminazione al proxyreceiver */
		if( tcp_sock == -1 && nlist == 0){
			buf_p.id = 0;
			buf_p.tipo = 'B';
			writen(udp_sock, (char*)&buf_p, HEADERSIZE+1, &to);
			aggiungi(&to_ack, buf_p, HEADERSIZE + 1);
			printf("\nTutti i pacchetti inviati, inviato segnale di chiusura\n");
		}

		/* debug: stampa quanti pacchetti ci sono nella lista */
		printf("\r%d  ", nlist);
		fflush(stdout);

		/* incremento del turno della porta del ritardatore */
		rit_turno = (rit_turno + 1) % 3;
		name_socket(&to, inet_addr(remote_ip), rit_port[rit_turno]);

		/* inizializzo le strutture per la select()
		 * il socket tcp viene aggiunto solo se è ancora attivo */
		FD_ZERO(&rfds);
		if(tcp_sock != -1)
			FD_SET(tcp_sock, &rfds);
		FD_SET(udp_sock, &rfds);

		/* azzeramento dei buffer */
		memset(&buf_l, 0, sizeof(lista));
		memset(&buf_p, 0, sizeof(packet));

		/* calcolo del timeout con cui chiamare la select:
		 * dipende dal tempo di inserimento del primo pacchetto. */

		/* se la lista è vuota, la select attende all'infinito */
		if(to_ack.next == NULL){
			retsel = select(fdmax, &rfds, NULL, NULL, NULL);
		} else {
			/* timestamp attuale */
			if(gettimeofday(&(curtime), NULL)){
				printf("gettimeofday() fallita, Err: %d \"%s\"\n",
							 errno,
							 strerror(errno)
					  );
				exit(EXIT_FAILURE);
			}
			/* calcolo del tempo di attesa del primo pacchetto della lista */
			timeval_subtract(&curtime, &curtime, &(to_ack.next->sentime));
			/* se il timer del pacchetto è scaduto la select si sveglierà
			 * subito */
			if(timeval_subtract(&towait, &timeout, &curtime)){
				towait.tv_usec = 0;
				towait.tv_sec = 0;
			}
		retsel = select(fdmax, &rfds, NULL, NULL, &towait);
		}

		/* se la select fallisce viene restituito errore */
		if (retsel == -1){
			printf("select() fallita, Err: %d \"%s\"\n",
						 errno,
						 strerror(errno)
					  );
			 exit(EXIT_FAILURE);
		}
		/* altrimenti, se la select è stata interrotta da un socket */
		else if (retsel) {

			/* se è stato il socket TCP a svegliare dalla select
			 * significa che ci sono dati disponibili da parte del sender */
			if(FD_ISSET(tcp_sock, &rfds)){

				/* lettura dei dati dal sender */
				nread = readn(tcp_sock, (char*)buf_p.body, BODYSIZE, NULL );

				/* se non viene letto nulla, significa che il sender ha chiuso
				 * la connessione, quindi ha terminato di inviare dati */
				if(nread == 0){
					printf("\nLa connessione TCP e' stata chiusa\n");
					/* chiusura della connessione TCP */
					close(tcp_sock);
					/* il descrittore del socket viene impostato a -1 per avviare
					 * le operazioni di chiusura del protocollo UDP */
					tcp_sock = -1;
				} else {

					/* l'ID dei datagram UDP è progressivo, parte da 1 */
					progressive_id++;
					/* l'ID viene convertito nell'endianess di rete */
					buf_p.id = htonl(progressive_id);
					buf_p.tipo = 'B';

					/* invio del datagram UDP contenente l'header generato
					 * e il body ricevuto dal sender */
					writen(udp_sock, (char*)&buf_p, HEADERSIZE + nread, &to);

					/* l'ID viene riconvertito nell'endianess della macchina
					 * e il pacchetto viene inserito nella lista dei datagram da
					 * confermare con ACK. L'inserimento avviene in coda */
					buf_p.id = ntohl(buf_p.id);
					aggiungi(&to_ack, buf_p, nread+HEADERSIZE);
				}
			}
			/* la select si è interrotta perchè ci sono dati disponibili
			 * da parte del proxyreceiver */
			if(FD_ISSET(udp_sock, &rfds)){

				/* lettura dei dati UDP */
				nread = readn( udp_sock, (char*)&buf_p, MAXSIZE, &from);

				/* se viene ricevuto un ACK viene rimosso il rispettivo
				 * pacchetto dalla lista */
				if(buf_p.tipo == 'B'){
					buf_p.id = ntohl(buf_p.id);
					buf_l = rimuovi(&to_ack, buf_p.id);

					/* se viene ricevuto un ACK del segnale di terminazione
					 * posso chiudere il programmma */
					if(buf_p.id == 0 && tcp_sock == -1 && nlist == 0){
						printf("\nACK chiusura, terminazione.\n");
						writen(udp_sock, (char*)&buf_p, HEADERSIZE + 1, &to);
						close(udp_sock);
						exit(EXIT_SUCCESS);
					}
				}
				/* se viene ricevuto un ICMP il pacchetto corrispondente
				 * deve essere rimosso dalla lista, inviato di nuovo e inserito
				 * in coda alla lista */
				if(buf_p.tipo == 'I'){
					buf_l = rimuovi(&to_ack, ((ICMP*)&buf_p)->idpck);
					/* se il pacchetto non è presente nella lista ignoro l'ICMP */
					if(buf_l.p.tipo != 'E'){
						buf_l.p.id = htonl(buf_l.p.id);
						writen( udp_sock, (char*)&buf_l.p, buf_l.size, &to);
						buf_l.p.id = ntohl(buf_l.p.id);
						aggiungi(&to_ack, buf_l.p, buf_l.size);
					}
				}
			}
		} else {
			/* se la select viene terminata dopo il timeout specificato
			 * bisogna inviare nuovamente il primo pacchetto della lista
			 * e reinserirlo in coda */
			buf_l = pop(&(to_ack));
			buf_l.p.id = htonl(buf_l.p.id);
			writen( udp_sock, (char*)&buf_l.p, buf_l.size, &to);
			buf_l.p.id = ntohl(buf_l.p.id);
			aggiungi(&to_ack, buf_l.p, buf_l.size);
    }
	}
	/* L'esecuzione non arriva mai a questo punto */
}
