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

#define BUFSIZE 50

#define MAXSIZE 65000/*65536*/

#define HEADERSIZE 5

#define BODYSIZE (MAXSIZE) - (HEADERSIZE)

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

/* -------- LISTE DINAMICHE con malloc() --------------*/

void stampalista(lista* sentinella){
    lista* cur = sentinella->next;
    printf("[");
    while(cur != NULL){
        printf(" (%d) ",cur->p.id);
        /*printf(" (%d|%c|%s - %d) ", cur->p.id, cur->p.tipo, cur->p.body, (int)cur->sentime.tv_sec);*/
        cur = cur->next;
    }
    printf("]\n");

}

void aggiungi( lista* sentinella, packet p ){
	/* printf("[%d|%c|%s]\n", p.id, p.tipo, p.body); */
    lista* new;
    lista* cur = sentinella;
    while(cur->next != NULL){
        cur = cur->next;
    }
    new = malloc((size_t)sizeof(lista));
    if(new == NULL){
		printf("malloc() failed\n");
		exit(1);
	}
	if(gettimeofday(&(new->sentime), NULL)) {
		printf ("gettimeofday() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
	}
    memcpy(&(new->p), &p, sizeof(packet));
    new->next = NULL;
    cur->next = new;

}

lista pop(lista* sentinella){
    lista* todel;
    lista ret;
    memset(&ret, 0, sizeof(lista));
    if(sentinella->next == NULL)
        return ret;
    todel = sentinella->next;
    sentinella->next = todel->next;
    memcpy(&ret, todel, sizeof(lista));
    free(todel);
    return ret;
}

lista rimuovi(lista* sentinella, uint32_t id){
	lista* cur = sentinella;
	lista* todel;
	lista ret;
	while(cur->next != NULL){
		if(cur->next->p.id == id){
			todel = cur->next;
			cur->next = cur->next->next;
			memcpy(&ret, todel, sizeof(lista));
			free(todel);
			return ret;
		}
		cur = cur->next;
	}
	printf("rimuovi(): pacchetto non trovato\n");
	exit(1);
}
/* -------------- SOCKET, funzioni ricorrenti -----------------*/

int get_socket(int type){
    /* SOCK_STREAM = TCP
     * SOCK_DGRAM  = UDP
     */
   int socketfd;
    printf ("socket()\n");
    socketfd = socket(AF_INET, type, 0);
    if (socketfd == -1) {
        printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
        exit(1);
    }
    return socketfd;
}

void sock_opt_reuseaddr(int socketfd){
    /* avoid EADDRINUSE error on bind() */
    int OptVal = 1;
    int ris;
    printf ("setsockopt()\n");
    ris = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&OptVal, sizeof(OptVal));
    if (ris == -1) {
        printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
        exit(1);
    }
}

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

void sock_bind(int socketfd, struct sockaddr_in* Local){
    int ris;
    printf ("bind()\n");
    ris = bind(socketfd, (struct sockaddr*) Local, sizeof(*Local));
    if (ris == -1)  {
        printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
}

void sock_connect(int sock, struct sockaddr_in *opt){
    /* connection request */
    int ris;
    printf ("connect()\n");
    ris = connect(sock, (struct sockaddr*) opt, sizeof(*opt));
    if (ris == -1)  {
        printf ("connect() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
}

void sock_listen(int sock){
    int ris;
    printf ("listen()\n");
    ris = listen(sock, 10 );
    if (ris == -1) {
        printf ("listen() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
}

int sock_accept(int socketfd, struct sockaddr_in *opt){
    int len, newsocketfd;
    memset (opt, 0, sizeof(*opt) );
    /* wait for connection request */
    len=sizeof(*opt);
    printf ("accept()\n");
    newsocketfd = accept(socketfd, (struct sockaddr*) opt, (socklen_t *)&len);
    if (newsocketfd == -1)  {
        printf ("accept() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
    return newsocketfd;
}

int TCP_connection_send(const char *remote_ip, int remote_port){

    int tcp_sock;
    struct sockaddr_in local, server;

    tcp_sock = get_socket(SOCK_STREAM);

    sock_opt_reuseaddr(tcp_sock);

    name_socket(&local, htonl(INADDR_ANY), 0);

    sock_bind(tcp_sock, &local);

    name_socket(&server, inet_addr(remote_ip), remote_port);

    sock_connect(tcp_sock, &server);

    printf ("connessione avvenuta\n");
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

    printf("connection from %s : %d\n",
		inet_ntoa(Cli.sin_addr),
		ntohs(Cli.sin_port)
    );

    return newsocketfd;
}

int UDP_sock(int local_port){
    int sock;
    struct sockaddr_in local;

    sock = get_socket(SOCK_DGRAM);
    sock_opt_reuseaddr(sock);
    name_socket(&local, htonl(INADDR_ANY), local_port);
    sock_bind(sock,&local);

    return sock;
}
