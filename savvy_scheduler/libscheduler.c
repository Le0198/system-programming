/**
* Savvy_scheduler Lab
* CS 241 - Fall 2018
*/

#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "print_functions.h"
typedef struct _job_info {
    int id;
    int priority;
    double runtime;
    double arrival_time;
    double start_time;
    double end_time;
    double rr_time;

    double used_time;
    /* Add whatever other bookkeeping you need into this struct. */
} job_info;

priqueue_t pqueue;
scheme_t pqueue_scheme;
comparer_t comparision_func;

//total info
int job_count;
double total_wait_time;
double total_turnaround_time;
double total_response_time;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any set up code you may need here
    job_count = 0;
    total_wait_time = 0;
    total_turnaround_time = 0;
    total_response_time = 0;
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) {
    job_info* joba = ((job*)a)->metadata;
    job_info* jobb = ((job*)b)->metadata;
    if ( joba->arrival_time < jobb->arrival_time ) { return -1; }
    if ( jobb->arrival_time < joba->arrival_time ) { return 1; }
    return 0;
}

int comparer_ppri(const void *a, const void *b) {
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
     
    job_info* joba = ((job*)a)->metadata;
    job_info* jobb = ((job*)b)->metadata;
    if (joba->priority < jobb->priority) {return -1;}
    if (joba->priority > jobb->priority) {return 1;}
    return break_tie(a, b);
}

int comparer_psrtf(const void *a, const void *b) {
    job_info* joba = ((job*)a)->metadata;
    job_info* jobb = ((job*)b)->metadata;
    double remain_a = joba->runtime - joba->used_time;
    double remain_b = jobb->runtime - jobb->used_time;
    if (remain_a < remain_b) {return -1;}
    if (remain_a > remain_b) {return 1;}
    return break_tie(a, b);
}

int comparer_rr(const void *a, const void *b) {
    job_info* joba = ((job*)a)->metadata;
    job_info* jobb = ((job*)b)->metadata;
    if (joba->rr_time < jobb->rr_time) {return -1;}
    if (joba->rr_time > jobb->rr_time) {return 1;}
    return 0;
}

int comparer_sjf(const void *a, const void *b) {
    job_info* joba = ((job*)a)->metadata;
    job_info* jobb = ((job*)b)->metadata;
    if (joba->runtime < jobb->runtime) {return -1;}
    if (jobb->runtime < joba->runtime) {return 1;}
    return break_tie(a, b);
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // TODO complete me!
    job_info* jobinfo = malloc(sizeof(job_info));
    newjob->metadata = jobinfo;

    jobinfo->id = job_number;
    jobinfo->arrival_time = time;
    jobinfo->priority = sched_data->priority;
    jobinfo->runtime = sched_data->running_time;

    jobinfo->start_time = 0;
    jobinfo->end_time = 0;
    jobinfo->rr_time = 0;
    jobinfo->used_time = 0;

    priqueue_offer(&pqueue, newjob);
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO complete me!
    /*
    job_info* info = job_evicted->metadata;
    if (pqueue_scheme == PPRI || pqueue_scheme == PSRTF || pqueue_scheme == RR) {
	if (job_evicted) {
           info->used_time += time - info->rr_time;
           priqueue_offer(&pqueue, job_evicted);
        }
        job *next = priqueue_poll(&pqueue);
        if (next) {
	    job_info* next_info = next->metadata;
            if (next_info->used_time == 0) {
                next_info->start_time = time;
            }
            next_info->rr_time = time;
        }
        return next;
    }

    if (job_evicted) {
	return job_evicted;
    } else {
	job* next = priqueue_poll(&pqueue);
	if (next) {
	    job_info* next_info = next->metadata;
	    if (next_info->used_time == 0) {
		next_info->start_time = time;
	    }
	    next_info->rr_time = time;
	}
	return next;
    }
*/

    job_info *info;
    job_info *job_next_info;
    switch (pqueue_scheme) {
        case PPRI:
        case PSRTF:
        case RR:
            if (job_evicted) {
                info = job_evicted->metadata;
                info->used_time += time - info->rr_time;
                priqueue_offer(&pqueue, job_evicted);
            }
            job *next_job = priqueue_poll(&pqueue);
            if (next_job) {
                job_next_info = next_job->metadata;
                if (job_next_info->used_time == 0) {
                    job_next_info->start_time = time;
                }
                job_next_info->rr_time = time;
            }
            return next_job;

        default:
            if (job_evicted) {
                return job_evicted;
            } else {
                job *next_job = priqueue_poll(&pqueue);
                if (next_job) {
                    job_next_info = next_job->metadata;
                    if (job_next_info->used_time == 0) {
                        job_next_info->start_time = time;
                    }
                    job_next_info->rr_time = time;
                }
                return next_job;
            }
    }
}

void scheduler_job_finished(job *job_done, double time) {
    // TODO complete me!
    
    job_info* info = (job_info*)(job_done->metadata);
    info->end_time = time;
    info->used_time += time - info->rr_time;    
    total_response_time += (info->start_time - info->arrival_time);
    total_turnaround_time += (info->end_time - info->arrival_time);
    total_wait_time += (info->end_time - info->arrival_time - info->runtime);

    job_count++;
    free(job_done->metadata);
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO complete me!
    double ret = total_wait_time/job_count;
    return ret;
}

double scheduler_average_turnaround_time() {
    // TODO complete me!
    double ret =  total_turnaround_time/job_count;
    return ret;
}

double scheduler_average_response_time() {
    // TODO complete me!
    double ret = total_response_time/job_count;
    return ret;
}

void scheduler_show_queue() {
    // Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
