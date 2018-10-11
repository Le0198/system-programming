/**
* Teaching Threads Lab
* CS 241 - Fall 2018
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct _reduce_task {
    size_t thread_part;
    size_t tnum;
    reducer reduce_func;
    int basecase;
    int *lst;
    size_t len;
}reduce_task;


/* You should create a start routine for your threads. */
void* myfunc(void *ptr) {
    if (!ptr) {return NULL;}
    reduce_task* p = ptr;
    size_t tnum = p->tnum;
    size_t tp = p->thread_part++;
    size_t skip = (p->len)/tnum;
    size_t len = skip;
    if (tp == tnum - 1) {len = p->len - skip * (tnum - 1);}
    int r = reduce(p->lst + tp * skip, len, p->reduce_func, p->basecase);
    int* ret = malloc(sizeof(int));
    *ret = r;
//printf("thread part: %zu lst: %d %d len: %zu r: %d ret:%d\n",tp, (p->lst+tp*skip)[0], (p->lst+tp*skip)[1], len, r, *ret);
    return ret;
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    if(!list || list_len == 0) {return 0;}
    if(list_len <= num_threads) {
	return reduce(list, list_len, reduce_func, base_case);
    }
    size_t num_th = num_threads;
    pthread_t id[num_th];
    reduce_task* task = malloc(sizeof(reduce_task));
    task->reduce_func = reduce_func;
    task->basecase = base_case;
    task->lst = list;
    task->len = list_len;
    task->tnum = num_threads;
    task->thread_part = 0;

    for (size_t i = 0; i < num_th; ++i) {
        pthread_create(&id[i], NULL, myfunc, task);
    }
    void* result[num_th];
    int ret[num_th];
    for (size_t i = 0; i < num_th; i++) {
	pthread_join(id[i], &result[i]);
	ret[i] = *(int*)(result[i]);
    }
    int retval = reduce(ret, num_th, reduce_func, base_case);
    for (size_t i = 0; i < num_th; i++) { free(result[i]); }
    return retval;
}
