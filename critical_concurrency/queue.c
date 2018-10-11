/**
* Critical Concurrency Lab
* CS 241 - Fall 2018
*/

#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
    queue* ret = malloc(sizeof(queue));
    ret->head = NULL;
    ret->tail = NULL;
    ret->size = 0;
    ret->max_size = max_size;
    pthread_cond_init(&(ret->cv), NULL);
    pthread_mutex_init(&(ret->m), NULL);
    return ret;
}

void queue_destroy(queue *this) {
    /* Your code here */
    queue_node* tmp = this->head;
    while(tmp) {
	queue_node *t = tmp;
	tmp = tmp->next;
	free(t);
    }
    pthread_cond_destroy(&(this->cv));
    pthread_mutex_destroy(&(this->m));
    free(this);
}

void queue_push(queue *this, void *data) {
    /* Your code here */
    pthread_mutex_lock(&(this->m));
    if (this->max_size > 0) {
    	while(this->size == this->max_size) {
	    pthread_cond_wait(&(this->cv), &(this->m));
        }
    }
    queue_node* newnode = malloc(sizeof(queue_node));
    newnode->data = data;
    newnode->next = NULL;
    if(this->size != 0) {
	this->tail->next = newnode;
    	this->tail = newnode;
    }
    else {
	this->tail = newnode;
	this->head = newnode;
    }
    this->size++;
    if (this->size == 1) {
	pthread_cond_broadcast(&(this->cv));
    }
    pthread_mutex_unlock(&(this->m));
}

void *queue_pull(queue *this) {
    /* Your code here */
    void* result = NULL;
    pthread_mutex_lock(&(this->m));
    while(this->size == 0) {
	pthread_cond_wait(&(this->cv), &(this->m));
    }
    queue_node* tmp = this->head;
    this->head = this->head->next;
    if (this->head == NULL) {this->tail = NULL;}
    result = tmp->data;
    free(tmp);
    this->size--;
    if (this->max_size > 0 && this->size == this->max_size - 1) {
	pthread_cond_broadcast(&(this->cv));
    }
    pthread_mutex_unlock(&(this->m));
    return result;
}
