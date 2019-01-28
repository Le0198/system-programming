/**
* Parallel Make Lab
* CS 241 - Fall 2018
*/


#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "queue.h"
#include "set.h"
#include "vector.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>


static queue* checked;
static queue* jobs;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

typedef struct _info {
  int satisfied_dependencies;
  vector* dependencies;
} info;

int checkCyclicFunc(char* v, graph* g, set* visited);
void find_useful_nodes_helper(graph* g, char* t);
int detectCycle(graph* g, char* tgt);
void find_useful_nodes(graph* g);
int rule_valid_to_run(rule_t *rule);
void execRule(rule_t* rule);
void checkSatisfied(rule_t* rule);
//void find_useful_nodes(graph *g);
void* myjob(void*);

// Detect Cycle
int checkCyclicFunc(char* v, graph* g, set* visited) {
    //rule_t* rule = (rule_t *)graph_get_vertex_value(g, v);
    if (set_contains(visited, v)) {
        return 1;
    }
    else {
        set_add(visited, v);
        vector* neighbors = graph_neighbors(g, v);
        for (size_t i = 0; i < vector_size(neighbors); i++) {
            char* n = vector_get(neighbors, i);
            if (checkCyclicFunc(n, g, visited)) {
                vector_destroy(neighbors);
                return 1;
            }
        }
        vector_destroy(neighbors);
        set_remove(visited, v);
        return 0;
    }
}

int detectCycle(graph* g, char* tgt) {
    set* s = string_set_create();
    if (checkCyclicFunc(tgt, g, s)) {
      set_destroy(s);
      return 1;
    }
    set_destroy(s);
    return 0;
}


void find_useful_nodes_helper(graph* g, char* t) {
  rule_t * rule = graph_get_vertex_value(g, t);
  if (rule->state == 1) {return;}
  rule->state = 1;
  vector * vec = graph_neighbors(g, t);
//  if (vector_empty(vec)) {return;}
  for (int a = 0; a < (int)vector_size(vec); a++) {
    find_useful_nodes_helper(g, vector_get(vec, a));
//    vector_destroy(vec);
  }
  vector_destroy(vec);
}

void find_useful_nodes(graph* g) {
  vector* vertices = graph_vertices(g);
  for (int i = 0; i < (int)vector_size(vertices); i++) {
    rule_t* rule = (rule_t *)graph_get_vertex_value(g, vector_get(vertices, i));
    rule->state = 0;
  }
  find_useful_nodes_helper(g, "");
  vector_destroy(vertices);
  vertices = graph_vertices(g);
  for (int i = 0; i < (int)vector_size(vertices); i++) {
    char* v = vector_get(vertices, i);
    rule_t* r = (rule_t *)graph_get_vertex_value(g, v);
    if (r->state == 0) { graph_remove_vertex(g, v); }
  }
  vector_destroy(vertices);
}


int rule_valid_to_run(rule_t *rule) {
    if (access(rule->target, F_OK) != 0) { return 1; }
    struct stat t_stat;
    struct stat d_stat;
    stat(rule->target, &t_stat);
    time_t update_time = t_stat.st_mtime;
    stat(rule->target, &t_stat);
    info* myinfo = (info*)rule->data;
    for (int i = 0; i < (int)vector_size(myinfo->dependencies); i++) {
        char* dependency = vector_get(myinfo->dependencies, i);
	if (access(dependency, F_OK) != 0) { return 1; }
        stat(dependency, &d_stat);
        if (d_stat.st_mtime > update_time) { return 1; }
    }
    return 0;
}

void execRule(rule_t* rule) {
  for (int i = 0; i < (int)vector_size(rule->commands); i++) {
    char* cmd = vector_get(rule->commands, i);
    int result = system(cmd);
    if (result) {
      pthread_mutex_lock(&m);
      rule->state = -1;
      pthread_mutex_unlock(&m);
      return;
    }
  }
  pthread_mutex_lock(&m);
  rule->state = 0;
  pthread_mutex_unlock(&m);
  return;
}

void checkSatisfied(rule_t* rule) {
  if (rule_valid_to_run(rule)) {
    execRule(rule);
    return;
  }
  pthread_mutex_lock(&m);
  rule->state = 0;
  pthread_mutex_unlock(&m);
}

void* myjob(void* ptr) {
  rule_t* job;
  while( (job = queue_pull(jobs)) ) {
    checkSatisfied(job);
    queue_push(checked, job);
  }
  queue_push(jobs, NULL);
  return ptr;
} 

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    graph* mygraph = parser_parse_makefile(makefile, targets);
    
    vector* tgts = graph_neighbors(mygraph, "");

    for (int i = 0; i < (int)vector_size(tgts); i++) {
      char* v = vector_get(tgts, i);
      if (detectCycle(mygraph, v)) {
        print_cycle_failure(v);
        graph_remove_edge(mygraph, "", v);
      }
    }

    find_useful_nodes(mygraph);
    vector* leaf = string_vector_create();
    vector* all = graph_vertices(mygraph);
    size_t sz = vector_size(all);
    
    for (int i = 0; i < (int)sz; i++) {
      char* node = vector_get(all, i);
      vector* neighbors = graph_neighbors(mygraph, node);
      rule_t* rule = (rule_t*)graph_get_vertex_value(mygraph, node);

      info* mydata = malloc(sizeof(info));
      rule->data = mydata;
      mydata->dependencies = shallow_vector_create();
      VECTOR_FOR_EACH(neighbors, v, vector_push_back(mydata->dependencies, v));
      mydata->satisfied_dependencies = 0;

      if (vector_empty(neighbors)) { vector_push_back(leaf, node); }
      vector_destroy(neighbors);
    }

    jobs = queue_create(0);
    checked = queue_create(0);
    
    for (int i = 0; i < (int)vector_size(leaf); i++) {
      char* t = vector_get(leaf, i);
      rule_t* rule = (rule_t*)graph_get_vertex_value(mygraph, t);
      queue_push(jobs, rule);
    }

    // Create threads
    pthread_t tid[num_threads];
    for (int i = 0; i < (int)num_threads; i++) {
      pthread_create(&tid[i], NULL, myjob, NULL);
    }

    while (1) {
      //char* target = queue_pull(checked);
      //rule_t* rule = (rule_t*)graph_get_vertex_value(mygraph, target);
      rule_t* rule = queue_pull(checked);
      char* target = rule->target;
      if (strcmp(rule->target, "") == 0) {
	break;
      }
      
      vector* parents = graph_antineighbors(mygraph, target);
      if (rule->state == -1) {
       // vector* parents = graph_antineighbors(mygraph, target);
        for (int i = 0; i < (int)vector_size(parents); i++) {
          char* p = vector_get(parents, i);
          rule_t* r = (rule_t*)graph_get_vertex_value(mygraph, p);
          pthread_mutex_lock(&m);
	  r->state = -1;
	  pthread_mutex_unlock(&m);
          queue_push(checked, r);
        }
      }
      else if (rule->state == 0) {
        for (int i = 0; i < (int)vector_size(parents); i++) {
          char* p = vector_get(parents, i);
          rule_t* r = (rule_t*)graph_get_vertex_value(mygraph, p);
	  info* myinfo = (info*)r->data;
	  pthread_mutex_lock(&m);
          myinfo->satisfied_dependencies++;
          if (myinfo->satisfied_dependencies == (int)vector_size(myinfo->dependencies)) {
            /*execRule(r);
            queue_push(checked, p);*/
	    queue_push(jobs, r);
          }
	  pthread_mutex_unlock(&m);
        }
      }
      vector_destroy(parents);
    }
    
    queue_push(jobs, NULL);
    for (int i = 0; i < (int)num_threads; i++) {
      pthread_join(tid[i], NULL);
    }

    //clean
    vector_destroy(leaf);
    vector_destroy(tgts);
    for (int i = 0; i < (int)vector_size(all); i++) { 
	rule_t* r = (rule_t*)graph_get_vertex_value(mygraph, vector_get(all, i));
	info* d = (info*)(r->data);
	vector_destroy(d->dependencies);
	free(r->data);
    }
    vector_destroy(all);
    queue_destroy(jobs);
    queue_destroy(checked);
    graph_destroy(mygraph);


    return 0;
}
