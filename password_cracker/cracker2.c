/**
* Password Cracker Lab
* CS 241 - Fall 2018
*/

#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <crypt.h>
/*
typedef struct _task {
    char* username;
    char* value;
    char* part_pwd;
} task;
*/
typedef struct _job {
    int thread_id;
    char* username;
    char* value;
    char* pwd;
    long start_index;
    long count;
    int hash_count;
} job;

static int find;
static int stop = 0;
static char* password;
static pthread_barrier_t barrier1;
static pthread_barrier_t barrier2;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void* start_routine(void* tmp) {
    job* t = (job*)tmp;
    struct crypt_data cdata;
    cdata.initialized = 0;
    
    while (1) {
      pthread_barrier_wait(&barrier1);
      if (stop) {break;}
      char* raw = strdup(t->pwd);
      int prefix = getPrefixLength(raw);
      char* dot = raw + prefix;
      setStringPosition(dot, t->start_index);
      v2_print_thread_start(t->thread_id, t->username, t->start_index, raw);

     // Find password 
      const char *hashed = NULL;
      char* hashed_value = t->value;
      int flag = 2;
      for (int i = 0; i < (int)t->count; i++) {
	pthread_mutex_lock(&m);
	if (find == 1) {
	   pthread_mutex_unlock(&m);
	   flag = 1;
	   break;
	}
	pthread_mutex_unlock(&m);
        hashed = crypt_r(raw, "xx", &cdata);
        t->hash_count++;
        if (strcmp(hashed, hashed_value) == 0) {
	 
	  pthread_mutex_lock(&m);
	  find = 1;
	  flag = 0;
	  strcpy(password, raw);
	  pthread_mutex_unlock(&m);
	  break;
        }
        incrementString(raw);
      }
      
      pthread_mutex_lock(&m); 
      v2_print_thread_result(t->thread_id, t->hash_count, flag);
      pthread_mutex_unlock(&m); 
      free(raw);

      pthread_barrier_wait(&barrier2);
    }
    return NULL;
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads

    pthread_barrier_init(&barrier1, NULL, thread_count + 1);
    pthread_barrier_init(&barrier2, NULL, thread_count + 1);
//    pthread_mutex_init(&m, NULL);
    job* mytask = malloc(thread_count * sizeof(job));
    pthread_t tid[thread_count];
    for (int i = 0; i < (int)thread_count; i++) {
	pthread_create(&tid[i], NULL, start_routine, &mytask[i]);
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t nread;
    while ((nread = getline(&line, &len, stdin)) != -1) {
      if (line[nread-1] == '\n') {line[nread-1] = '\0';}
      char s[2] = " ";
      char *saveptr;
      char* username = strtok_r(line, s, &saveptr);
      char* value = NULL;
      char* pwd = NULL;
      if(username != NULL) {
        value = strtok_r(NULL, s, &saveptr);
      }
      if(value != NULL) {
        pwd = strtok_r(NULL, s, &saveptr);
      }
      password = malloc(9);
      for (int i = 0; i < (int)thread_count; i++) {
        mytask[i].username = username;
 	mytask[i].value = value;
	mytask[i].pwd = pwd;
	mytask[i].thread_id = i + 1;
	mytask[i].hash_count = 0;
	int unknown = strlen(pwd) - getPrefixLength(pwd);
	getSubrange(unknown, thread_count, i+1, &mytask[i].start_index, &mytask[i].count);
      }
      pthread_mutex_lock(&m);
      find = 0;
      pthread_mutex_unlock(&m);
      v2_print_start_user(username);
      double start = getTime();
      double startCPU = getCPUTime();

      pthread_barrier_wait(&barrier1);
      pthread_barrier_wait(&barrier2);

      int totalHash = 0;
      for (int i = 0; i < (int)thread_count; i++) {
	totalHash += mytask[i].hash_count;
      }

      double timeElapse = getTime() - start;
      double totalCPUTime = getCPUTime() - startCPU;
      v2_print_summary(username, password, totalHash, timeElapse, totalCPUTime, !find);
      
      free(password); 
    }
    
    stop = 1;
    pthread_barrier_wait(&barrier1);
    for (int i = 0; i < (int)thread_count; i++) {
	pthread_join(tid[i], NULL);
    }
    free(mytask);
    free(line);
//    pthread_mutex_destroy(&m);
    pthread_barrier_destroy(&barrier1);
    pthread_barrier_destroy(&barrier2);
    
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
