/*
    receivefile.c - receive a file through a TCP connection

    usage:
    receivefile.exe LOCAL_PORT FILE_NAME
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
    short int local_port_number;
    int socketfd;
    char buf[BUFSIZE];

    char local_filename[100];

    int dest_file;

    ssize_t nwrite = 0;
    ssize_t nread  = 0;

    if(argc != 3) { 
        printf ("necessari 2 parametri: LOCAL_PORT FILE_NAME\n");
        exit(1);  
    } else {
        local_port_number = atoi(argv[1]);
        strncpy(local_filename, argv[2], 99);
    }

    /* Open the file to write */
    printf("open()\n");
    dest_file = open(local_filename, O_CREAT|O_WRONLY, S_IRWXU);
    if (dest_file == -1)  {
        printf ("open() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    socketfd = TCP_connection_recv(local_port_number);

    /* Transfer data */
    printf ("read()\n");
    while( (nread=read(socketfd, buf, BUFSIZE )) >0) {
        nwrite = write(dest_file, buf, nread);
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

    printf("File letto correttamente\n");        
  
    /* chiusura */
    printf ("close()\n");
    close(socketfd);
    close(dest_file);

    return(0);
}

