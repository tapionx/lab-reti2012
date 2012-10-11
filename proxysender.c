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

#include "utils.h"


int main(int argc, char *argv[]){

    #define BUFSIZE 1024

    char buf[BUFSIZE];
    char remote_ip[100];
    int tcp_sock, udp_sock, nread, nwrite;
    int local_port, remote_port;

    struct sockaddr_in udp_to;

    if(argc < 4){
        printf("usage: LOCAL_PORT REMOTE_IP REMOTE_PORT\n");
        exit(1);
    }

    local_port = atoi(argv[1]);
    strncpy(remote_ip, argv[2], 99);
    remote_port = atoi(argv[3]);

    udp_sock = UDP_sock(htons(0));

    tcp_sock = TCP_connection_recv(local_port);

    memset(&udp_to, 0, sizeof(udp_to));
    udp_to.sin_family = AF_INET;
    udp_to.sin_addr.s_addr  = inet_addr(remote_ip);
    udp_to.sin_port = htons(remote_port);
    
    /* Transfer data */
    printf ("read()\n");
    while( (nread = read(tcp_sock, buf, BUFSIZE )) > 0) {

        printf("sendto()\n");

        printf("%s\n", buf);

        nwrite = sendto( udp_sock, 
                         buf, 
                         nread, 
                         0, 
                         (struct sockaddr*)&udp_to, 
                         (socklen_t )sizeof(udp_to)
                       );

        if(nwrite == -1){
            printf ("sendto() failed, Err: %d \"%s\"\n",errno,strerror(errno));
            exit(1);
        }
    }

    if(nread == -1) {
        printf ("read() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    printf("File trasferito correttamente\n");

    /* chiusura */
    printf ("close()\n");
    close(tcp_sock);
    close(udp_sock);

    return(0);
}
