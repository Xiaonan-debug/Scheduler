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
#define DEBUG_RR_SCHED 1

#if DEBUG_RR_SCHED
#define DEBUG(fmt, args...) DEBUG_PRINT("rr_sched: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define ERROR(fmt, args...) ERROR_PRINT("rr_sched: " fmt, ##args)
#define INFO(fmt, args...)  INFO_PRINT("rr_sched: " fmt, ##args)


// Struct definition that holds state for this scheduler
typedef struct sched_state {
  sim_sched_t* sim;
  bool busy;
  sim_event_t* current_event;
  sim_event_t* current_timer;
} sched_state_t;


// Initialization for this scheduler
static int init(void* state, sim_context_t* context) {
  sched_state_t* s = (sched_state_t*)state;

  // initially, nothing is scheduled
  s->busy = false;

  return 0;
}


// Function called when an aperiodic job arrives
static sim_sched_acceptance_t aperiodic_job_arrival(void*          state,
                                                    sim_context_t* context,
                                                    double         current_time,
                                                    sim_job_t*     job) {

  sched_state_t* s = (sched_state_t*)state;
  
  DEBUG("Time[%lf] ARRIVAL, job %lu, size %lf\n", current_time, job->id, job->size);
  DEBUG("job remaining size %lf\n", job->remaining_size);

  // add the job to the queue of jobs at the end
  sim_job_queue_enqueue(&context->aperiodic_queue, job);

  
  // only start a new job if there is not one already running
  if (!s->busy) {
    DEBUG("starting new job %lu because we are idle\n", job->id);

    sim_event_t* event = sim_event_create(current_time + job->remaining_size,
                                        context,
                                        SIM_EVENT_JOB_DONE,
                                        job);
    sim_event_t* timer = sim_event_create(current_time + context->quantum,
                                        context,
                                        SIM_EVENT_TIMER,
                                        job);
    if (!event || !timer) {
      ERROR("failed to allocate event\n");
      return SIM_SCHED_REJECT;
    }

    // post the event
    s->current_event = event;
    s->current_timer = timer;
    sim_event_queue_post(&context->event_queue, event);
    sim_event_queue_post(&context->event_queue, timer);
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
  sim_event_queue_delete(&context->event_queue, s->current_timer);

  // remove the job from the job queue
  sim_job_queue_remove(&context->aperiodic_queue, job);
  s->busy = false;

  // mark the job as completed
  if (sim_job_complete(context, job)) {
    ERROR("failed to complete job\n");
    return;
  }

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
                                        DEBUG("%lf switching to job %lf\n", current_time, event->timestamp);
  if (!event) {
    ERROR("failed to allocate event\n");
    return;
  }

  sim_event_t* timer = sim_event_create(current_time + context->quantum,
                                        context,
                                        SIM_EVENT_TIMER,
                                        next);
  if (!timer) {
    ERROR("failed to allocate event\n");
    return;
  }
  // post the event
  sim_event_queue_post(&context->event_queue, event);
  sim_event_queue_post(&context->event_queue, timer);
  s->busy = true;
  s->current_event = event;
  s->current_timer = timer;
  // We can only reschedule a new when a timeslice expires, not when a job is finished
  // The scheduler always provides a full timeslice, jobs may use less than their entire timeslice
  // We need to wait until the end of the current timeslice
  // Therefore, don't delete the "Timer" event of the job
}


// Function called when a timeslice expires
static void timer_interrupt(void*          state,
                            sim_context_t* context,
                            double         current_time) {
  sched_state_t* s = (sched_state_t*)state;
  if (s->busy) {
      // When a timeslice expires, remove current job from the jobqueue
      // Then add it to the end of the jobqueue
      sim_job_t* cur_job = sim_job_queue_dequeue(&context->aperiodic_queue);
      sim_job_set_remaining_size(cur_job, cur_job->remaining_size - context->quantum);
      sim_job_queue_enqueue(&context->aperiodic_queue, cur_job);
  
      DEBUG("%lf job %lu expires, remaining size %lf\n", current_time, cur_job->id, cur_job->remaining_size);

      // delete the "Job Done" event of the job
      sim_event_queue_delete(&context->event_queue, s->current_event);
  }


  // check if there is a next job at the front of the queue
  sim_job_t* next = sim_job_queue_peek(&context->aperiodic_queue);

  // if there is no job, we're done here
  if (!next) {
    DEBUG("no more jobs in queue\n");
    return;
  }

  // there is a job, so let's schedule it
  DEBUG("%lf switching to job %lu, remaining size %lf\n", current_time, next->id, next->remaining_size);
  sim_event_t* event = sim_event_create(current_time + next->remaining_size,
                                        context,
                                        SIM_EVENT_JOB_DONE,
                                        next);
  sim_event_t* timer = sim_event_create(current_time + context->quantum,
                                        context,
                                        SIM_EVENT_TIMER,
                                        next);
  if (!timer) {
    ERROR("failed to allocate event\n");
    return;
  }

  // post the event
  sim_event_queue_post(&context->event_queue, event);
  sim_event_queue_post(&context->event_queue, timer);
  s->busy = true;
  s->current_event = event;
  s->current_timer = timer;
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
__attribute__((constructor)) void rr_sched_init() {
  sched_state_t* my_state = malloc(sizeof(sched_state_t));
  if (!my_state) {
    ERROR("cannot allocate scheduler state\n");
    return;
  }
  memset(my_state, 0, sizeof(sched_state_t));

  // IMPORTANT: the string here is the name of this scheduler and MUST match the expected name
  my_state->sim = sim_sched_register("rr_sched", my_state, &ops);
}