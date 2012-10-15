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

#include <sys/select.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 50

#define MAXSIZE 65000/*65536*/

#define HEADERSIZE 5

#define BODYSIZE (MAXSIZE) - (HEADERSIZE)

typedef struct packet{
	uint32_t id;
	char tipo;
	char body[BODYSIZE];
} packet;

typedef struct lista{
    struct lista* next;
    packet p;
} lista;

void stampalista(lista* sentinella){
    lista* cur = sentinella->next;
    printf("[");
    while(cur != NULL){
        printf(" %d ",cur->p.id);
        cur = cur->next;
    }
    printf("]\n");

}

void aggiungi( lista* sentinella, packet p ){
    lista* new;
    lista* cur = sentinella;
    while(cur->next != NULL){
        cur = cur->next;
    }
    new = malloc((size_t)sizeof(lista));
    if(new == NULL){
		printf("malloc() failed\n");
		exit(1);
	}
    memcpy(&new->p, &p, sizeof(packet));
    new->next = NULL;
    cur->next = new;
}

void pop(lista* sentinella){
    lista* todel;
    if(sentinella->next == NULL)
        return;
    todel = sentinella->next;
    sentinella->next = todel->next;
    free(todel);
}

void rimuovi(lista* sentinella, int id){
	lista* cur = sentinella;
	if(cur->next == NULL)

	while(cur->next->p.id != id){
		cur = cur->next;
	}
	cur->next = cur->next->next;
	free(cur->next);
}

int main(){

    lista sentinella;

    int scelta;
    uint32_t valore;

	sentinella.next = NULL;

	packet

    while(1){
        printf("\n\n >: ");
        scanf("%d %d", &scelta, (int*)&valore);
        switch(scelta){

            case 1:
                aggiungi(&sentinella,valore);
                break;

            case 2:
                pop(&sentinella);
                break;

			case 3:
				rimuovi(&sentinella, valore);
				break;
            default:
                break;

        }

        stampalista(&sentinella);
    }

    return 0;

}
