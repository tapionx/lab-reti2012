/* 
    Proxy Receiver - riceve un flusso di dati UDP e lo trasmette in TCP
    al receiver garantendo la correttezza della trasmissione
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
    int tcp_sock, udp_sock, nread, nwrite, len;
    int local_port, remote_port;

    struct sockaddr_in udp_from;

    if(argc < 4){
        printf("usage: LOCAL_PORT REMOTE_IP REMOTE_PORT\n");
        exit(1);
    }

    local_port = atoi(argv[1]);
    strncpy(remote_ip, argv[2], 99);
    remote_port = atoi(argv[3]);

    udp_sock = UDP_sock(local_port);

    tcp_sock = TCP_connection_send(remote_ip, remote_port);

    memset(&udp_from, 0, sizeof(udp_from));
    
    /* Transfer data */
    printf ("read()\n");
    len = sizeof(udp_from);
    while( (nread = recvfrom( udp_sock, 
                              buf, 
                              BUFSIZE, 
                              0, 
                              (struct sockaddr*) &udp_from,
                              (socklen_t*)&len
                            ) > 0)) {
       
        printf("write()\n");

        printf("%s\n", buf);

        nwrite = write( tcp_sock, 
                        buf, 
                        nread 
                       );

        if(nwrite == -1){
            printf ("write() failed, Err: %d \"%s\"\n",errno,strerror(errno));
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
