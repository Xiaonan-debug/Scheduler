/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "eventqueue.h"
#include "jobqueue.h"
#include "scheduler.h"


// forward declarations to avoid header dependency
typedef struct sim_sched sim_sched_t;

// nothing in this struct may be modified by schedulers
typedef struct sim_context {
  // the queues of events and jobs
  sim_event_queue_t event_queue;
  sim_job_queue_t realtime_queue;
  sim_job_queue_t aperiodic_queue;

  // scheduler to use and quantum for it
  sim_sched_t* scheduler;
  double quantum;

  // output log files
  FILE* queuelen_file;
  FILE* log_file;
  FILE* job_file;

  // the remainder is statistics tracking
  uint64_t num_periodic_tasks;
  uint64_t num_periodic_tasksrejected;
  uint64_t num_periodic_jobs;
  uint64_t num_periodic_misses;
  double sum_periodic_misssize;
  double sum_periodic_missratio;
  double sum2_periodic_misssize;
  double sum2_periodic_missratio;

  uint64_t num_sporadic_jobs;
  uint64_t num_sporadic_jobsrejected;
  uint64_t num_sporadic_misses;
  double sum_sporadic_misssize;
  double sum_sporadic_missratio;
  double sum2_sporadic_misssize;
  double sum2_sporadic_missratio;

  uint64_t num_aperiodic;
  double sum_resptime;
  double sum_slowdown;
  double sum2_resptime;
  double sum2_slowdown;

  uint64_t num_timer_interrupts;
} sim_context_t;


// simulation creation/completion
int  sim_context_init(sim_context_t* context, char* sched_name, double quantum);
void sim_context_deinit(sim_context_t* context);
int  sim_context_load_events(sim_context_t* context, char* filename);
int  sim_context_begin(sim_context_t* context);

// run the simulation
sim_event_t* sim_context_get_next_event(sim_context_t* context);
void         sim_context_dispatch_event(sim_context_t* context, sim_event_t* event);

// helper function to get current time in the simulation
double sim_context_get_current_time(sim_context_t* context);

// track stats when various scheduling actions occur
void sim_context_inform_job_arrival(sim_context_t* context, sim_job_t* job);
void sim_context_inform_task_arrival(sim_context_t* context, sim_job_t* job);
void sim_context_inform_job_acceptance(sim_context_t* context, sim_job_t* job, sim_sched_acceptance_t rc);
void sim_context_inform_task_acceptance(sim_context_t* context, sim_job_t* job, sim_sched_acceptance_t rc);
void sim_context_inform_job_done(sim_context_t* context, sim_job_t* job);
void sim_context_inform_timer_interrupt(sim_context_t* context);

// print stats
void sim_context_print_all(sim_context_t* context, FILE* f);
void sim_context_print_stats(sim_context_t* context, FILE* f);
void sim_context_print_job_queues(sim_context_t* context, FILE* f);
void sim_context_print_event_queue(sim_context_t* context, FILE* f);

// use external tool to display a graph of results
void sim_context_display_queue_depths(sim_context_t* context);

