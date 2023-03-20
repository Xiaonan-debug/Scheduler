/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "debug.h"
#include "event.h"
#include "job.h"
#include "scheduler.h"


// control debugging prints throughout this file
#if DEBUG_EVENT
#define DEBUG(fmt, args...) DEBUG_PRINT("event: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define ERROR(fmt, args...) ERROR_PRINT("event: " fmt, ##args)
#define INFO(fmt, args...)  INFO_PRINT("event: " fmt, ##args)


// monotonically increasing value used to assign event IDs
static uint64_t current_event_id = 0;


/* Internal helper functions */

static void sim_event_init(sim_event_t*     e,
                           double           time,
                           sim_context_t*   context,
                           sim_event_type_t type,
                           sim_job_t*       job) {
  memset(e, 0, sizeof(*e));

  e->id = current_event_id++;

  e->timestamp = time;
  e->context   = context;
  e->type      = type;
  e->job       = job;

  INIT_LIST_HEAD(&e->node);
}


/* Public functions */

sim_event_t* sim_event_create(double           time,
                              sim_context_t*   context,
                              sim_event_type_t type,
                              sim_job_t*       job) {
  sim_event_t* e = malloc(sizeof(sim_event_t));
  if (!e) {
    ERROR("can't allocate event\n");
    return NULL;
  }

  sim_event_init(e, time, context, type, job);

  return e;
}

void sim_event_dispatch(sim_event_t* e) {

  // inform context of event occurring
  // call appropriate scheduler function, if any
  double current_time = sim_context_get_current_time(e->context);
  sim_sched_acceptance_t rc;
  switch (e->type) {
    case SIM_EVENT_PERIODIC_TASK_ARRIVAL:
      sim_context_inform_task_arrival(e->context, e->job);

      rc = sim_sched_periodic_task_arrival(e->context->scheduler,
                                           e->context,
                                           current_time,
                                           e->job);

      sim_context_inform_task_acceptance(e->context, e->job, rc);

      break;

    case SIM_EVENT_SPORADIC_JOB_ARRIVAL:
      sim_context_inform_job_arrival(e->context, e->job);

      rc = sim_sched_sporadic_job_arrival(e->context->scheduler,
                                          e->context,
                                          current_time,
                                          e->job);

      sim_context_inform_job_acceptance(e->context, e->job, rc);

      break;

    case SIM_EVENT_APERIODIC_JOB_ARRIVAL:
      sim_context_inform_job_arrival(e->context, e->job);

      rc = sim_sched_aperiodic_job_arrival(e->context->scheduler,
                                           e->context,
                                           current_time,
                                           e->job);

      sim_context_inform_job_acceptance(e->context, e->job, rc);

      break;

    case SIM_EVENT_JOB_DONE:
      sim_context_inform_job_done(e->context, e->job);
      sim_sched_job_done(e->context->scheduler, e->context, current_time, e->job);

      break;

    case SIM_EVENT_TIMER:
      sim_context_inform_timer_interrupt(e->context);
      sim_sched_timer_interrupt(e->context->scheduler, e->context, current_time);

      break;

    case SIM_EVENT_PRINT_STATS:
      sim_context_print_stats(e->context, stdout);

      break;

    case SIM_EVENT_PRINT_JOB_QUEUES:
      sim_context_print_job_queues(e->context, stdout);

      break;

    case SIM_EVENT_PRINT_EVENT_QUEUE:
      sim_context_print_event_queue(e->context, stdout);

      break;

    case SIM_EVENT_PRINT_ALL:
      sim_context_print_all(e->context, stdout);

      break;

    case SIM_EVENT_DISPLAY_QUEUE_DEPTHS:
      sim_context_display_queue_depths(e->context);

      break;

    default:
      ERROR("unknown event type\n");
      exit(-1);
  }
}

void sim_event_print(sim_event_t* e, FILE* f) {
  fprintf(f, "event %lu time %lf %s ",
          e->id, e->timestamp,
          e->type == SIM_EVENT_PERIODIC_TASK_ARRIVAL ? "PERIODIC_TASK_ARRIVAL" :
          e->type == SIM_EVENT_SPORADIC_JOB_ARRIVAL  ? "SPORADIC_JOB_ARRIVAL" :
          e->type == SIM_EVENT_APERIODIC_JOB_ARRIVAL  ? "APERIODIC_JOB_ARRIVAL" :
          e->type == SIM_EVENT_JOB_DONE ? "JOB_DONE" :
          e->type == SIM_EVENT_TIMER ? "TIMER" :
          e->type == SIM_EVENT_PRINT_STATS ? "PRINT_STATS" :
          e->type == SIM_EVENT_PRINT_ALL ? "PRINT_ALL" :
          e->type == SIM_EVENT_PRINT_JOB_QUEUES ? "PRINT_JOB_QUEUES" :
          e->type == SIM_EVENT_PRINT_EVENT_QUEUE ? "PRINT_EVENT_QUEUE" :
          e->type == SIM_EVENT_DISPLAY_QUEUE_DEPTHS ? "DISPLAY_QUEUE_DEPTHS" :
          "UNKNOWN");
  switch (e->type) {
    case SIM_EVENT_PERIODIC_TASK_ARRIVAL:
    case SIM_EVENT_SPORADIC_JOB_ARRIVAL:
    case SIM_EVENT_APERIODIC_JOB_ARRIVAL:
    case SIM_EVENT_JOB_DONE:
      sim_job_print(e->job, f);
      break;
    default:
      // nothing to do
      break;
  }
}

void sim_event_complete(sim_event_t* e) {
  e->job = NULL;
}

