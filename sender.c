/* 

    sendfile.c - Trasferisce un file attraverso una connessione TCP

    usage:

    sendfile.exe IP_ADDRESS PORT FILEPATH

*/
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

#define SOCKET_ERROR   ((int)-1)
#define SIZEBUF 1000000


int main(int argc, char *argv[])
{
    #define BUFSIZE 1024 
    char string_remote_ip_address[100];
    char filename[100];
    short int remote_port_number;
    int socketfd;
	
    char buf[BUFSIZE];

    ssize_t nread = 0;	
	ssize_t nwrite = 0;

    int to_transfer;

    if(argc!=4) { 
        printf ("necessari 3 parametrii: REMOTE_IP REMOTE_PORT FILE\n");  
        exit(1);  
    } else {
        strncpy(string_remote_ip_address, argv[1], 99);
        remote_port_number = atoi(argv[2]);
        strncpy(filename, argv[3], 99);
    }

    /* Open the file to be transfered in READ MODE */
    to_transfer = open(filename, O_RDONLY);
    if (to_transfer == -1) {
        printf("open() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    socketfd = TCP_connection_send(string_remote_ip_address, remote_port_number);

    printf("inizio trasferimento file\n");

    /* Trasferimento del file */
    while( (nread = read(to_transfer, buf, BUFSIZE)) > 0) {
	
        printf("%s\n", buf);
	
		nwrite = write(socketfd, buf, nread);
		
		if (nwrite == -1){
			printf ("write() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        	exit(1);
   		}
	}

	if (nread == -1){
		 printf ("read() failed, Err: %d \"%s\"\n",errno,strerror(errno));
         exit(1);
	} else {
		printf ("Trasferimento completato\n");
	}

    /* chiusura */
    close(socketfd);
	close(to_transfer);

    printf("Connessione TCP chiusa\n");

    return(0);
}

