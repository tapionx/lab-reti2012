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
    struct timeval sentime;
} lista;


void stampalista(lista* sentinella){
    lista* cur = sentinella->next;
    printf("[");
    while(cur != NULL){
        /*printf(" %d ",cur->p.id);*/
        printf(" (%d|%c|%s - %d) ", cur->p.id, cur->p.tipo, cur->p.body, (int)cur->sentime.tv_sec);
        cur = cur->next;
    }
    printf("]\n");

}

void aggiungi( lista* sentinella, packet p ){
	/* printf("[%d|%c|%s]\n", p.id, p.tipo, p.body); */
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
	if(gettimeofday(&(new->sentime), NULL)) {
		printf ("gettimeofday() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
	}
    memcpy(&(new->p), &p, sizeof(packet));
    new->next = NULL;
    cur->next = new;

}

void aggiungi_in_ordine( lista* sentinella, packet p ){
	/* printf("[%d|%c|%s]\n", p.id, p.tipo, p.body); */
    lista* new;
    lista* cur = sentinella;
    /* Se il paccketto è già stato spedito, lo scarto */
    if(cur->next != NULL && cur->next->p.id > p.id)
		return;
    while(cur->next != NULL && cur->next->p.id < p.id){
        cur = cur->next;
    }
    /* se il pacchetto esiste già lo scarto */
    if(cur->next != NULL && cur->next->p.id == p.id)
		return;
    new = malloc((size_t)sizeof(lista));
    if(new == NULL){
		printf("malloc() failed\n");
		exit(1);
	}
	if(gettimeofday(&(new->sentime), NULL)) {
		printf ("gettimeofday() failed, Err: %d \"%s\"\n",errno,strerror(errno));
        exit(1);
	}
    memcpy(&(new->p), &p, sizeof(packet));
    new->next = cur->next;
    cur->next = new;
}

lista pop(lista* sentinella){
    lista* todel;
    lista ret;
    memset(&ret, 0, sizeof(lista));
    if(sentinella->next == NULL)
        return ret;
    todel = sentinella->next;
    sentinella->next = todel->next;
    memcpy(&ret, todel, sizeof(lista));
    free(todel);
    return ret;
}

void rimuovi(lista* sentinella, uint32_t id){
	lista* cur = sentinella;
	lista* todel;
	while(cur->next != NULL){
		if(cur->next->p.id == id){
			todel = cur->next;
			cur->next = cur->next->next;
			free(todel);
			return;
		}
		cur = cur->next;
	}
}
int main(){

    lista sentinella;
	packet a;
	lista b;
    int scelta;
	a.tipo = 'B';
	strcpy(a.body, "ciao");
	sentinella.next = NULL;

    while(1){
        printf("\n\n >: ");
        scanf("%d %d", &scelta, (int*)&(a.id));
        switch(scelta){

            case 1:
                aggiungi(&sentinella, a);
                break;

            case 2:
                b = pop(&sentinella);
                printf("rimosso (%d|%c|%s - %d)\n", b.p.id, b.p.tipo, b.p.body, (int)b.sentime.tv_sec);
                break;

			case 3:
				rimuovi(&sentinella, a.id);
				break;

			case 4:
				aggiungi_in_ordine(&sentinella, a);
				break;
            default:
                break;

        }

        stampalista(&sentinella);
    }

    return 0;

}
