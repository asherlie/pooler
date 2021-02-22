#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct func_arg{
    int _id;

    volatile void* (*func)(void*);
    void* arg;

    volatile _Bool spool_up;
    volatile _Bool exit;
};

struct thread{
    struct func_arg* f_a;
    pthread_t pth;
};


/* linked list containing threads
 */
struct thread_ll{
    struct thread* thread_info;
    /*
     * pthread_t thread;
     * struct func_arg* f_a;
     */
    struct thread_ll* next, * prev;
};

struct thread_ll* create_tll(struct thread* th){
    struct thread_ll* ret = malloc(sizeof(struct thread_ll));
    ret->thread_info = th;
    /* ret->thread = pth; */
    return ret;
}

/* inserts node into list *tll */
void insert_tll(struct thread_ll** tll, struct thread_ll* node){
    node->next = NULL;
    /* if(!(*tll))new_node = *tll = malloc(sizeof(struct thread_ll)); */
    /* if this is the first element */
    if(!(*tll)){
        *tll = node;
        (*tll)->prev = NULL;
        return;
    }
    struct thread_ll* prev_node;
    for(prev_node = *tll; prev_node->next; prev_node = prev_node->next);
    prev_node->next = node;
    node->prev = prev_node;
}

/* moves a tll node from one tll to another */
/*
 * i need back references to simplify this
 * this function will be used to move a thread from in_use/avail to avail/in_use
 */
/*
 * void swap_tll(struct thread_ll* a, struct thread_ll* b){
 * }
 */

struct routine_queue{
    /* this lock is used to ensure that messages are removed from the queue before */
    pthread_mutex_t lock;
    struct func_arg* fa, * base_ptr;
    int n_requests, cap;
};

void init_routine_queue(struct routine_queue* rq){
    pthread_mutex_init(&rq->lock, NULL);
    rq->cap = 200;
    rq->n_requests = 0;
    rq->base_ptr = rq->fa = malloc(sizeof(struct func_arg)*rq->cap);
}

void insert_routine_queue(struct routine_queue* rq, volatile void* (*func)(void*), void* arg){
    pthread_mutex_lock(&rq->lock);
    if(rq->n_requests == rq->cap){
        rq->cap *= 2;
        if(!rq->cap)rq->cap = 200;
        struct func_arg* tmp = malloc(sizeof(struct func_arg)*rq->cap);
        memcpy(tmp, rq->fa, sizeof(struct func_arg)*rq->n_requests);
        /* free(rq->fa); */
        free(rq->base_ptr);
        rq->base_ptr = rq->fa = tmp;
    }
    rq->fa[rq->n_requests].func = func;
    rq->fa[rq->n_requests++].arg = arg;
    pthread_mutex_unlock(&rq->lock);
}

struct func_arg* pop_routine_queue(struct routine_queue* rq){
    struct func_arg* ret;
    pthread_mutex_lock(&rq->lock);
    if(!rq->n_requests)ret = NULL;
    else{
        ret = rq->fa++;
        --rq->n_requests;
        --rq->cap;
    }
    pthread_mutex_unlock(&rq->lock);
    return ret;
}

#if !1
we need to move completed tasks into the available thread_ll
we need to select threads from the available thread_ll for each item in the queue
this will be done in two separate threads that compete with one another

or it could be done in one that just continuously looks for finished in_use threads before marking them as available
and setting their f_a->arg and func

but this kind of makes the entire linked list redundant
if we are just going through our list of threads what use is the structure

honestly i should just use both threads for practice lol

spooler thread, which finds available threads and immediately pops from routine_queue if possible
    then populates thread^s func, arg and moves to in_use
    NOTE: lock on the mutex lock to be sure nothing funky happens

schedule thread, which moves threads with !spool_up to available
#endif

/* spawns n_threads, each ready to be assigned */
struct thread_pool{
    int n_threads;

    pthread_mutex_t tll_lock;

    struct thread_ll* available, * in_use;

    struct routine_queue rq;
    /* routine queue structure */
    #if 0
    each request must contain func, arg
    it will not specify thread to run on, all threads are identical
    and will be selected by the scheduler thread
    #endif
};


/* waits until the scheduler attempts to join each thread using tryjoin */
/* the scheduler  */
void* scheduler(void* v_thread_pool){
    struct thread_pool* p = v_thread_pool;
    while(1){
        for(struct thread_ll* tll = p->in_use; tll; tll = tll->next){
            /* thread no longer executing a routine */
            if(!tll->thread_info->f_a->spool_up){
                pthread_mutex_lock(&p->tll_lock);
                tll->prev->next = tll->next;
                /* what if this is NULL */
                tll->prev = NULL;
                /* if(!p->available)p->available = tll; */
                if(p->available){
                    tll->next = p->available;
                    p->available->prev = tll;
                    tll->prev = NULL;
                }
                p->available = tll;

                pthread_mutex_unlock(&p->tll_lock);
            }
        }
    }
    return NULL;
}

void* spooler(void* v_thread_pool){
    return NULL;
}


void begin_thread_mgmt(struct thread_pool* p){
    pthread_t spool, schedule;
    pthread_create(&spool, NULL, spooler, p);
    pthread_create(&schedule, NULL, scheduler, p);
    pthread_detach(spool);
    pthread_detach(schedule);
}

void destroy_routine_queue(struct routine_queue* rq){
    free(rq->base_ptr);
    pthread_mutex_destroy(&rq->lock);
}


void* await_instructions(void* v_f_a){
    struct func_arg* f_a = v_f_a;
    printf("thread %i started up\n", f_a->_id);
    /* while(!f_a->func){ */
    while(!f_a->exit){
        while(!f_a->spool_up){
            if(f_a->exit)return NULL;
            usleep(100);
        }
        f_a->func(f_a->arg);
        f_a->spool_up = 0;
    }
    return NULL;
}

struct thread* spawn_thread(_Bool* success, int id){
    struct thread* ret = malloc(sizeof(struct thread));
    ret->f_a = malloc(sizeof(struct func_arg));
    ret->f_a->exit = 0;
    ret->f_a->spool_up = 0;
    ret->f_a->_id = id;
    /*ret->f_a->func =*/ ret->f_a->arg = NULL;

    #pragma GCC diagnostic ignored "-Wuninitialized"
    pthread_t pth;

    ret->pth = pth;
    _Bool failure = pthread_create(&ret->pth, NULL, await_instructions, ret->f_a);
    if(success)*success = !failure;

    return ret;
}

void init_pool(struct thread_pool* p, int n_threads){
    p->n_threads = n_threads;
    /* p->available = insert_tll(&); */
    p->available = p->in_use = NULL;
    struct thread* th;
    for(int i = 0; i < p->n_threads; ++i){
        /* pthread_attr_init(&pth); */
        th = spawn_thread(NULL, i);

        insert_tll(&p->available, create_tll(th));
    }

    pthread_mutex_init(&p->tll_lock, NULL);
    init_routine_queue(&p->rq);
}

/* we have two options - exec_routine() can be blocking, only returning when we have a spot
 * or we can have exec_routine() submit a exec request that is guaranteed to begin when a spot
 * is available
 */
void exec_routine(){
}

/* sets exit to 1 for each available thread, pthread_join */
void destroy_pool(struct thread_pool* p){
    for(struct thread_ll* tll = p->available; tll; tll = tll->next){
        tll->thread_info->f_a->exit = 1;
        pthread_join(tll->thread_info->pth, NULL);
        printf("closing available thread %i\n", tll->thread_info->f_a->_id);
    }
    for(struct thread_ll* tll = p->in_use; tll; tll = tll->next){
        /* tll->thread_info->f_a->exit = 1; */
        pthread_join(tll->thread_info->pth, NULL);
        printf("closing in_use thread %i\n", tll->thread_info->f_a->_id);
    }
    destroy_routine_queue(&p->rq);
    pthread_mutex_destroy(&p->tll_lock);
}

/*
 * maybe create a global variable and INIT_POOL()
 */

int main(){
    struct thread_pool p;
    init_pool(&p, 10);
    destroy_pool(&p);
}
