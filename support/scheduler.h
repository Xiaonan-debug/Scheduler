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


// forward declarations to avoid header dependency
typedef struct sim_context sim_context_t;
typedef struct sim_job sim_job_t;


typedef enum {
  SIM_SCHED_ACCEPT,
  SIM_SCHED_REJECT
} sim_sched_acceptance_t;

// This is the set of calls your scheduler needs to be able to
// handle.  "state" is a pointer to the scheduler's state you supplied
// when registering it.
typedef struct sim_sched_ops {
  int (* init)(void*          state,
               sim_context_t* context);

  sim_sched_acceptance_t (* periodic_job_arrival)(void*          state,
                                                  sim_context_t* context,
                                                  double         current_time,
                                                  sim_job_t*     job);
  sim_sched_acceptance_t (* sporadic_job_arrival)(void*          state,
                                                  sim_context_t* context,
                                                  double         current_time,
                                                  sim_job_t*     job);
  sim_sched_acceptance_t (* aperiodic_job_arrival)(void*          state,
                                                   sim_context_t* context,
                                                   double         current_time,
                                                   sim_job_t*     job);

  void (* job_done)(void*          state,
                    sim_context_t* context,
                    double         current_time,
                    sim_job_t*     job);

  void (* timer_interrupt)(void*          state,
                           sim_context_t* context,
                           double         current_time);
} sim_sched_ops_t;

// maximum string length of scheduler names
#define SIM_SCHED_NAME_MAX 32

// nothing in this struct may be modified by schedulers
typedef struct sim_sched {
  char name[SIM_SCHED_NAME_MAX];

  void* state;

  sim_sched_ops_t* ops;

  struct list_head node;
} sim_sched_t;


// Each scheduler must register itself at start
// with a given name => null means failure to register
sim_sched_t* sim_sched_register(char*            name,
                                void*            state,
                                sim_sched_ops_t* ops);

// and we can find it again with its name
// note that state + ops = object
sim_sched_t* sim_sched_find(char* name);

// print list of schedulers
void sim_sched_list(FILE* o);


/* Functions that forward to the specific scheduler instance */

int sim_sched_init(sim_sched_t* sched, sim_context_t* context);

sim_sched_acceptance_t sim_sched_periodic_task_arrival(sim_sched_t*   sched,
                                                       sim_context_t* context,
                                                       double         current_time,
                                                       sim_job_t*     job);

sim_sched_acceptance_t sim_sched_sporadic_job_arrival(sim_sched_t*   sched,
                                                      sim_context_t* context,
                                                      double         current_time,
                                                      sim_job_t*     job);

sim_sched_acceptance_t sim_sched_aperiodic_job_arrival(sim_sched_t*   sched,
                                                       sim_context_t* context,
                                                       double         current_time,
                                                       sim_job_t*     job);

void sim_sched_job_done(sim_sched_t*   sched,
                        sim_context_t* context,
                        double         current_time,
                        sim_job_t*     job);

void sim_sched_timer_interrupt(sim_sched_t*   sched,
                               sim_context_t* context,
                               double         current_time);

