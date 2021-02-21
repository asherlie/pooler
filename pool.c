#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* linked list containing threads
 */
struct thread_ll{
    pthread_t thread;
    struct thread_ll* next;
};

struct thread_ll* create_tll(pthread_t pth){
    struct thread_ll* ret = malloc(sizeof(struct thread_ll));
    ret->thread = pth;
    return ret;
}

/* inserts node into list *tll */
void insert_tll(struct thread_ll** tll, struct thread_ll* node){
    node->next = NULL;
    /* if(!(*tll))new_node = *tll = malloc(sizeof(struct thread_ll)); */
    if(!(*tll)){
        *tll = node;
        return;
    }
    struct thread_ll* prev_node;
    for(prev_node = *tll; prev_node->next; prev_node = prev_node->next);
    prev_node->next = node;
}

/* spawns n_threads, each ready to be assigned */
struct thread_pool{
    int n_threads;

    struct thread_ll* available, * in_use;
};

void init_pool(struct thread_pool* p, int n_threads){
    p->n_threads = n_threads;
    /* p->available = insert_tll(&); */
    for(int i = 0; i < p->n_threads; ++i){
        pthread_t pth;
        insert_tll(&p->available, create_tll(pth));
    }
    p->in_use = NULL;
}

void spool_up(struct thread_pool* p){
}

/*
 * maybe create a global variable and INIT_POOL()
 */

int main(){
}
