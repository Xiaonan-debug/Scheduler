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

#include "context.h"
#include "job.h"
#include "list.h"


// forward declarations to avoid header dependency
typedef struct sim_context sim_context_t;
typedef struct sim_job sim_job_t;


typedef enum {
  SIM_EVENT_PERIODIC_TASK_ARRIVAL,
  SIM_EVENT_SPORADIC_JOB_ARRIVAL,
  SIM_EVENT_APERIODIC_JOB_ARRIVAL,
  SIM_EVENT_JOB_DONE,
  SIM_EVENT_TIMER,
  SIM_EVENT_PRINT_ALL,
  SIM_EVENT_PRINT_STATS,
  SIM_EVENT_PRINT_JOB_QUEUES,
  SIM_EVENT_PRINT_EVENT_QUEUE,
  SIM_EVENT_DISPLAY_QUEUE_DEPTHS,
} sim_event_type_t;

// nothing in this struct may be modified by schedulers
typedef struct sim_event {
  // details about the event
  uint64_t id;
  double timestamp;
  sim_event_type_t type;

  // the simulation context
  sim_context_t* context;

  // the job this event is associated with
  // set to NULL if there is no associated job
  sim_job_t* job;

  // an event can be in only one list at a time
  struct list_head node;
} sim_event_t;


// must be called to create an event
sim_event_t* sim_event_create(double           time,
                              sim_context_t*   context,
                              sim_event_type_t type,
                              sim_job_t*       job);

// called internally by the main simulation loop
void sim_event_dispatch(sim_event_t* event);

// called internally to print event details
void sim_event_print(sim_event_t* event, FILE* f);

// must be called when events are completed
void sim_event_complete(sim_event_t* event);

