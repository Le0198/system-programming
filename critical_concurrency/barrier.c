/**
* Critical Concurrency Lab
* CS 241 - Fall 2018
*/

#include "barrier.h"

//static int flag = 0; 

// The returns are just for errors if you want to check for them.
int barrier_destroy(barrier_t *barrier) {
    int error = 0;
    if (barrier == NULL) {return error;}
    pthread_cond_destroy(&(barrier->cv));
    pthread_mutex_destroy(&(barrier->mtx));
    return error;
}

int barrier_init(barrier_t *barrier, unsigned int num_threads) {
    int error = 0;
    if (barrier == NULL) {return error;}
    barrier->count = num_threads;
    barrier->times_used = 0;
    barrier->n_threads = num_threads;
    pthread_mutex_init(&(barrier->mtx), NULL);
    pthread_cond_init(&(barrier->cv), NULL);
    return error;
}

int barrier_wait(barrier_t *barrier) {
    
    if (barrier == NULL) {return 0;}
    pthread_mutex_lock(&(barrier->mtx));
    //barrier->count--;
    while(barrier->count > barrier->n_threads) {
        pthread_cond_wait(&(barrier->cv), &(barrier->mtx));
    }
    barrier->count--;
    if (barrier->count == 0) {
	barrier->count = 2 * barrier->n_threads - 1;
	barrier->times_used++;
    }
    else{
    	while(barrier->count < barrier->n_threads) {
	    pthread_cond_wait(&(barrier->cv), &(barrier->mtx));
        }
        barrier->count--;
    }

    pthread_mutex_unlock(&(barrier->mtx));
    pthread_cond_broadcast(&(barrier->cv));  
  return barrier->times_used;
}
