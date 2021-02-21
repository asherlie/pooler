#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

struct func_arg{
    volatile void* (*func)(void*);
    void* arg;

    volatile _Bool spool_up;
    volatile _Bool exit;
};

/* linked list containing threads
 */
struct thread_ll{
    pthread_t thread;
    struct func_arg* f_a;
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


void* await_instructions(void* v_f_a){
    struct func_arg* f_a = v_f_a;
    /* while(!f_a->func){ */
    while(!f_a->spool_up){
        if(f_a->exit)return NULL;
        usleep(100);
    }
    f_a->func(f_a->arg);
    return NULL;
}

struct thread{
    struct func_arg* f_a;
    pthread_t pth;
};

struct thread* spawn_thread(){
    struct thread* ret = malloc(sizeof(struct thread));
    ret->f_a = malloc(sizeof(struct func_arg));
    ret->f_a->exit = 0;
    ret->f_a->spool_up = 0;
    /*ret->f_a->func =*/ ret->f_a->arg = NULL;

    pthread_t pth;

    ret->pth = pth;

    pthread_create(&ret->pth, NULL, await_instructions, ret->f_a);

    return ret;
}

void init_pool(struct thread_pool* p, int n_threads){
    p->n_threads = n_threads;
    /* p->available = insert_tll(&); */
    for(int i = 0; i < p->n_threads; ++i){
        pthread_t pth;
        /* pthread_attr_init(&pth); */
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
