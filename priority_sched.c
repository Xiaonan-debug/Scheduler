// Scheduler implementation for CS343

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "context.h"
#include "event.h"
#include "job.h"
#include "jobqueue.h"
#include "scheduler.h"

// Enable debugging for this scheduler? 1=True
// Be sure to rename this for each scheduler
#define DEBUG_PRIORITY_SCHED 1

#if DEBUG_PRIORITY_SCHED
#define DEBUG(fmt, args...) DEBUG_PRINT("priority_sched: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define ERROR(fmt, args...) ERROR_PRINT("priority_sched: " fmt, ##args)
#define INFO(fmt, args...)  INFO_PRINT("priority_sched: " fmt, ##args)


// Struct definition that holds state for this scheduler
typedef struct sched_state {
  sim_sched_t* sim;
  bool busy;
  sim_event_t* current_event;
} sched_state_t;


// Initialization for this scheduler
static int init(void* state, sim_context_t* context) {
  sched_state_t* s = (sched_state_t*)state;

  // initially, nothing is scheduled
  s->busy = false;

  return 0;
}

// job with higher priority should be executed first
int find_higher_order(sim_job_t* lhs, sim_job_t* rhs) {
    if(lhs->static_priority > rhs->static_priority) {
        return -1;
    }
    else if(lhs->static_priority < rhs->static_priority) {
        return 1;
    }
    else {
        return 0;
    }
}

// Function called when an aperiodic job arrives
static sim_sched_acceptance_t aperiodic_job_arrival(void*          state,
                                                    sim_context_t* context,
                                                    double         current_time,
                                                    sim_job_t*     job) {

  sched_state_t* s = (sched_state_t*)state;
  
  DEBUG("Time[%lf] ARRIVAL, job %lu, size %lf, static priority %lu \n", current_time, job->id, job->size, job->static_priority);

  // add the job to the queue in order of job's static priority
  int (* compare)(sim_job_t* lhs, sim_job_t* rhs);
  compare = &find_higher_order;
  
  if(s->busy) {
    sim_job_t* cur_job = sim_job_queue_peek(&context->aperiodic_queue);
    sim_job_set_remaining_size(cur_job, s->current_event->timestamp - current_time);
    sim_job_queue_enqueue_in_order(&context->aperiodic_queue, job, compare);

    DEBUG("cur_job %lu has %lf left, job %lu has %lf left\n", s->current_event->job->id, cur_job->remaining_size, job->id, job->remaining_size);
    if(job->static_priority > cur_job->static_priority) {
      DEBUG("Switching from job %lu to job %lu according to static priority\n", cur_job->id, job->id);
      sim_event_t* new_event = sim_event_create(current_time + job->remaining_size, context, SIM_EVENT_JOB_DONE, job);
      if (!new_event) {
        ERROR("failed to allocate event\n");
        return SIM_SCHED_REJECT;
      }
      sim_event_queue_delete(&context->event_queue, s->current_event);
      s->current_event = new_event;
      sim_event_queue_post(&context->event_queue, new_event);
    }
  }
  // only start a new job if there is not one already running
  else {
    DEBUG("starting new job %lu because we are idle\n", job->id);
    sim_job_queue_enqueue_in_order(&context->aperiodic_queue, job, compare);
    // create an event for when this job is done
    sim_event_t* event = sim_event_create(current_time + job->remaining_size,
                                          context,
                                          SIM_EVENT_JOB_DONE,
                                          job);
    if (!event) {
      ERROR("failed to allocate event\n");
      return SIM_SCHED_REJECT;
    }

    s->current_event = event;
    // post the event
    sim_event_queue_post(&context->event_queue, event);
    s->busy = true;
  }

  return SIM_SCHED_ACCEPT;
}


// Function called when a job is finished
static void job_done(void*          state,
                     sim_context_t* context,
                     double         current_time,
                     sim_job_t*     job) {

  sched_state_t* s = (sched_state_t*)state;

  DEBUG("TIME[%lf] DONE, job %lu\n", current_time, job->id);

  // remove the job from the job queue
  sim_job_queue_remove(&context->aperiodic_queue, job);
  s->busy = false;

  // mark the job as completed
  if (sim_job_complete(context, job)) {
    ERROR("failed to complete job\n");
    return;
  }

  // check if there is a next job at the front of the queue
  sim_job_t* next = sim_job_queue_peek(&context->aperiodic_queue);

  // if there is no job, we're done here
  if (!next) {
    DEBUG("no more jobs in queue\n");
    return;
  }

  // there is a job, so let's schedule it
  DEBUG("%lf switching to job %lu\n", current_time, next->id);
  sim_event_t* event = sim_event_create(current_time + next->remaining_size,
                                        context,
                                        SIM_EVENT_JOB_DONE,
                                        next);
  if (!event) {
    ERROR("failed to allocate event\n");
    return;
  }

  // post the event
  sim_event_queue_post(&context->event_queue, event);
  s->current_event = event;
  s->busy = true;
}


// Function called when a timeslice expires
static void timer_interrupt(void*          state,
                            sim_context_t* context,
                            double         current_time) {
  // nothing to do in this scheduler
  DEBUG("ignoring timer interrupt\n");
}


/* Scheduler configuration */

// Map of the generic scheduler operations into specific function calls in this scheduler
// Each of these lines should be a function pointer to a function in this file
static sim_sched_ops_t ops = {
  .init = init,

  // Only aperiodic jobs will occur in this lab
  .periodic_job_arrival  = NULL,
  .sporadic_job_arrival  = NULL,
  .aperiodic_job_arrival = aperiodic_job_arrival,

  // job status calls
  .job_done        = job_done,
  .timer_interrupt = timer_interrupt,
};

// Register this scheduler with the simulation
// All functions with the `constructor` attribute run _before_ `main()` is called
// Note that the name of this function MUST be unique
__attribute__((constructor)) void priority_sched_init() {
  sched_state_t* my_state = malloc(sizeof(sched_state_t));
  if (!my_state) {
    ERROR("cannot allocate scheduler state\n");
    return;
  }
  memset(my_state, 0, sizeof(sched_state_t));

  // IMPORTANT: the string here is the name of this scheduler and MUST match the expected name
  my_state->sim = sim_sched_register("priority_sched", my_state, &ops);
}