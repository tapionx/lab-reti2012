/* 
    Proxy Sender - riceve un flusso di dati TCP e lo trasmette in UDP
    attraverso il ritardatore garantendo la correttezza della trasmissione
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



int main(int argc, char *argv[]){

    #define BUFSIZE 1024

    char buf[BUFSIZE];
    char remote_ip[100], local_ip[100];
    int tcp_sock, udp_sock, ris, len, tcp_client_sock, nread, nwrite;
    int local_port, remote_port;
    int OptVal;

    struct sockaddr_in tcp_recv, udp_send, udp_to;

    if(argc < 4){
        printf("usage: LOCAL_PORT REMOTE_IP REMOTE_PORT\n");
        exit(1);
    }

    local_port = atoi(argv[1]);
    strncpy(remote_ip, argv[2], 99);
    remote_port = atoi(argv[3]);

    /* SOCKET */
    tcp_sock = socket(AF_INET, SOCK_STREAM,  0);
    if(tcp_sock == -1){
        printf ("socket() failed, Err: %d \"%s\"\n", errno, strerror(errno));
        exit(1);
    }

    udp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(udp_sock == -1){
        printf ("socket() failed, Err: %d \"%s\"\n", errno, strerror(errno));
        exit(1);
    }
    
    /* OPTIONS */
    
    /* avoid EADDRINUSE error on bind() */
    OptVal = 1;
    printf ("setsockopt()\n");
    ris = setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&OptVal, sizeof(OptVal));
    if (ris == -1)  {
        printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
        exit(1);
    }
 
    OptVal = 1;
    printf ("setsockopt()\n");
    ris = setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&OptVal, sizeof(OptVal));
    if (ris == -1)  {
        printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
        exit(1);
    }

    /* BIND */
    memset(&tcp_recv, 0, sizeof(tcp_recv));
    tcp_recv.sin_family = AF_INET;
    tcp_recv.sin_addr.s_addr = inet_addr(local_ip);
    tcp_recv.sin_port = htons(local_port);
    printf("bind()\n");
    ris = bind(tcp_sock, (struct sockaddr*) &tcp_recv, sizeof(tcp_recv));
    if(ris == -1) {
        printf ("bind() failed, Err: %d \"%s\"\n", errno, strerror(errno));
        exit(1);
    }
    
    memset(&udp_send, 0, sizeof(udp_send));
    udp_send.sin_family = AF_INET;
    udp_send.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_send.sin_port = htons(0);
    printf("bind()\n");
    ris = bind(udp_sock, (struct sockaddr*) &udp_send, sizeof(udp_send));
    if(ris == -1) {
        printf ("bind() failed, Err: %d \"%s\"\n", errno, strerror(errno));
        exit(1);
    }

    memset(&udp_to, 0, sizeof(udp_to));
    udp_to.sin_family = AF_INET;
    udp_to.sin_addr.s_addr  = inet_addr(remote_ip);
    udp_to.sin_port = htons(remote_port);
    
    /* LISTEN */
    printf ("listen()\n");
    ris = listen(tcp_sock, 10 );
    if (ris == -1)  {
        printf ("listen() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    /* ACCEPT */
    len = sizeof(tcp_recv);
    printf ("accept()\n");
    tcp_client_sock = accept(tcp_sock, (struct sockaddr*) &tcp_recv, (socklen_t *)&len);
    if (tcp_client_sock == -1)  {
        printf ("accept() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    printf("connection from %s : %d\n",
        inet_ntoa(tcp_recv.sin_addr),
        ntohs(tcp_recv.sin_port)
    );

    /* Transfer data */
    printf ("read()\n");
    while( (nread=read(tcp_client_sock, buf, BUFSIZE )) > 0) {
        nwrite = write(udp_sock, buf, nread);
        if(nwrite == -1){
            printf ("write() failed, Err: %d \"%s\"\n",errno,strerror(errno));
            exit(1);
        }
        if(nwrite != nread){
            printf(" nread != nwrite O.o \n");
        }
    }

    if(nread == -1) {
        printf ("read() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    printf("File trasferito correttamente\n");

    /* chiusura */
    printf ("close()\n");
    close(tcp_client_sock);
    close(tcp_sock);
    close(udp_sock);

    return(0);
}
