/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "debug.h"
#include "event.h"
#include "job.h"

// control debugging prints throughout this file
#if DEBUG_JOB
#define DEBUG(fmt, args...) DEBUG_PRINT("job: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define ERROR(fmt, args...) ERROR_PRINT("job: " fmt, ##args)
#define INFO(fmt, args...)  INFO_PRINT("job: " fmt, ##args)


// monotonically increasing value used to assign job IDs
static uint64_t current_job_id = 0;


/* Internal helper functions */

// initialize an already allocated job
static void sim_job_init(sim_job_t*     job,
                         sim_job_type_t type,
                         double         arrival_time,
                         double         size,
                         double         remaining_size,
                         uint64_t       static_priority,
                         uint64_t       dynamic_priority,
                         double         period,
                         uint64_t       numiters,
                         double         deadline) {

  memset(job, 0, sizeof(*job));

  job->id = current_job_id++;

  job->type             = type;
  job->arrival_time     = arrival_time;
  job->size             = size;
  job->remaining_size   = remaining_size;
  job->static_priority  = static_priority;
  job->dynamic_priority = dynamic_priority;
  job->deadline         = deadline;
  job->period           = period;
  job->numiters         = numiters;
  job->first_arrival    = true;

  INIT_LIST_HEAD(&job->node);
}


/* Public functions */

sim_job_t* sim_job_create(sim_job_type_t type,
                          double         arrival_time,
                          double         size,
                          double         remaining_size,
                          uint64_t       static_priority,
                          uint64_t       dynamic_priority,
                          double         period,
                          uint64_t       numiters,
                          double         deadline) {

  sim_job_t* job = malloc(sizeof(*job));
  if (!job) {
    ERROR("cannot allocate job\n");
    return NULL;
  }

  sim_job_init(job,
               type,
               arrival_time,
               size,
               remaining_size,
               static_priority,
               dynamic_priority,
               period,
               numiters,
               deadline);

  return job;
}

int sim_job_complete(sim_context_t* context,
                     sim_job_t*     job) {
  if (job->type == SIM_JOB_PERIODIC) {
    // a periodic job needs to re-arrive
    job->first_arrival = false;
    job->numiters--;

    if (job->numiters == 0) {
      sim_job_destroy(job);
    } else {
      job->arrival_time = job->arrival_time + job->period;
      job->deadline     = job->deadline + job->period;

      // event either fires in the future when it's supposed to
      // or right now if we already passed that time
      double event_time = sim_context_get_current_time(context);
      if (event_time < job->arrival_time) {
        event_time = job->arrival_time;
      }

      sim_event_t* e = sim_event_create(event_time,
                                        context,
                                        SIM_EVENT_PERIODIC_TASK_ARRIVAL,
                                        job);

      if (!e) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, e);
    }
  } else {
    sim_job_destroy(job);
  }

  return 0;
}

void sim_job_destroy(sim_job_t* job) {
  free(job);
}

void sim_job_print(sim_job_t* job, FILE* f) {
  fprintf(f, "job %lu arrival %lf size %lf remaining size %lf ",
          job->id, job->arrival_time, job->size, job->remaining_size);

  switch (job->type) {
    case SIM_JOB_PERIODIC:
      fprintf(f, "periodic deadline %lf period %lf numiters %lu %s",
              job->deadline, job->period, job->numiters, job->first_arrival ? "first" : "");
      break;

    case SIM_JOB_SPORADIC:
      fprintf(f, "sporadic deadline %lf", job->deadline);
      break;

    case SIM_JOB_APERIODIC:
      fprintf(f, "aperiodic static priority %lu dynamic priority %lu",
              job->static_priority, job->dynamic_priority);
      break;
  }
}

void sim_job_set_remaining_size(sim_job_t* job, double remaining_size) {
  job->remaining_size = remaining_size;
}

void sim_job_set_dynamic_priority(sim_job_t* job, uint64_t dynamic_priority) {
  job->dynamic_priority = dynamic_priority;
}

