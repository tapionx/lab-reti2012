/* SERVER UDP */

/* Aspetta su un socket UDP dei datagram con la select() */

/* Se arriva un datagram lo stampa e si mette di nuovo in attesa con la select() */

/* Se scade il timeout dice che é scaduto il timeout e si rimette in attesa con la select() */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOCKET_ERROR   ((int)-1)
#define SIZEBUF 100000L

int main(int argc, char *argv[]){

	struct sockaddr_in Local, From;
	char string_remote_ip_address[100];
	short int remote_port_number, local_port_number;
	int socketfd, OptVal, msglen, Fromlen, ris;
	char msg[SIZEBUF];

	if(argc!=2) { 
        printf ("necessario 1 parametri\n"); 
        exit(1);  
    } else {
		local_port_number = atoi(argv[1]);
	}

    /* SOCKET() */
	/* get a datagram socket */
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd == SOCKET_ERROR) {
		printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}

    /* SETSOCKOPT() ????? */
	/* avoid EADDRINUSE error on bind() */
	OptVal = 1;
	ris = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&OptVal, sizeof(OptVal));
	if (ris == SOCKET_ERROR)  {
		printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
	}

	/* IPv4 */
	Local.sin_family = AF_INET;
	/* INADDR_ANY = indirizzo IP del computer, impostato dal kernel */
	Local.sin_addr.s_addr = htonl(INADDR_ANY);
	Local.sin_port = htons(local_port_number);
	
    /* BIND() */
    ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
	if (ris == SOCKET_ERROR)  {
		printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}

	while(1)
    {

        /* RECVFROM() */
        Fromlen=sizeof(struct sockaddr_in);
        msglen = recvfrom ( socketfd, msg, (int)SIZEBUF, 0, (struct sockaddr*)&From, &Fromlen);
        if (msglen<0) {
            char msgerror[1024];
            sprintf(msgerror,"recvfrom() failed [err %d] ", errno);
            perror(msgerror);
        }
        sprintf((char*)string_remote_ip_address,"%s",inet_ntoa(From.sin_addr));
        remote_port_number = ntohs(From.sin_port);
        printf("ricevuto  msg: \"%s\" len %d, from host %s, port %d\n",
               msg, msglen, string_remote_ip_address, remote_port_number);
	}

	return (0);
}
