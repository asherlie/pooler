#include <unistd.h>

#define DELAY usleep(1000)

extern struct thread_pool POOL;
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


struct thread_node{
    struct thread* thread_info;
    struct thread_node* next, * prev;
};
/* linked list containing threads
 */
struct thread_ll{
    /* struct thread* thread_info; */
    int n_threads;
    struct thread_node* first, * last;
    /*
     * pthread_t thread;
     * struct func_arg* f_a;
     */
    /* struct thread_ll* next, * prev; */
};

struct routine_queue{
    /* this lock is used to ensure that messages are removed from the queue before */
    pthread_mutex_t lock;
    struct func_arg* fa, * base_ptr;
    int n_requests, cap;
};

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

void init_pool(struct thread_pool* p, int n_threads);
void destroy_pool(struct thread_pool* p);
int exec_routine(struct thread_pool* p, volatile void* (*func)(void*), void* arg);

/* these three functions operate on the global POOL in order to simplify the usage of this library */
void start_pool(int n_threads);
void end_pool();
void exec_pool(volatile void* (*func)(void*), void* arg);

_Bool is_complete(struct thread_pool* tp);
