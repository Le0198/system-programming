/**
* Password Cracker Lab
* CS 241 - Fall 2018
*/

#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include <unistd.h>
#include <crypt.h>
#include <string.h>
#include <stdio.h>
#include "queue.h"
#include <pthread.h>

struct _task {
    char username[9];
    char value[14];
    char part_pwd[9];
};
typedef struct _task task;

typedef struct _job {
    int thread_id;
    int success;
    int fail;
} job;

queue* work;

void* start_routine(void* tmp) {
    job* t = (job*)tmp;
    task* new_work;
    struct crypt_data cdata;
    cdata.initialized = 0;
    while ((new_work = queue_pull(work))) {
      v1_print_thread_start(t->thread_id, new_work->username);
      double start_time = getThreadCPUTime();

     // Find password 
      const char *hashed = NULL;
      int count = 0;
      char* password = NULL;
      char* hashed_value = new_work->value;
      char* raw = strdup(new_work->part_pwd);
      char* last = strdup(raw);
      int prefix = getPrefixLength(raw);
      for (int i = prefix; i < (int)strlen(raw); i++) {
  	  raw[i] = 'a';
	  last[i] = 'z';
      }

      do {
	hashed = crypt_r(raw, "xx", &cdata);
        count++;
        if (strcmp(hashed, hashed_value) == 0) {password = raw; break;}

      } while (strcmp(raw, last) != 0 && incrementString(raw));

      int flag = 0;
      if (password == NULL) {flag = 1; t->fail++;}
      else {t->success++;}
      double end_time = getThreadCPUTime();
      double timeElapsed = end_time - start_time;
      v1_print_thread_result(t->thread_id, new_work->username, password, count, timeElapsed, flag);
      free(raw);
      free(last);
      //free(new_work->username);
      free(new_work);
    }
    queue_push(work, NULL);
    return NULL;
}


int start(size_t thread_count) {
    // TODO you:r code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    work = queue_create(0);
     
    job threads[thread_count];
    pthread_t tid[thread_count];
    for (size_t i = 0; i < thread_count; i++) {
	threads[i].thread_id = i + 1;
	threads[i].success = 0;
	threads[i].fail = 0;
        pthread_create(&tid[i], NULL, start_routine, &threads[i]);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    while((nread = getline(&line, &len, stdin)) != -1) {
	line[nread - 1] = '\0';
	task* ret = malloc(sizeof(task));
	//char* input = strdup(line);
	char s[2] = " ";
        char *saveptr;
	char* username = strtok_r(line, s, &saveptr);
        char* value = NULL;
        char* part_pwd = NULL;
        if(username != NULL) {
            value = strtok_r(NULL, s, &saveptr);
        }
        if(ret->value != NULL) {
            part_pwd = strtok_r(NULL, s, &saveptr);
        }
        strcpy(ret->username, username);
	strcpy(ret->value, value);
	strcpy(ret->part_pwd, part_pwd);
	queue_push(work, ret);
    }
    free(line);
    //for (int i = 0; i < (int)thread_count; i++) {
	queue_push(work, NULL);
    //}

    int num_success = 0;
    int num_fail = 0;
    for (size_t i = 0; i < thread_count; i++) {
	pthread_join(tid[i], NULL);
	num_success += threads[i].success;
        num_fail += threads[i].fail;
    }
/*
    for (size_t i = 0; i < thread_count; i++) {
        num_success += threads[i].success;
        num_fail += threads[i].fail;
    }
*/    
    v1_print_summary(num_success, num_fail);


    queue_destroy(work);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
