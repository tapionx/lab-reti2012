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


#define SOCKET_ERROR   ((int)-1)
#define SIZEBUF 1000000

int main(int argc, char *argv[])
{
    #define BUFSIZE 1024
    struct sockaddr_in Local, Cli;
    short int local_port_number;
    int socketfd, newsocketfd, OptVal, len, ris;
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

    /* get a stream socket */
    printf ("socket()\n");
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == SOCKET_ERROR) {
        printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
        exit(1);
    }

    /* avoid EADDRINUSE error on bind() */
    OptVal = 1;
    printf ("setsockopt()\n");
    ris = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&OptVal, sizeof(OptVal));
    if (ris == SOCKET_ERROR)  {
        printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
        exit(1);
    }

    /* BIND */ 
    memset ( &Local, 0, sizeof(Local) );
    Local.sin_family		=	AF_INET;
    /* indicando INADDR_ANY viene collegato il socket all'indirizzo locale IP     */
    /* dell'interaccia di rete che verrà utilizzata per inoltrare il datagram IP  */
    Local.sin_addr.s_addr	=	htonl(INADDR_ANY);         /* wildcard */
    Local.sin_port		=	htons(local_port_number);
    printf ("bind()\n");
    ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
    if (ris == SOCKET_ERROR)  {
        printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    /* LISTEN */
    printf ("listen()\n");
    ris = listen(socketfd, 10 );
    if (ris == SOCKET_ERROR)  {
        printf ("listen() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
 
    /* Open the file to write */
    printf("open()\n");
    dest_file = open(local_filename, O_CREAT|O_WRONLY, S_IRWXU);
    if (dest_file == -1)  {
        printf ("open() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    /* wait for connection request */
    memset ( &Cli, 0, sizeof(Cli) );
    len=sizeof(Cli);
    printf ("accept()\n");
    newsocketfd = accept(socketfd, (struct sockaddr*) &Cli, (socklen_t *)&len);
    if (newsocketfd == SOCKET_ERROR)  {
        printf ("accept() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    printf("connection from %s : %d\n", 
        inet_ntoa(Cli.sin_addr), 
        ntohs(Cli.sin_port)
    );

    /* Transfer data */
    printf ("read()\n");
    while( (nread=read(newsocketfd, buf, BUFSIZE )) >0) {
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
    close(newsocketfd);
    close(socketfd);
    close(dest_file);

    return(0);
}

