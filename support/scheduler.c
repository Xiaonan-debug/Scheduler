/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "scheduler.h"


// internal list of registered schedulers
static struct list_head sched_list = LIST_HEAD_INIT(sched_list);


/* Public functions */

sim_sched_t* sim_sched_register(char*            name,
                                void*            state,
                                sim_sched_ops_t* ops) {

  sim_sched_t* s = malloc(sizeof(*s));
  if (!s) {
    return NULL;
  }

  memset(s, 0, sizeof(*s));

  strncpy(s->name, name, SIM_SCHED_NAME_MAX);
  s->name[SIM_SCHED_NAME_MAX - 1] = 0;

  s->state = state;
  s->ops   = ops;

  list_add_tail(&s->node, &sched_list);

  return s;
}

sim_sched_t* sim_sched_find(char* name) {
  struct list_head* cur;

  list_for_each(cur, &sched_list) {
    sim_sched_t* s = list_entry(cur, sim_sched_t, node);
    if (!strncmp(s->name, name, SIM_SCHED_NAME_MAX)) {
      return s;
    }
  }

  return NULL;
}

void sim_sched_list(FILE* o) {
  struct list_head* cur;

  list_for_each(cur, &sched_list) {
    sim_sched_t* s = list_entry(cur, sim_sched_t, node);
    fprintf(o, "  %s\n", s->name);
  }
}

int sim_sched_init(sim_sched_t* sched, sim_context_t* context) {
  return sched->ops->init(sched->state, context);
}

sim_sched_acceptance_t sim_sched_periodic_task_arrival(sim_sched_t*   sched,
                                                       sim_context_t* context,
                                                       double         current_time,
                                                       sim_job_t*     job) {
  return sched->ops->periodic_job_arrival(sched->state, context, current_time, job);
}

sim_sched_acceptance_t sim_sched_sporadic_job_arrival(sim_sched_t*   sched,
                                                      sim_context_t* context,
                                                      double         current_time,
                                                      sim_job_t*     job) {
  return sched->ops->sporadic_job_arrival(sched->state, context, current_time, job);
}

sim_sched_acceptance_t sim_sched_aperiodic_job_arrival(sim_sched_t*   sched,
                                                       sim_context_t* context,
                                                       double         current_time,
                                                       sim_job_t*     job) {
  return sched->ops->aperiodic_job_arrival(sched->state, context, current_time, job);
}

void sim_sched_job_done(sim_sched_t*   sched,
                        sim_context_t* context,
                        double         current_time,
                        sim_job_t*     job) {
  sched->ops->job_done(sched->state, context, current_time, job);
}

void sim_sched_timer_interrupt(sim_sched_t*   sched,
                               sim_context_t* context,
                               double         current_time) {
  sched->ops->timer_interrupt(sched->state, context, current_time);
}

