/*
    sender.c - Trasferisce un file attraverso una connessione TCP

    usage:

    sender.exe IP_ADDRESS PORT FILEPATH
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
#include <sys/time.h>

#include "utils.h"

int main(int argc, char *argv[])
{
    char string_remote_ip_address[100];
    char filename[100];
    int remote_port_number;
    int socketfd;

    char buf[BUFSIZE];

    ssize_t nread = 0;
	ssize_t nwrite = 0;

    int to_transfer;

	int i = 0;

	remote_port_number = 59000;
	strcpy(string_remote_ip_address, "127.0.0.1");

    /*
    if(argc!=3) {
        printf ("necessari 3 parametrii: REMOTE_IP FILE\n");
        exit(1);
    } else {
        strncpy(string_remote_ip_address, argv[1], 99);
        strncpy(filename, argv[2], 99);
    }
    */

    /* Open the file to be transfered in READ MODE */
    /*
    to_transfer = open(filename, O_RDONLY);
    if (to_transfer == -1) {
        printf("open() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
	*/

    socketfd = TCP_connection_send(string_remote_ip_address, remote_port_number);


    /* Trasferimento del file */
    for(i=0;i<=100;i++){

        printf("%d\n", i);

		sprintf(buf, "%d\n", i);

		nwrite = write(socketfd, buf, strlen(buf)+1);

		if (nwrite == -1){
			printf ("write() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        	exit(1);
   		}
	}

    /* chiusura */
    close(socketfd);

    printf("Connessione TCP chiusa\n");

    return(0);
}

