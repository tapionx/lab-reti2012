#include <stdio.h>
#include <stdlib.h>

typedef struct lista{
    struct lista* next;
    int value;
} lista;

void stampalista(lista* sentinella){
    lista* cur = sentinella->next;
    printf("[");
    while(cur != NULL){
        printf(" %d ",cur->value);
        cur = cur->next;
    }
    printf("]\n");
    
}

void aggiungi( lista* sentinella, int valore ){
    lista* cur = sentinella;
    while(cur->next != NULL){
        cur = cur->next;
    }
    lista* new = malloc((size_t)sizeof(lista));
    new->value = valore;
    new->next = NULL;
    cur->next = new;
    
    //return new;
}

void pop(lista* sentinella){
    if(sentinella->next == NULL)
        return;
    lista* todel = sentinella->next;
    sentinella->next = todel->next;
    free(todel);
}

int main(){

    lista sentinella;
    sentinella.next = NULL;
    
    int scelta;
    int valore;

    while(1){
        printf("\n\n >: ");
        scanf("%d %d", &scelta, &valore);
        switch(scelta){
        
            case 1:
                aggiungi(&sentinella,valore);
                break;
            
            case 2:
                pop(&sentinella);
                break;

            default:
                break;
    
        }

        stampalista(&sentinella);
    }

    return 0;

}
