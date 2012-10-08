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

#define SOCKET_ERROR   ((int)-1)
#define SIZEBUF 1000000


int main(int argc, char *argv[])
{
    #define BUFSIZE 1024 
    struct sockaddr_in Local, Serv;
    char string_remote_ip_address[100];
    char filename[100];
    short int remote_port_number;
    int socketfd, OptVal, ris;
	
    char buf[BUFSIZE];

    ssize_t nread = 0;	
	ssize_t nwrite = 0;

    int to_transfer;

    if(argc!=4) { 
        printf ("necessari 2 parametrii: IP PORT FILE\n");  
        exit(1);  
    } else {
        strncpy(string_remote_ip_address, argv[1], 99);
        remote_port_number = atoi(argv[2]);
        strncpy(filename, argv[3], 99);
    }

    /* get a datagram socket */
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

    /* name the socket */
    memset ( &Local, 0, sizeof(Local) );
    Local.sin_family		=	AF_INET;
    /* indicando INADDR_ANY viene collegato il socket all'indirizzo locale IP     */
    /* dell'interaccia di rete che verrà utilizzata per inoltrare i dati          */
    Local.sin_addr.s_addr	=	htonl(INADDR_ANY);         /* wildcard */
    Local.sin_port	=	htons(59000);
    printf ("bind()\n");
    ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
    if (ris == SOCKET_ERROR)  {
        printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    /* assign our destination address */
    memset ( &Serv, 0, sizeof(Serv) );
    Serv.sin_family	 =	AF_INET;
    Serv.sin_addr.s_addr  =	inet_addr(string_remote_ip_address);
    Serv.sin_port		 =	htons(remote_port_number);

    /* Open the file to be transfered in READ MODE */
    to_transfer = open(filename, O_RDONLY);
    if (to_transfer == -1) {
        printf("open() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }

    /* connection request */
    printf ("connect()\n");
    ris = connect(socketfd, (struct sockaddr*) &Serv, sizeof(Serv));
    if (ris == SOCKET_ERROR)  {
        printf ("connect() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
    }
    printf ("connessione TCP avvenuta\n");
    fflush(stdout);

    printf("inizio trasferimento file\n");

    /* Trasferimento del file */
    while( (nread = read(to_transfer, buf, BUFSIZE)) > 0) {
		
		nwrite = write(socketfd, buf, nread);
		
		if (nwrite == -1)
		{
			printf ("write() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        	exit(1);
   		}
		
		if (nread != nwrite)
		{
			printf ("write() failed, Err: Not all the data has been sent");
        	exit(1);
		}

	}

	if (nread == -1)
	{
		 printf ("read() failed, Err: %d \"%s\"\n",errno,strerror(errno));
         exit(1);
	}
	else
	{
		printf ("Trasferimento completato\n");
	}

    /* chiusura */
    close(socketfd);
	close(to_transfer);

    printf("Connessione TCP chiusa\n");

    return(0);
}

