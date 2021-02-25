#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "pool.h"

/*extern*/ struct thread_pool POOL;

/*
 * oops - this creates a node - misnamed
 * given 
 */
/* struct thread_ll* create_tll(struct thread* th){ */
struct thread_node* create_tn(struct thread* th){
    /* struct thread_ll* ret = malloc(sizeof(struct thread_ll)); */
    struct thread_node* ret = malloc(sizeof(struct thread_node));
    /* ret->thread_info = th; */
    ret->next = ret->prev = NULL;
    ret->thread_info = th;
    /* ret->n_threads = 0; */
    /* ret->thread = pth; */
    return ret;
}

/* inserts node into list *tll */
/* void insert_tll(struct thread_ll* tll, struct thread_ll* node){ */
void insert_tll(struct thread_ll* tll, struct thread_node* node){
    node->next = NULL;
    ++tll->n_threads;
    if(!tll->first){
        node->prev = NULL;
        tll->last = tll->first = node;
        return;
    }
    tll->last->next = node;
    node->prev = tll->last;
    tll->last = node;
}

void init_routine_queue(struct routine_queue* rq){
    pthread_mutex_init(&rq->lock, NULL);
    rq->cap = 200;
    rq->n_requests = 0;
    rq->base_ptr = rq->fa = malloc(sizeof(struct func_arg)*rq->cap);
}

int insert_routine_queue(struct routine_queue* rq, volatile void* (*func)(void*), void* arg){
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
    int ret = rq->n_requests;
    pthread_mutex_unlock(&rq->lock);
    return ret;
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
void print_tll(struct thread_ll* tll){
    for(struct thread_node* n = tll->first; n; n = n->next){
        printf("  %i\n", n->thread_info->f_a->_id);
    }
}

void print_threads(struct thread_pool* p){
    puts("available:");
    print_tll(p->available);
    puts("in_use:");
    print_tll(p->in_use);
}

/* struct thread_ll* pop_tll(struct thread_ll** t){ */
struct thread_node* remove_node(struct thread_ll* t, struct thread_node* n){
    if(n == t->first){
        t->first = n->next;
        if(t->first)t->first->prev = NULL;
        --t->n_threads;
        return n;
    }
    if(n == t->last){
        t->last = n->prev;
        n->prev->next = NULL;
        --t->n_threads;
        return n;
    }
    /* internal node */
    n->prev->next = n->next;
    n->next->prev = n->prev;

    --t->n_threads;
    return n;
}
struct thread_node* pop_tll(struct thread_ll* t){
    struct thread_node* n = t->first;
    if(n){
        /* updating tll pointer */
        t->first = n->next;

        /* all n's pointers should be NULL to prepare
         * for entry in a new tll
         */
        n->next = n->prev = NULL;
        --t->n_threads;
    }
    return n;

    #if 0
    if((*t)->prev)(*t)->prev = (*t)->next;
    if((*t)->next)(*t)->next->prev = (*t)->prev;
    struct thread_ll* ret = *t;
    /* if this is the base pointer */
    if(!(*t)->prev){
        if(!(*t)->next)*t = NULL;
        else *t = (*t)->next;
    }
    /* if(!(*t)->prev && !(*t)->next)*t = NULL; */
    ret->prev = ret->next = NULL;
    return ret;
    /* struct thread_ll* ret = p->; */
    #endif
}

void prepend_tll(struct thread_ll* target, struct thread_node* n){
    /* n->prev, n->next == NULL */
    if(!target->first){
        target->first = target->last = n;
        return;
    }
    target->last->next = n;
    n->prev = target->last;
    target->last = n;
    target->last->next = NULL;
    /* target- */
    #if 0
    if(*target){
        (*target)->prev = t;
        t->next = *target;
    }
    *target = t;
    #endif
}

void* scheduler(void* v_thread_pool){
    struct thread_pool* p = v_thread_pool;
    
    while(1){
        /*
         * puts("before sched:");
         * print_threads(p);
         */
        for(struct thread_node* n = p->in_use->first; n; n = n->next){
            if(!n->thread_info->f_a->spool_up){
                pthread_mutex_lock(&p->tll_lock);
                struct thread_node* new_avail = remove_node(p->in_use, n);
                new_avail->next = new_avail->prev = NULL;
                prepend_tll(p->available, new_avail);
                /* printf("thread %i has been made available\n", new_avail->thread_info->f_a->_id); */
                pthread_mutex_unlock(&p->tll_lock);
            }
        }
        DELAY;
        /*
         * puts("after sched:");
         * print_threads(p);
         */
    }
}

void* spooler(void* v_thread_pool){
    struct thread_pool* p = v_thread_pool;
    struct func_arg* fa;
    while(1){
        DELAY;
        if(!(fa = pop_routine_queue(&p->rq))){
            DELAY;
            continue;
        }

        while(!p->available->first)DELAY;

        pthread_mutex_lock(&p->tll_lock);
        /* printf("spooling up thread %i\n", p->available->first->thread_info->f_a->_id); */
        /* struct thread_ll* next_in_use = pop_tll(&p->available); */
        struct thread_node* next_in_use = remove_node(p->available, p->available->first);
        next_in_use->next = next_in_use->prev = NULL;
        prepend_tll(p->in_use, next_in_use);
        next_in_use->thread_info->f_a->arg = fa->arg;
        next_in_use->thread_info->f_a->func = fa->func;
        next_in_use->thread_info->f_a->spool_up = 1;
        pthread_mutex_unlock(&p->tll_lock);
        /*
         * usleep(1000000);
         * puts("spooler called");
         * print_threads(p);
         */
    }
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
    /* printf("thread %i started up\n", f_a->_id); */
    /* while(!f_a->func){ */
    while(!f_a->exit){
        while(!f_a->spool_up){
            if(f_a->exit)return NULL;
            DELAY;
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

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wuninitialized"
    pthread_t pth;

    ret->pth = pth;
    #pragma GCC diagnostic pop
    _Bool failure = pthread_create(&ret->pth, NULL, await_instructions, ret->f_a);
    if(success)*success = !failure;

    return ret;
}

void init_tll(struct thread_ll* tll){
    tll->n_threads = 0;
    tll->first = tll->last = NULL;
}

void init_pool(struct thread_pool* p, int n_threads){
    p->n_threads = n_threads;
    /* p->available = insert_tll(&); */
    p->available = malloc(sizeof(struct thread_ll));
    p->in_use = malloc(sizeof(struct thread_ll));

    init_tll(p->available);
    init_tll(p->in_use);

    /* p->in_use = NULL; */
    struct thread* th;
    for(int i = 0; i < p->n_threads; ++i){
        /* pthread_attr_init(&pth); */
        th = spawn_thread(NULL, i);

        /* insert_tll(&p->available, create_tll(th)); */
        insert_tll(p->available, create_tn(th));
    }

    pthread_mutex_init(&p->tll_lock, NULL);
    init_routine_queue(&p->rq);

    begin_thread_mgmt(p);
}

/* sets exit to 1 for each available thread, pthread_join */
void destroy_pool(struct thread_pool* p){
    for(struct thread_node* n = p->available->first; n; n = n->next){
        n->thread_info->f_a->exit = 1;
        pthread_join(n->thread_info->pth, NULL);
        /* printf("closing available thread %i\n", n->thread_info->f_a->_id); */
    }
    for(struct thread_node* n = p->in_use->first; n; n = n->next){
        n->thread_info->f_a->exit = 1;
        pthread_join(n->thread_info->pth, NULL);
        /* printf("closing available thread %i\n", n->thread_info->f_a->_id); */
    }
    destroy_routine_queue(&p->rq);
    pthread_mutex_destroy(&p->tll_lock);
    free(p->available);
    free(p->in_use);
}

int exec_routine(struct thread_pool* p, volatile void* (*func)(void*), void* arg){
    return insert_routine_queue(&p->rq, func, arg);
}

/* these three functions operate on the global POOL in order to simplify the usage of this library */
void start_pool(int n_threads){
    init_pool(&POOL, n_threads);
}

void end_pool(){
    destroy_pool(&POOL);
}

void exec_pool(volatile void* (*func)(void*), void* arg){
    exec_routine(&POOL, func, arg);
}
