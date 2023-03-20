/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "jobqueue.h"


// control debugging prints throughout this file
#if DEBUG_JOB_QUEUE
#define DEBUG(fmt, args...) DEBUG_PRINT("jobqueue: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define ERROR(fmt, args...) ERROR_PRINT("jobqueue: " fmt, ##args)
#define INFO(fmt, args...)  INFO_PRINT("jobqueue: " fmt, ##args)


/* Internal helper functions */

static int total_time(void* state, sim_job_t* job) {
  *(double*)state += job->size;
  return 0;
}

static int total_remaining_time(void* state, sim_job_t* job) {
  *(double*)state += job->remaining_size;
  return 0;
}


/* Public functions */

void sim_job_queue_init(sim_job_queue_t* jq) {
  memset(jq, 0, sizeof(*jq));
  INIT_LIST_HEAD(&jq->list);
}

void sim_job_queue_enqueue(sim_job_queue_t* jq, sim_job_t* job) {
  list_add_tail(&job->node, &jq->list);
  jq->num_jobs++;
}

void sim_job_queue_enqueue_before(sim_job_queue_t* jq, sim_job_t* job, sim_job_t* target) {
  list_add(&job->node, &target->node);
  jq->num_jobs++;
}

void sim_job_queue_enqueue_after(sim_job_queue_t* jq, sim_job_t* job, sim_job_t* target) {
  list_add_tail(&job->node, &target->node);
  jq->num_jobs++;
}

void sim_job_queue_enqueue_in_order(sim_job_queue_t* jq,
                                    sim_job_t* job,
                                    int (* compare)(sim_job_t* lhs, sim_job_t* rhs)) {

  if (list_empty(&jq->list)) {
    list_add(&job->node, &jq->list);
    jq->num_jobs++;
    return;
  }

  struct list_head* cur, * temp;

  list_for_each_safe(cur, temp, &jq->list) {
    sim_job_t* cur_job = list_entry(cur, sim_job_t, node);

    if (compare(job, cur_job) < 0) {
      // insert before this node
      list_add_tail(&job->node, cur);
      jq->num_jobs++;
      return;
    }
  }

  // if we got here, we need to put it the end
  list_add_tail(&job->node, &jq->list);
  jq->num_jobs++;
}

sim_job_t* sim_job_queue_peek(sim_job_queue_t* jq) {
  if (list_empty(&jq->list)) {
    return NULL;
  }

  struct list_head* cur = jq->list.next;

  return list_entry(cur, sim_job_t, node);
}

sim_job_t* sim_job_queue_dequeue(sim_job_queue_t* jq) {
  sim_job_t* j = sim_job_queue_peek(jq);

  sim_job_queue_remove(jq, j);

  return j;
}

void sim_job_queue_remove(sim_job_queue_t* jq, sim_job_t* j) {
  list_del_init(&j->node);
  jq->num_jobs--;
}

sim_job_t* sim_job_queue_search(sim_job_queue_t* jq,
                                int (* cond)(void* state, sim_job_t* job),
                                void* state) {
  struct list_head* cur, * temp;

  list_for_each_safe(cur, temp, &jq->list) {
    sim_job_t* j = list_entry(cur, sim_job_t, node);
    if (cond(state, j)) {
      return j;
    }
  }
  return NULL;
}

int sim_job_queue_map(sim_job_queue_t* jq,
                      int (* func)(void* state, sim_job_t* job),
                      void* state) {
  int rc = 0;
  struct list_head* cur, * temp;

  list_for_each_safe(cur, temp, &jq->list) {
    sim_job_t* j = list_entry(cur, sim_job_t, node);
    rc |= func(state, j);
  }
  return rc;
}

double sim_job_queue_get_total_time(sim_job_queue_t* jq) {
  double sum = 0;
  sim_job_queue_map(jq, total_time, (void*)&sum);
  return sum;
}

double sim_job_queue_get_total_remaining_time(sim_job_queue_t* jq) {
  double sum = 0;
  sim_job_queue_map(jq, total_remaining_time, (void*)&sum);
  return sum;
}

void sim_job_queue_print(sim_job_queue_t* jq, FILE* f) {
  fprintf(f, "job queue num_jobs %lu jobs follow:\n", jq->num_jobs);

  struct list_head* cur;

  list_for_each(cur, &jq->list) {
    sim_job_t* cur_job = list_entry(cur, sim_job_t, node);
    sim_job_print(cur_job, f);
    fprintf(f, "\n");
  }
}

