/* la dimensione massima dei pacchetti */
#define MAXSIZE 65000 /*65536*/

/* la dimensione in byte dell'header dei datagram UDP */
#define HEADERSIZE 5

/* la dimensione massima del body dei pacchetti */
#define BODYSIZE (MAXSIZE) - (HEADERSIZE)

/* il timeout della select del proxysender */
#define TIMEOUT 0   /* secondi */
#define MSTIMEOUT 800000 /* microsecondi */

/* timeout di attesa della chiusura UDP da parte del proxyreceiver */
#define CLOSETIMEOUT 5

/* struttura del pacchetto UDP */
typedef struct packet{
	/* l'ID del paccheto, intero senza segno a 32 bit */
	uint32_t id;
	/* il carattere che identifica il tipo di pacchetto:
	 * 'B' per i normali pacchetti BODY, anche gli ACK
	 * 'I' per gli ICMP
	 * 'E' per gli errori interni (aggiunto da noi) */
	char tipo;
	/* una stringa per il corpo del pacchetto */
	char body[BODYSIZE];
} __attribute__((packed)) packet;

/* la struttura dell'ICMP */
typedef struct ICMP{
	/* intero senza segno a 32 bit che rappresenta l'id dell'ICMP */
	uint32_t id;
	/* un carattere che rappresenta il tipo 'I' dell'ICMP */
	char tipo;
	/* l'ID del pacchetto scartato */
	uint32_t idpck;
} __attribute__((packed)) ICMP;

/* la struttura degli elementi delle liste dinamiche utilizzate dai
 * proxy. Questa struttura è usata anche dalle sentinelle, che però
 * utilizzano solo il campo next */
typedef struct lista{
	/* il puntatore all'elemento successivo */
    struct lista* next;
    /* il pacchetto */
    packet p;
    /* il timestamp abbinato al pacchetto */
    struct timeval sentime;
    /* la dimensione del pacchetto */
    int size;
} __attribute__((packed)) lista;

/* prototipi delle funzioni di utility definite in "utils.c" */

int timeval_subtract (struct timeval *result,
				      struct timeval *x,
				      struct timeval *y);

int controlla_scadenza(struct timeval *p);

/* variabile globale che contiene il numero di pacchetti contenuti
 * in una lista */
int nlist;

void stampalista(lista* sentinella);
void aggiungi( lista* sentinella, packet p, int size );
void aggiungi_in_ordine( lista* sentinella, packet p, int size );
lista pop(lista* sentinella);
lista rimuovi(lista* sentinella, uint32_t id);

int get_socket(int type);
void sock_opt_reuseaddr(int socketfd);
void name_socket(struct sockaddr_in *opt, uint32_t ip_address, uint16_t port);
void sock_bind(int socketfd, struct sockaddr_in* Local);
void sock_connect(int sock, struct sockaddr_in *opt);
void sock_listen(int sock);
int sock_accept(int socketfd, struct sockaddr_in *opt);
int TCP_connection_send(const char *remote_ip, int remote_port);
int TCP_connection_recv(int local_port);
int UDP_sock(int local_port);

ssize_t readn (int fd, char *buf, size_t n, struct sockaddr_in *from);
void writen (int fd, char *buf, size_t n, struct sockaddr_in *to);
