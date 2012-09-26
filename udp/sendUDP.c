/* sendUDP.c   spedisce datagram UDP  unicast 
   su SunOS compilare con gcc -o sendUDP -lsocket -lnsl sendUDP.c
   su linux               gcc -o sendUDP sendUDP.c                  */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define SOCKET_ERROR   ((int)-1)

void usage(void) 
{  printf ("usage: sendUDP MESSAGE REMOTE_IP_NUMBER REMOTE_PORT [LOCAL_IP_NUMBER]\n");
   exit(1);
}

int main(int argc, char *argv[])
{
  struct sockaddr_in Local, To;
  char string_remote_ip_address[100];
  char string_local_ip_address[100];
  short int remote_port_number;
  int socketfd, OptVal, addr_size;
  int ris;

  if((argc!=4)&&(argc!=5)) { printf ("necessari 3 o 4 parametri\n"); usage(); exit(1);  }
  else {
    strncpy(string_remote_ip_address, argv[2], 99);
    remote_port_number = atoi(argv[3]);
    if(argc==5)
    	strncpy(string_local_ip_address, argv[4], 99);
  }

  /* get a datagram socket */
  socketfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socketfd == SOCKET_ERROR) {
    printf ("socket() failed, Err: %d \"%s\"\n", errno,strerror(errno));
    exit(1);
  }

  /* avoid EADDRINUSE error on bind() */ 
  OptVal = 1;
  ris = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (char *)&OptVal, sizeof(OptVal));
  if (ris == SOCKET_ERROR)  {
    printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
    exit(1);
  }

  /* name the socket */
  Local.sin_family		=	AF_INET;
  if(argc==5)
  	Local.sin_addr.s_addr  =	inet_addr(string_local_ip_address);
  else
  	/* si lega il socket all'indirizzo IP dell'interfaccia che verrà usata per inoltrare il datagram IP  */
  	Local.sin_addr.s_addr	=	htonl(INADDR_ANY);
  /* 0 come porta, il s.o. decide lui  a quale porta UDP collegare il socket */
  Local.sin_port = htons(0); /* il s.o. decide la porta locale */
  ris = bind(socketfd, (struct sockaddr*) &Local, sizeof(Local));
  if (ris == SOCKET_ERROR)  {
    printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
    exit(1);
  }

  /* assign our destination address */
  To.sin_family	 =	AF_INET;
  To.sin_addr.s_addr  =	inet_addr(string_remote_ip_address);
  To.sin_port		 =	htons(remote_port_number);

  addr_size = sizeof(struct sockaddr_in);
  /* send to the address */
  ris = sendto(socketfd, argv[1], strlen(argv[1])+1 , 0, (struct sockaddr*)&To, addr_size);
  if (ris < 0) {
    printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
    exit(1);
  }
  else
     printf("datagram UDP \"%s\" sent to (%s:%d)\n",
             argv[1], string_remote_ip_address, remote_port_number);

}


