#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>

#include "utils.h"

/* ---------------------- Sottrarre TIMEVAL --------------------------
 * http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 */

/* CALCOLA X - Y
 * ritorna 1 se il risultato è negativo */

/* Subtract the `struct timeval' values X and Y,
storing the result in RESULT.
Return 1 if the difference is negative, otherwise 0. */

int timeval_subtract (struct timeval *result,
				      struct timeval *x,
				      struct timeval *y) {
/* Perform the carry for the later subtraction by updating y. */
if (x->tv_usec < y->tv_usec) {
 int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
 y->tv_usec -= 1000000 * nsec;
 y->tv_sec += nsec;
}
if (x->tv_usec - y->tv_usec > 1000000) {
 int nsec = (x->tv_usec - y->tv_usec) / 1000000;
 y->tv_usec += 1000000 * nsec;
 y->tv_sec -= nsec;
}

/* Compute the time remaining to wait.
  tv_usec is certainly positive. */
result->tv_sec = x->tv_sec - y->tv_sec;
result->tv_usec = x->tv_usec - y->tv_usec;

/* Return 1 if result is negative. */
return x->tv_sec < y->tv_sec;
}

/* quanti elementi nella lista */
int nlist = 0;

/* Controlla se un pacchetto del proxysender ha il timer scaduto */
int controlla_scadenza(struct timeval *p){
	struct timeval attuale, trascorso, timeout;
    int ret;
	timeout.tv_sec  = TIMEOUT;
	timeout.tv_usec = MSTIMEOUT;
	/* prendo il tempo attuale */
	if(gettimeofday(&(attuale), NULL)){
		printf("gettimeofday() fallita, Err: %d \"%s\"\n",
					 errno,
					 strerror(errno)
			  );
		exit(EXIT_FAILURE);
	}
	/* calcolo il tempo trascorso */
	timeval_subtract(&trascorso, &attuale, p);
	/* controllo se il tempo rimanente supera il timer */
	ret = timeval_subtract(p, &timeout, &trascorso);
    return ret;
}

/* -------- LISTE DINAMICHE con malloc() --------------*/
/* funzione di debug che stampa il contenuto di una lista passata */
void stampalista(lista* sentinella){
    lista* cur = sentinella->next;
    printf("[");
    while(cur != NULL){
        printf(" (%d - %d - %s) ",cur->p.id, cur->size, cur->p.body);
        /*printf(" (%d|%c|%s - %d) ", cur->p.id, cur->p.tipo, cur->p.body, (int)cur->sentime.tv_sec);*/
        cur = cur->next;
    }
    printf("]\n");

}

/* aggiunge un pacchetto in coda alla lista, prende una sentinella */
void aggiungi( lista* sentinella, packet p, int size){
	/* printf("[%d|%c|%s]\n", p.id, p.tipo, p.body); */
    lista* new;
    lista* cur = sentinella;
    while(cur->next != NULL){
        cur = cur->next;
    }
    /* allocazione memoria per il nuovo elemento con la malloc */
    new = malloc((size_t)sizeof(lista));
    if(new == NULL){
		printf("malloc() failed\n");
		exit(1);
	}
	/* nel pacchetto inserito viene aggiunto il timestamp attuale */
	if(gettimeofday(&(new->sentime), NULL)) {
		printf ("gettimeofday() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
	}
	/* inserisco il pacchetto nella lista */
    memcpy(&(new->p), &p, sizeof(packet));
    /* aggiornamento dei puntatori per rendere la lista coerente */
    new->size = size;
    new->next = NULL;
    cur->next = new;
    /* aggiorno il numero di pacchetti attraverso la variabile globale*/
    nlist++;
}

/* come la funzione aggiungi, ma aggiunge il pacchetto in ordine
 * nella lista in base all' ID */
void aggiungi_in_ordine( lista* sentinella, packet p, int size){
	/* printf("[%d|%c|%s]\n", p.id, p.tipo, p.body); */
    lista* new;
    lista* cur = sentinella;
	/* scorro la lista fino ad individuare la posizione in cui inserire
	 * il pacchetto */
    while(cur->next != NULL && cur->next->p.id < p.id){
        cur = cur->next;
    }
    /* se il pacchetto esiste già lo scarto */
    if(cur->next != NULL && cur->next->p.id == p.id)
		return;
	/* alloco il nuovo elemento con malloc */
    new = malloc((size_t)sizeof(lista));
    if(new == NULL){
		printf("malloc() failed\n");
		exit(1);
	}
	/* il timestamp non viene inserito, perchè questa funzione è usata
	 * solo dal proxyreceiver, che non utilizza il tempo dei pacchetti
	 * in questo modo si incrementano le performance effettuando meno
	 * system call
	if(gettimeofday(&(new->sentime), NULL)) {
		printf ("gettimeofday() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
	} */
	/* inserisco il pacchetto nella lista */
    memcpy(&(new->p), &p, sizeof(packet));
    /* aggiorno i puntatori in modo da mantenere la lista coerente */
    new->size = size;
    new->next = cur->next;
    cur->next = new;
    /* aggiorno il numero di pacchetti attraverso la variabile globale*/
    nlist++;
}

/* elimina e restituisce il pacchetto in testa alla lista passata come
 * parametro (la sentinella) */
/* NOTA: questa funzione non viene mai chiamata con la lista vuota */
lista pop(lista* sentinella){
    lista* todel;
    lista ret;
    memset(&ret, 0, sizeof(lista));
    /* puntatore all'elemento da eliminare */
    todel = sentinella->next;
    /* aggiorno i puntatori */
    sentinella->next = todel->next;
    /* elemento da ritornare */
    memcpy(&ret, todel, sizeof(lista));
    /* libero la memoria */
    free(todel);
    /* aggiornamento del numero dei pacchetti nella lista */
    nlist--;
    return ret;
}

/* elimina un pacchetto dalla lista con un determinato id passato come
 * parametro, se il pacchetto non esiste viene restituito un pacchetto
 * speciale "E" di errore */
lista rimuovi(lista* sentinella, uint32_t id){
	lista* cur = sentinella;
	lista* todel;
	lista ret;
	/* scorro fino alla fine della lista */
	while(cur->next != NULL){
		/* se l'elemento corrente è quello da eliminare */
		if(cur->next->p.id == id){
			todel = cur->next;
			/* aggiorno i puntatori */
			cur->next = cur->next->next;
			/* copio il valore da ritornare */
			memcpy(&ret, todel, sizeof(lista));
			/* libero la memoria */
			free(todel);
			/* decremento il numero di pacchetti nella lista */
			nlist--;
			return ret;
		}
		/* scorro al prossimo elemento */
		cur = cur->next;
	}
	/* se il pacchetto non è stato trovato restituisco un pacchetto
	 * speciale "E" di errore */
	ret.p.tipo = 'E';
	return ret;
}
/* -------------- SOCKET, funzioni ricorrenti -----------------*/

/* questa funzione crea un socket TCP o UDP controllando gli errori */
int get_socket(int type){
    /* SOCK_STREAM = TCP
     * SOCK_DGRAM  = UDP
     */
   int socketfd;
   socketfd = socket(AF_INET, type, 0);
    if (socketfd == -1) {
        printf ("socket() fallita, Err: %d \"%s\"\n", errno,strerror(errno));
        exit(1);
    }
    return socketfd;
}

/* questa funzione imposta il socket con l'opzione REUSEADDR */
void sock_opt_reuseaddr(int socketfd){
    /* avoid EADDRINUSE error on bind() */
    int OptVal = 1;
    int ris;
    ris = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&OptVal, sizeof(OptVal));
    if (ris == -1) {
        printf ("setsockopt() SO_REUSEADDR fallita, Err: %d \"%s\"\n", errno,strerror(errno));
        exit(1);
    }
}

/* popola la struttura sockaddr_in con indirizzo e porta */
void name_socket(struct sockaddr_in *opt, uint32_t ip_address, uint16_t port){
    /* name the socket */
    memset(opt, 0, sizeof(*opt));
    opt->sin_family      =   AF_INET;
    /* or htonl(INADDR_ANY)
     * or inet_addr(const char *)
     */
    opt->sin_addr.s_addr =   ip_address;
    opt->sin_port        =   htons(port);
}

/* effettua la bind sul socket controllando gli errori */
void sock_bind(int socketfd, struct sockaddr_in* Local){
    int ris;
    ris = bind(socketfd, (struct sockaddr*) Local, sizeof(*Local));
    if (ris == -1)  {
        printf ("bind() fallita, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
}

/* esegue la connessione del socket TCP controllando gli errori */
void sock_connect(int sock, struct sockaddr_in *opt){
    /* connection request */
    int ris;
    ris = connect(sock, (struct sockaddr*) opt, sizeof(*opt));
    if (ris == -1)  {
        printf ("connect() fallita, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
}

/* mette un socket TCP in ascolto su una determinata porta
 * controllando gli errori */
void sock_listen(int sock){
    int ris;
    ris = listen(sock, 10 );
    if (ris == -1) {
        printf ("listen() fallita, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
}

/* accetta una connessione TCP e restituisce il socket che rappresenta
 * la connessione appena instaurata, il socket di partenza non viene
 * piu considerato, vengono controllati gli errori */
int sock_accept(int socketfd, struct sockaddr_in *opt){
    int len, newsocketfd;
    memset (opt, 0, sizeof(*opt) );
    /* wait for connection request */
    len=sizeof(*opt);
    newsocketfd = accept(socketfd, (struct sockaddr*) opt, (socklen_t *)&len);
    if (newsocketfd == -1)  {
        printf ("accept() fallita, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
    return newsocketfd;
}

/* inizializza una connessione TCP effettuando tutte le system call
 * necessarie, attraverso le funzioni definite sopra */
int TCP_connection_send(const char *remote_ip, int remote_port){

    int tcp_sock;
    struct sockaddr_in local, server;

    tcp_sock = get_socket(SOCK_STREAM);

    sock_opt_reuseaddr(tcp_sock);

    name_socket(&local, htonl(INADDR_ANY), 0);

    sock_bind(tcp_sock, &local);

    name_socket(&server, inet_addr(remote_ip), remote_port);

    sock_connect(tcp_sock, &server);

    printf ("Connessione avvenuta\n");
    fflush(stdout);

    return tcp_sock;
}

/*
 * wait for TCP connection on the provided port
 * and return the TCP socket when a client connects
 */
int TCP_connection_recv(int local_port) {

    int sock, newsocketfd;
    struct sockaddr_in Local, Cli;

    sock = get_socket(SOCK_STREAM);

    sock_opt_reuseaddr(sock);

    name_socket(&Local, htonl(INADDR_ANY), local_port);

    sock_bind(sock,&Local);

    sock_listen(sock);

    newsocketfd = sock_accept(sock, &Cli);

    printf("Connessione da %s : %d\n",
		inet_ntoa(Cli.sin_addr),
		ntohs(Cli.sin_port)
    );

    return newsocketfd;
}

/* inizializza un socket UDP restituendo il socket creato
 * utilizza le funzioni definite sopra */
int UDP_sock(int local_port){
    int sock;
    struct sockaddr_in local;

    sock = get_socket(SOCK_DGRAM);
    sock_opt_reuseaddr(sock);
    name_socket(&local, htonl(INADDR_ANY), local_port);
    sock_bind(sock,&local);

    return sock;
}

/* legge dati da un socket TCP o UDP controllando gli errori
 * se la syscall viene interrotta dal kernel con EINTR viene ripetuta */
ssize_t readn (int fd, char *buf, size_t n, struct sockaddr_in *from){
	socklen_t len;
	size_t nleft;
	ssize_t nread;
	nleft = n;
	if(from == NULL)
		len = 0;
	else
		len = sizeof(struct sockaddr_in);
	while (1) {
		nread = recvfrom( fd,
						  buf+n-nleft,
						  nleft,
						  0,
						  (struct sockaddr*) from,
						  &len
						 );
		nleft -= nread;
		if ( nread < 0) {
			if (errno != EINTR){
				/*return(-1);*/   /* restituisco errore */
				printf ("recvfrom() fallita, Err: %d \"%s\"\n",
					    errno,
					    strerror(errno)
					   );
				exit(1);
			}
		} else {
			return( n - nleft);
		}

	}
	return( n - nleft);  /* return >= 0 */
}

/* scrive esattamente n byte del buffer passato come parametro
 * sul socket TCP o UDP passato, controlla gli errori e se la
 * system call viene interrotta dal kernel con EINTR la ripete */
void writen (int fd, char *buf, size_t n, struct sockaddr_in *to){
	size_t nleft;
	ssize_t nwritten;
	char *ptr;
	ptr = buf;
	nleft = n;
	while (nleft > 0){
		if ( (nwritten = sendto(
								fd,
								ptr,
								nleft,
								MSG_NOSIGNAL,
								(struct sockaddr*)to,
								(socklen_t )sizeof(struct sockaddr_in)
								 )) < 0) {
			if (errno == EINTR)
				nwritten = 0;   /* and call write() again*/
			else {
				/*return(-1);*/       /* error */
				printf ("recvfrom() fallita, Err: %d \"%s\"\n",
					    errno,
					    strerror(errno)
					   );
				exit(1);
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
}
