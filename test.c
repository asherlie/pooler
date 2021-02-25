#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pool.h"

volatile void* test(void* arg){
    int x;
    memcpy(&x, arg, sizeof(int));
    printf("test func %i\n", x);
    free(arg);
    return NULL;
}

volatile void* spawn(void* v_depth){
    int* depth = v_depth;
    printf("reached recursive spawn depth of %i\n", *depth);
    if(!*depth){
        return NULL;
    }
    *depth = *depth-1;
    exec_pool(spawn, depth);
    return NULL;
}

volatile void* strtest(void* v_str){
    printf("strtest: \"%s\"\n", (char*)v_str);
    free(v_str);
    return NULL;
}

/*
 * maybe create a global variable and INIT_POOL()
 */

int main(){
    start_pool(100);
    for(int i = 0; i < 100; ++i){
        int* arg = malloc(4);
        *arg = i;
        exec_pool(test, arg);
    }
    int i = 10;
    exec_pool(spawn, &i);

    size_t n;
    char* ln = NULL;
    int sz;
    while((sz = getline(&ln, &n, stdin)) > 0){
        if(*ln == 'q')break;
        ln[--sz] = 0;
        exec_pool(strtest, ln);
        ln = NULL;
    }
    end_pool();
}
