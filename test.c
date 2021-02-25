#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pool.h"

volatile void* test(void* arg){
    int x;
    memcpy(&x, arg, sizeof(int));
    printf("test func %i\n", x);
    return NULL;
}
/*
 * maybe create a global variable and INIT_POOL()
 */

int main(){
    start_pool(12);
    for(int i = 0; i < 20; ++i){
        int* arg = malloc(4);
        *arg = i;
        exec_pool(test, arg);
    }
    while(getc(stdin) != 'q')DELAY;
    end_pool();
}
