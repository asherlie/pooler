#include <stdio.h>
#include <pthread.h>

/* linked list containing threads
 */
struct thread_ll{
    pthread_t thread;
    struct thread_ll* next;
};

/* spawns n_threads, each ready to be assigned */
struct thread_pool{
    int n_threads;

    struct thread_ll* available, * in_use;
};

void init_pool(struct thread_pool* p){
}

void spool_up(struct thread_pool* p){
}

/*
 * maybe create a global variable and INIT_POOL()
 */

int main(){
}
