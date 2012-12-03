/*
 * 	proxysender.c : usando datagram UDP simula una connessione TCP
 * 	attraverso un programma ritardatore che scarta i pacchetti in modo
 * 	casuale, simulando una rete reale
 */

/* Necessario per utilizzare inet_aton senza warning */
#define _GNU_SOURCE

/* File di header */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>

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
	uint16_t porte_rit[3];
	int rit_turno = 0;

	struct sockaddr_in to, from;

	packet buf_p;
	lista  buf_l;

	char remote_ip[16];
	in_addr_t ip_ritardatore;

	uint32_t progressive_id = 0;

	/* per la select */
	fd_set rfds;
	int retsel;
	int fdmax;

	/* contatori per le statistiche */
	int quanti_timeout = 0;
	int quanti_pacchetti_tcp = 0;
	int quanti_datagram_inviati = 0;
	int quanti_icmp = 0;
	int quanti_ack = 0;
	int overhead = 0;

	struct in_addr indirizzo_ritardatore;
	
	/* per gestire il timeout dei pacchetti */
	struct timeval timeout;

	/* timestamp di inizio e fine dell' esecuzione */
	struct timeval inizio, fine;

	/* elemento sentinella della lista dei pacchetti che
	 * devono ricevere ACK */
	lista to_ack;
	/* la lista viene inizializzata come vuota */
	to_ack.next = NULL;

	/* recupero dei parametri da linea di comando.
	 * nessun parametro è obbligatorio, infatti se un parametro non
	 * viene fornito, viene utilizzato il valore di default */

	/* IP del Ritardatore, default localhost */
	if(argc > 1)
		strcpy(remote_ip, argv[1]);
	else
		strcpy(remote_ip, "127.0.0.1");

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

	/* Variabili copiate nell'endianess di rete per migliorare le
	 * prestazioni quando vengono filtrati i pacchetti in ingresso */
	indirizzo_ritardatore = DNSquery(remote_ip);
	ip_ritardatore = indirizzo_ritardatore.s_addr;
	porte_rit[0] = htons(rit_port[0]);
	porte_rit[1] = htons(rit_port[1]);
	porte_rit[2] = htons(rit_port[2]);

	printf("IP ritardatore: %s\n", inet_ntoa(indirizzo_ritardatore));
	printf("porta TCP: %d\n", local_port_tcp);
	printf("porta UDP: %d\n", local_port_udp);
	printf("porta ritardatore: %d\n", rit_port[0]);
	
  /********************************************************/

	/* inizializzazione del socket UDP (utils.c) */
	udp_sock = UDP_sock(local_port_udp);
	
	printf("In attesa di connessione da parte del sender...\n");

	/* inizializzazione del socket TCP (utils.c)
	 * il valore restituito è il socket di connessione, non quello
	 * iniziale generico */
	tcp_sock = TCP_connection_recv(local_port_tcp);

	/* salvo il tempo di inizio dell'esecuzione */
	if(gettimeofday(&(inizio), NULL)){
		printf("gettimeofday() fallita, Err: %d \"%s\"\n",
					 errno,
					 strerror(errno)
			  );
		exit(EXIT_FAILURE);
	}

	/* calcolo NFDS: indica il range dei files descriptors a cui siamo 
     * interessati per la select() */
	fdmax = (udp_sock > tcp_sock)? udp_sock+1 : tcp_sock+1;

	/* il programma consiste in un ciclo infinito */
	while(1){

		/* se la connessione TCP è stata chiusa e sono stati inviati tutti
		 * i pacchetti, invio un segnale di terminazione al proxyreceiver */
		if( tcp_sock == -1 && nlist == 0){
			buf_p.id = 0;
			buf_p.tipo = 'B';
			buf_p.body[0] = '1';
			quanti_datagram_inviati++;
			writen(udp_sock, (char*)&buf_p, HEADERSIZE+1, &to);
			aggiungi(&to_ack, buf_p, HEADERSIZE + 1);
			printf("\nTutti i pacchetti inviati.\n");
		}

		/* debug: stampa quanti pacchetti ci sono nella lista */
		printf("\rpacchetti rimanenti: %d  ", nlist);
		fflush(stdout);

		/* incremento del turno della porta del ritardatore */
		rit_turno = (rit_turno + 1) % 3;
		name_socket(&to, ip_ritardatore, rit_port[rit_turno]);

		/* inizializzo le strutture per la select()
		 * il socket tcp viene aggiunto solo se è ancora attivo */
		FD_ZERO(&rfds);
		if(tcp_sock != -1)
			FD_SET(tcp_sock, &rfds);
		FD_SET(udp_sock, &rfds);

		/* azzeramento dei buffer */
		memset(&buf_l, 0, sizeof(lista));
		memset(&buf_p, 0, sizeof(packet));

		/* se la lista è vuota, la select attende all'infinito */
		if(to_ack.next == NULL)
			retsel = select(fdmax, &rfds, NULL, NULL, NULL);
		else {
            /* calcolo del timeout con cui chiamare la select:
             * dipende dal tempo di inserimento del primo pacchetto. */
            timeout = to_ack.next->sentime;
            if(controlla_scadenza(&timeout)){
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
            }
            retsel = select(fdmax, &rfds, NULL, NULL, &timeout);
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
				quanti_pacchetti_tcp++;
				/* lettura dei dati dal sender */
				nread = readn(tcp_sock, (char*)buf_p.body, BODYSIZE, NULL );
				/* NEL CASO DEL TCP SIAMO SICURI CHE IL MESSAGGIO
				 * PROVENGA DAL SENDER */
				/* se non viene letto nulla, significa che il sender ha chiuso
				 * la connessione, quindi ha terminato di inviare dati */
				if(nread == 0){
					printf("\nLa connessione TCP e' stata chiusa\n");
					/* chiusura della connessione TCP */
					close(tcp_sock);
					/* il descrittore del socket viene impostato a -1 per 
           * avviare le operazioni di chiusura del protocollo UDP */
					tcp_sock = -1;
				} else {
					/* l'ID dei datagram UDP è progressivo, parte da 1 */
					progressive_id++;
					/* l'ID viene convertito nell'endianess di rete */
					buf_p.id = htonl(progressive_id);
					buf_p.tipo = 'B';

					/* invio del datagram UDP contenente l'header generato
					 * e il body ricevuto dal sender */
					quanti_datagram_inviati++;
					writen(udp_sock, (char*)&buf_p, HEADERSIZE + nread, &to);

					/* l'ID viene riconvertito nell'endianess della macchina
					 * e il pacchetto viene inserito nella lista dei datagram 
           * da confermare con ACK. L'inserimento avviene in coda */
					buf_p.id = ntohl(buf_p.id);
					aggiungi(&to_ack, buf_p, nread+HEADERSIZE);
				}
			}
			/* la select si è interrotta perchè ci sono dati disponibili
			 * da parte del proxyreceiver */
			if(FD_ISSET(udp_sock, &rfds)){

				/* lettura dei dati UDP */
				nread = readn( udp_sock, (char*)&buf_p, MAXSIZE, &from);

				/* se il pacchetto non proviene dal ritardatore,
				 * viene scartato */
				if( nread > HEADERSIZE &&
				    from.sin_addr.s_addr == ip_ritardatore &&
				     (  from.sin_port == porte_rit[0]
				     || from.sin_port == porte_rit[1]
				     || from.sin_port == porte_rit[2])){

					/* se viene ricevuto un ACK viene rimosso il rispettivo
					 * pacchetto dalla lista */
					if(buf_p.tipo == 'B'){
						quanti_ack++;
						buf_p.id = ntohl(buf_p.id);
						buf_l = rimuovi(&to_ack, buf_p.id);
						/* se viene ricevuto un ACK del segnale di terminazione
						 * posso chiudere il programmma */
						if(buf_p.id == 0 && tcp_sock == -1 && nlist == 0){
							printf("\nACK chiusura, terminazione.\n");
							/* Invio ACK finale al proxyreceiver */
							buf_p.body[0] = '2';
							writen(udp_sock, (char*)&buf_p, HEADERSIZE + 1, &to);
							close(udp_sock);
							/* prendo il tempo di fine esecuzione */
							/* salvo il tempo di inizio dell'esecuzione */
							if(gettimeofday(&(fine), NULL)){
								printf("gettimeofday() fallita, Err: %d \"%s\"\n",
											 errno,
											 strerror(errno)
									  );
								exit(EXIT_FAILURE);
							}
							/* sottraggo i tempi */
							timeval_subtract(&fine, &fine, &inizio);
							/* stampo le statistiche */
							overhead = (quanti_datagram_inviati - quanti_pacchetti_tcp) * 100 / quanti_pacchetti_tcp;
							printf("---- statistiche ----\n");
							printf("timeout: %d\n", quanti_timeout);
							printf("pacchetti tcp ricevuti: %d\n", quanti_pacchetti_tcp);
							printf("datagram udp inviati: %d\n", quanti_datagram_inviati);
							printf("icmp: %d\n", quanti_icmp);
							printf("ack: %d\n", quanti_ack);
							printf("overhead: %d%%\n", overhead);
							printf("tempo di esecuzione: %d sec\n", (int)fine.tv_sec);
							exit(EXIT_SUCCESS);
						}
					}
					/* se viene ricevuto un ICMP il pacchetto corrispondente
					 * deve essere rimosso dalla lista, inviato di nuovo e 
					 * inserito in coda alla lista */
					if(buf_p.tipo == 'I'){
						quanti_icmp++;
						buf_l = rimuovi(&to_ack, ((ICMP*)&buf_p)->idpck);
						/* se il pacchetto non è nella lista ignoro l'ICMP */
						if(buf_l.p.tipo != 'E'){
							buf_l.p.id = htonl(buf_l.p.id);
							quanti_datagram_inviati++;
							writen(udp_sock, (char*)&buf_l.p, buf_l.size, &to);
							buf_l.p.id = ntohl(buf_l.p.id);
							aggiungi(&to_ack, buf_l.p, buf_l.size);
						}
					}
				} else {
					printf("ricevuto datagram UDP estraneo, scarto.\n");
				}
			}
		} else {
			/* se la select viene terminata dopo il timeout specificato
			 * bisogna inviare nuovamente il primo pacchetto della lista
			 * e reinserirlo in coda */
			quanti_timeout++;
			buf_l = pop(&(to_ack));
			buf_l.p.id = htonl(buf_l.p.id);
			quanti_datagram_inviati++;
			writen( udp_sock, (char*)&buf_l.p, buf_l.size, &to);
			buf_l.p.id = ntohl(buf_l.p.id);
			aggiungi(&to_ack, buf_l.p, buf_l.size);
    }
	}
	/* L'esecuzione non arriva mai a questo punto */
}
