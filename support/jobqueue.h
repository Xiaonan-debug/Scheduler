/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include "job.h"
#include "list.h"


// forward declaration to avoid header dependency
typedef struct sim_job sim_job_t;

// nothing in this struct may be modified by schedulers
typedef struct sim_job_queue {
  // total number of jobs in the queue
  uint64_t num_jobs;

  struct list_head list;
} sim_job_queue_t;


// initialize a job queue
void sim_job_queue_init(sim_job_queue_t* jq);


/* Functions for inserting jobs */

// enqueue at end
void sim_job_queue_enqueue(sim_job_queue_t* jq, sim_job_t* job);

// put job before or after the target
void sim_job_queue_enqueue_before(sim_job_queue_t* jq, sim_job_t* job, sim_job_t* target);
void sim_job_queue_enqueue_after(sim_job_queue_t* jq, sim_job_t* job, sim_job_t* target);

// insert job into queue according to order implied by comparison function
// <0 => lhs before rhs
//  0 => lhs in same equivalence class as rhs
// >0 => rhs before lhs
void sim_job_queue_enqueue_in_order(sim_job_queue_t* jq,
                                    sim_job_t* job,
                                    int (* compare)(sim_job_t* lhs, sim_job_t* rhs));


/* Functions for removing jobs */

// take a look at the head job
sim_job_t* sim_job_queue_peek(sim_job_queue_t* jq);

// dequeue the head job
sim_job_t* sim_job_queue_dequeue(sim_job_queue_t* jq);

// remove job from the queue, whereever it is
void sim_job_queue_remove(sim_job_queue_t* jq, sim_job_t* job);

// apply the selection function to the jobs and return the first one that matches
sim_job_t* sim_job_queue_search(sim_job_queue_t* jq,
                                int (* condition)(void* state, sim_job_t* job),
                                void* state);


/* Other helpful functions */

// apply function across all jobs in the queue
int sim_job_queue_map(sim_job_queue_t* jq,
                      int (* func)(void* state, sim_job_t* job),
                      void* state);

// get total time to complete all jobs in queue
double sim_job_queue_get_total_time(sim_job_queue_t* jq);

// get total remaining to complete all jobs in queue
double sim_job_queue_get_total_remaining_time(sim_job_queue_t* jq);

// print all contents of the job queue
void sim_job_queue_print(sim_job_queue_t* jq, FILE* f);

