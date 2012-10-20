#define BUFSIZE 50

#define MAXSIZE 65000/*65536*/

#define HEADERSIZE 5

#define BODYSIZE (MAXSIZE) - (HEADERSIZE)

#define TIMEOUT 5 /* seconds */

typedef struct packet{
	uint32_t id;
	char tipo;
	char body[BODYSIZE];
} packet;


typedef struct lista{
    struct lista* next;
    packet p;
    struct timeval sentime;
} lista;

void stampalista(lista* sentinella);
void aggiungi( lista* sentinella, packet p );
lista pop(lista* sentinella);
void rimuovi(lista* sentinella, uint32_t id);


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
