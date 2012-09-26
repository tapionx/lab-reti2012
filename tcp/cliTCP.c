/* cliTCP.c    spedisce stringa, riceve stringa traslata
   su SunOS compilare con gcc -o cliTCP.c -lsocket -lnsl cliTCP.c
   su linux               gcc -o cliTCP cliTCP.c                   

   eseguire ad esempio su 137.204.72.49 lanciando la seguente riga di comandi
   cliTCP 130.136.2.7 5001 
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

#define SOCKET_ERROR   ((int)-1)
#define SIZEBUF 1000000


void usage(void) 
{  printf ("usage: cliTCP REMOTE_IP_NUMBER REMOTE_PORT_NUMBER\n");
   exit(1);
}

int main(int argc, char *argv[])
{
  #define MAXSIZE 1000000 
  struct sockaddr_in Local, Serv;
  char string_remote_ip_address[100];
  short int remote_port_number, local_port_number;
  int socketfd, OptVal, msglen, Fromlen, ris;
  int n, i, nread, nwrite, len;
  char buf[MAXSIZE];
  char msg[MAXSIZE];   // ="012345ABCD";

  for(i=0;i<MAXSIZE;i++) msg[i]='a';
  msg[MAXSIZE-1]='\0';
  

  if(argc!=3) { printf ("necessari 2 parametri\n"); usage(); exit(1);  }
  else {
    strncpy(string_remote_ip_address, argv[1], 99);
    remote_port_number = atoi(argv[2]);
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
  Local.sin_port	=	htons(0);
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

  /* connection request */
  printf ("connect()\n");
  ris = connect(socketfd, (struct sockaddr*) &Serv, sizeof(Serv));
  if (ris == SOCKET_ERROR)  {
    printf ("connect() failed, Err: %d \"%s\"\n",errno,strerror(errno));
    exit(1);
  }
  printf ("dopo connect()\n");
  fflush(stdout);


  /* scrittura */
  len = strlen(msg)+1;
  nwrite=0;
  printf ("write()\n");
  fflush(stdout);
  while( (n=write(socketfd, &(msg[nwrite]), len-nwrite)) >0 )
     nwrite+=n;
  if(n<0) {
    char msgerror[1024];
    sprintf(msgerror,"write() failed [err %d] ",errno);
    perror(msgerror);
    fflush(stdout);
    return(1);
  }

  /* lettura */
  nread=0;
  printf ("read()\n");
  fflush(stdout);
  while( (len>nread) && ((n=read(socketfd, &(buf[nread]), len-nread )) >0))
  {
     nread+=n;
     printf("read effettuata, risultato n=%d  len=%d nread=%d\
len-nread=%d\n", n, len, nread, len-nread );
     fflush(stdout);

  }
  if(n<0) {
    char msgerror[1024];
    sprintf(msgerror,"read() failed [err %d] ",errno);
    perror(msgerror);
    fflush(stdout);
    return(1);
  }
  
  /* stampa risultato 
  printf("stringa traslata: %s\n", buf);
  fflush(stdout);
  */


  /* chiusura */
  close(socketfd);

  return(0);
}

