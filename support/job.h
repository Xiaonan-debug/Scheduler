/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */
#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"


// forward declaration of to avoid header dependency
typedef struct sim_context sim_context_t;


typedef enum {
  SIM_JOB_PERIODIC,  // realtime jobs that recur
  SIM_JOB_SPORADIC,  // realtime jobs that run once only
  SIM_JOB_APERIODIC, // non-realtime jobs, can include priority
} sim_job_type_t;

// a job contains information from all kinds of jobs/tasks, some of which may
//  not be relevant for the given job type
// nothing in this struct may be modified by schedulers
typedef struct sim_job {
  uint64_t id;
  sim_job_type_t type;

  // information that is valid for all jobs
  double arrival_time;
  double size;
  uint64_t static_priority; // higher number implies higher priority

  // bookkeeping variables for scheduler use
  // these may both be modified by the scheduler as desired
  // the values of these variables have no affect on the simulation
  double remaining_size; // modified with sim_job_set_remaining_size()
  uint64_t dynamic_priority; // modified with sim_job_set_dynamic_priority()

  // information that is valid for periodic or sporadic
  // real-time jobs
  double deadline;

  // information that is valid for periodic real-time jobs
  // numiters is the number of times the job will re-arrive
  double period;
  uint64_t numiters;
  bool first_arrival;

  // this allows you to put the job into a job queue
  // it can only be in one job queue at a time
  struct list_head node;

} sim_job_t;


// allocate and initialize a job
sim_job_t* sim_job_create(sim_job_type_t type,
                          double         arr_time,
                          double         size,
                          double         remaining_size,
                          uint64_t       static_priority,
                          uint64_t       dynamic_priority,
                          double         period,
                          uint64_t       numiters,
                          double         deadline);

// call on job completion - will regenerate periodic job
int sim_job_complete(sim_context_t* context,
                     sim_job_t*     job);


// the only way to delete a job - it must already be removed from any queue
void sim_job_destroy(sim_job_t* job);

// print the contents of the job for debugging
void sim_job_print(sim_job_t* job, FILE* f);

// modify the remaining size of the job
void sim_job_set_remaining_size(sim_job_t* job, double remaining_size);

// modify the dynamic priority of the job
void sim_job_set_dynamic_priority(sim_job_t* job, uint64_t dynamic_priority);

