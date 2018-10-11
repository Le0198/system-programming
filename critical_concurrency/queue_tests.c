/**
* Critical Concurrency Lab
* CS 241 - Fall 2018
*/

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "queue.h"

static queue* a;

static void* pop();
static void* push(void* ch);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: %s test_number\n", argv[0]);
        exit(1);
    }
    printf("Please write tests cases\n");
    
    queue* a = queue_create(10);
    
   printf("reach here\n"); 
   // queue_push(a, "what");
    pthread_t t1, t2, t3, t4, t5, t6;
    char* str = strdup("abdcde");
    //void* str = mm;
    pthread_create(&t1, NULL, push, str);
printf("here");
    pthread_create(&t2, NULL, pop, NULL);
    pthread_create(&t3, NULL, pop, NULL);
    pthread_create(&t4, NULL, push, str);
    pthread_create(&t5, NULL, push, str);
    pthread_create(&t6, NULL, pop, NULL);


    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    pthread_join(t5, NULL);
    pthread_join(t6, NULL);
    
    free(str);
    queue_destroy(a);
    
    return 0;
}


void* pop() {
  printf("%s\n", queue_pull(a));
  return NULL;
}

void* push(void* ch) {
  queue_push(a, ch);
  return NULL;
}
