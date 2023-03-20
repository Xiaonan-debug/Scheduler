/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "debug.h"
#include "event.h"
#include "eventqueue.h"
#include "scheduler.h"


// control debugging prints throughout this file
#if DEBUG_CONTEXT
#define DEBUG(fmt, args...) DEBUG_PRINT("context: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define ERROR(fmt, args...) ERROR_PRINT("context: " fmt, ##args)
#define INFO(fmt, args...)  INFO_PRINT("context: " fmt, ##args)


/* Internal helper functions */

// logs status of all queues
static void sim_context_write_queue_info(sim_context_t* c) {
  fprintf(c->queuelen_file, "%lf %lu %lf %lf %lu %lf %lf\n",
          sim_context_get_current_time(c),
          c->realtime_queue.num_jobs,
          sim_job_queue_get_total_time(&c->realtime_queue),
          sim_job_queue_get_total_remaining_time(&c->realtime_queue),
          c->aperiodic_queue.num_jobs,
          sim_job_queue_get_total_time(&c->aperiodic_queue),
          sim_job_queue_get_total_remaining_time(&c->aperiodic_queue));
  fflush(c->queuelen_file);
}


/* Public functions */

int sim_context_init(sim_context_t* context, char* sched_name, double quantum) {

  // intialize simulation state
  memset(context, 0, sizeof(*context));
  context->quantum = quantum;

  // connect to the user-selected scheduler
  if (!(context->scheduler = sim_sched_find(sched_name))) {
    ERROR("cannot find scheduler named %s\n", sched_name);
    return -1;
  }

  // open log files
  if (!(context->queuelen_file = fopen("logs/queuesim.queuelen.out", "w"))) {
    ERROR("failed to open queue length file\n");
    return -1;
  }
  if (!(context->log_file = fopen("logs/queuesim.log.out", "w"))) {
    ERROR("failed to open log file\n");
    return -1;
  }
  if (!(context->job_file = fopen("logs/queuesim.job.out", "w"))) {
    ERROR("failed to open job file\n");
    return -1;
  }

  // initialize the queues
  sim_event_queue_init(&context->event_queue);
  sim_job_queue_init(&context->realtime_queue);
  sim_job_queue_init(&context->aperiodic_queue);

  return 0;
}

void sim_context_deinit(sim_context_t* c) {
  fclose(c->queuelen_file);
  fclose(c->log_file);
  fclose(c->job_file);
}

int sim_context_load_events(sim_context_t* context, char* filename) {

  FILE* in = fopen(filename, "r");
  if (!in) {
    ERROR("Can't read events from %s\n", filename);
    return -1;
  }

  // read in events from file
  char b[1024];
  char cmd[1024];
  while (!feof(in)) {
    char* buf = b;
    sim_event_t* event = NULL;
    sim_job_t* job     = NULL;

    // read in line from file
    if (!fgets(buf, 1024, in)) {
      break;
    }
    if (strlen(buf) == 0) {
      continue;
    }

    // skip over spaces
    while (isspace(*buf)) {
      buf++;
    }
    if (*buf == 0) {
      continue;
    }

    // skip over comments
    if (toupper(buf[0]) == '#') {
      continue;
    }

    // read in command from the line
    // for each type of command:
    //  * read further arguments
    //  * create job if needed
    //  * create an event and add it to queue
    double timestamp = 0;
    sscanf(buf, "%lf %s", &timestamp, cmd);

    if (!strcasecmp(cmd, "APERIODIC_JOB_ARRIVAL")) {
      double size       = 0;
      uint64_t priority = 0;
      sscanf(buf, "%lf %s %lf %lu", &timestamp, cmd, &size, &priority);

      if (!(job = sim_job_create(SIM_JOB_APERIODIC,
                                 timestamp,
                                 size,
                                 size,
                                 priority,
                                 0,
                                 0,
                                 1,
                                 0))) {
        ERROR("failed to allocate job\n");
        return -1;
      }

      if (!(event = sim_event_create(timestamp,
                                     context,
                                     SIM_EVENT_APERIODIC_JOB_ARRIVAL,
                                     job))) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, event);

      continue;
    }

    if (!strcasecmp(cmd, "SPORADIC_JOB_ARRIVAL")) {
      double size     = 0;
      double deadline = 0;
      sscanf(buf, "%lf %s %lf %lf", &timestamp, cmd, &size, &deadline);

      if (!(job = sim_job_create(SIM_JOB_SPORADIC,
                                 timestamp,
                                 size,
                                 size,
                                 0,
                                 0,
                                 0,
                                 1,
                                 deadline))) {
        ERROR("failed to allocate job\n");
        return -1;
      }

      if (!(event = sim_event_create(timestamp,
                                     context,
                                     SIM_EVENT_SPORADIC_JOB_ARRIVAL,
                                     job))) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, event);

      continue;
    }

    if (!strcasecmp(cmd, "PERIODIC_TASK_ARRIVAL")) {
      double deadline = 0;
      double size     = 0;
      int numiters    = 0;
      sscanf(buf, "%lf %s %lf %lf %d", &timestamp, cmd, &deadline, &size, &numiters);

      if (!(job = sim_job_create(SIM_JOB_PERIODIC,
                                 timestamp,
                                 size,
                                 size,
                                 0,
                                 0,
                                 deadline,
                                 numiters,
                                 timestamp + deadline))) {
        ERROR("failed to allocate job\n");
        return -1;
      }

      if (!(event = sim_event_create(timestamp,
                                     context,
                                     SIM_EVENT_PERIODIC_TASK_ARRIVAL,
                                     job))) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, event);

      continue;
    }

    if (!strcasecmp(cmd, "PRINT_ALL")) {
      sscanf(buf, "%lf %s", &timestamp, cmd);

      if (!(event = sim_event_create(timestamp,
                                     context,
                                     SIM_EVENT_PRINT_ALL,
                                     NULL))) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, event);

      continue;
    }

    if (!strcasecmp(cmd, "PRINT_STATS")) {
      sscanf(buf, "%lf %s", &timestamp, cmd);

      if (!(event = sim_event_create(timestamp,
                                     context,
                                     SIM_EVENT_PRINT_STATS,
                                     NULL))) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, event);

      continue;
    }

    if (!strcasecmp(cmd, "PRINT_JOB_QUEUES")) {
      sscanf(buf, "%lf %s", &timestamp, cmd);

      if (!(event = sim_event_create(timestamp,
                                     context,
                                     SIM_EVENT_PRINT_JOB_QUEUES,
                                     NULL))) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, event);

      continue;
    }

    if (!strcasecmp(cmd, "PRINT_EVENT_QUEUE")) {
      sscanf(buf, "%lf %s", &timestamp, cmd);

      if (!(event = sim_event_create(timestamp,
                                     context,
                                     SIM_EVENT_PRINT_EVENT_QUEUE,
                                     NULL))) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, event);

      continue;
    }

    if (!strcasecmp(cmd, "DISPLAY_QUEUE_DEPTHS")) {
      sscanf(buf, "%lf %s", &timestamp, cmd);

      if (!(event = sim_event_create(timestamp,
                                     context,
                                     SIM_EVENT_DISPLAY_QUEUE_DEPTHS,
                                     NULL))) {
        ERROR("failed to allocate event\n");
        return -1;
      }

      sim_event_queue_post(&context->event_queue, event);

      continue;
    }

  }

  // close workload file
  fclose(in);
  return 0;
}

int sim_context_begin(sim_context_t* context) {
  return sim_sched_init(context->scheduler, context);
}

void sim_context_print_stats(sim_context_t* c, FILE* f) {
  fprintf(f, "--------------------------------------------------------------------------------\n");
  fprintf(f, "Statistics for time %lf\n\n", sim_context_get_current_time(c));

  fprintf(f, "number of periodic tasks:               %lu\n", c->num_periodic_tasks);
  fprintf(f, "number of sporadic jobs:                %lu\n", c->num_sporadic_jobs);
  fprintf(f, "number of aperiodic jobs:               %lu\n\n", c->num_aperiodic);

  fprintf(f, "number of timer interrupts:             %lu\n\n", c->num_timer_interrupts);

  if (c->num_periodic_tasks != 0) {
    fprintf(f, "number of rejected periodic tasks:      %lu\n", c->num_periodic_tasksrejected);
    fprintf(f, "number of periodic jobs:                %lu\n", c->num_periodic_jobs);
    fprintf(f, "number of missing periodic jobs:        %lu\n", c->num_periodic_misses);
    fprintf(f, "average miss size:                      %lf\n",
            c->num_periodic_misses == 0 ? 0 : (c->sum_periodic_misssize / c->num_periodic_misses));
    fprintf(f, "stddev miss size:                       %lf\n",
            sqrt((c->num_periodic_misses <
                  2 ? 0 : (c->sum2_periodic_misssize -
                           (c->sum_periodic_misssize * c->sum_periodic_misssize / c->num_periodic_misses)) /
                  (c->num_periodic_misses - 1))));
    fprintf(f, "average miss ratio:                     %lf\n",
            c->num_periodic_misses == 0 ? 0 : (c->sum_periodic_missratio / c->num_periodic_misses));
    fprintf(f, "stddev miss ratio:                      %lf\n\n",
            sqrt((c->num_periodic_misses <
                  2 ? 0 : (c->sum2_periodic_missratio -
                           (c->sum_periodic_missratio * c->sum_periodic_missratio / c->num_periodic_misses)) /
                  (c->num_periodic_misses - 1))));
  }

  if (c->num_sporadic_jobs != 0) {
    fprintf(f, "number of rejected sporadic jobs:       %lu\n", c->num_sporadic_jobsrejected);
    fprintf(f, "number of missing sporadic jobs:        %lu\n", c->num_sporadic_misses);
    fprintf(f, "average miss size:                      %lf\n",
            c->num_sporadic_misses == 0 ? 0 : (c->sum_sporadic_misssize / c->num_sporadic_misses));
    fprintf(f, "stddev miss size:                       %lf\n",
            sqrt((c->num_sporadic_misses <
                  2 ? 0 : (c->sum2_sporadic_misssize -
                           (c->sum_sporadic_misssize * c->sum_sporadic_misssize / c->num_sporadic_misses)) /
                  (c->num_sporadic_misses - 1))));
    fprintf(f, "average miss ratio:                     %lf\n",
            c->num_sporadic_misses == 0 ? 0 : (c->sum_sporadic_missratio / c->num_sporadic_misses));
    fprintf(f, "stddev miss size:                       %lf\n\n",
            sqrt((c->num_sporadic_misses <
                  2 ? 0 : (c->sum2_sporadic_missratio -
                           (c->sum_sporadic_missratio * c->sum_sporadic_missratio / c->num_sporadic_misses)) /
                  (c->num_sporadic_misses - 1))));
  }

  if (c->num_aperiodic != 0) {
    fprintf(f, "average turnaround time:                %lf\n",
            c->num_aperiodic == 0 ? 0 : (c->sum_resptime / c->num_aperiodic));
    fprintf(f, "stddev turnaround time:                 %lf\n",
            sqrt((c->num_aperiodic <
                  2 ? 0 : (c->sum2_resptime - (c->sum_resptime * c->sum_resptime / c->num_aperiodic)) /
                  (c->num_aperiodic - 1))));
    fprintf(f, "average slowdown:                       %lf\n",
            c->num_aperiodic == 0 ? 0 : (c->sum_slowdown / c->num_aperiodic));
    fprintf(f, "stddev slowdown:                        %lf\n",
            sqrt((c->num_aperiodic <
                  2 ? 0 : (c->sum2_slowdown - (c->sum_slowdown * c->sum_slowdown / c->num_aperiodic)) /
                  (c->num_aperiodic - 1))));
  }
  fprintf(f, "--------------------------------------------------------------------------------\n");
}

void sim_context_print_job_queues(sim_context_t* c, FILE* f) {
  fprintf(f, "REALTIME QUEUE\n====================\n");
  sim_job_queue_print(&c->realtime_queue, f);
  fprintf(f, "APERIODIC QUEUE\n====================\n");
  sim_job_queue_print(&c->aperiodic_queue, f);
}

void sim_context_print_event_queue(sim_context_t* c, FILE* f) {
  fprintf(f, "EVENT QUEUE\n====================\n");
  sim_event_queue_print(&c->event_queue, f);
}

void sim_context_print_all(sim_context_t* c, FILE* f) {
  sim_context_print_stats(c, f);
  sim_context_print_job_queues(c, f);
  sim_context_print_event_queue(c, f);
}

void sim_context_display_queue_depths(sim_context_t* context) {
  system("./tools/plot_show.pl");
}

void sim_context_inform_job_done(sim_context_t* c, sim_job_t* job) {
  double turnaroundtime = sim_context_get_current_time(c) - job->arrival_time;
  double slowdown       = turnaroundtime / (job->size);
  bool missed = false;

  if (job->type == SIM_JOB_APERIODIC) {
    c->sum_resptime  += turnaroundtime;
    c->sum2_resptime += turnaroundtime * turnaroundtime;
    c->sum_slowdown  += slowdown;
    c->sum2_slowdown += slowdown * slowdown;
  } else {
    if (job->deadline < sim_context_get_current_time(c)) {
      missed = true;
      double misssize       = sim_context_get_current_time(c) - job->deadline;
      double avail_interval = job->deadline - job->arrival_time;
      double missratio      = misssize / avail_interval;
      if (job->type == SIM_JOB_SPORADIC) {
        c->num_sporadic_misses++;
        c->sum_sporadic_misssize   += misssize;
        c->sum2_sporadic_misssize  += misssize * misssize;
        c->sum_sporadic_missratio  += missratio;
        c->sum2_sporadic_missratio += missratio * missratio;
      } else {
        c->num_periodic_misses++;
        c->sum_periodic_misssize   += misssize;
        c->sum2_periodic_misssize  += misssize * misssize;
        c->sum_periodic_missratio  += missratio;
        c->sum2_periodic_missratio += missratio * missratio;
      }
    }
  }

  sim_context_write_queue_info(c);

  fprintf(c->log_file, "%lf DONE ", sim_context_get_current_time(c));
  sim_job_print(job, c->log_file);
  fprintf(c->log_file, "\n");

  if (job->type == SIM_JOB_APERIODIC) {
    fprintf(c->job_file, "%lf APERIODIC %lf %lf %lf # ",
            sim_context_get_current_time(c),
            job->size,
            turnaroundtime,
            slowdown);
    sim_job_print(job, c->job_file);
    fprintf(c->job_file, "\n");
  } else if (job->type == SIM_JOB_SPORADIC) {
    fprintf(c->job_file, "%lf %s %lf %lf %lf %lf # ",
            sim_context_get_current_time(c),
            missed ? "SPORADIC_MISS" : "SPORADIC_HIT",
            job->size,
            turnaroundtime,
            slowdown,
            job->deadline);
    sim_job_print(job, c->job_file);
    fprintf(c->job_file, "\n");
  } else if (job->type == SIM_JOB_PERIODIC) {
    fprintf(c->job_file, "%lf %s %lf %lf %lf %lf # ",
            sim_context_get_current_time(c),
            missed ? "PERIODIC_MISS" : "PERIODIC_HIT",
            job->size,
            turnaroundtime,
            slowdown,
            job->deadline);
    sim_job_print(job, c->job_file);
    fprintf(c->job_file, "\n");
  }
}

void sim_context_inform_job_arrival(sim_context_t* c, sim_job_t* job) {
  if (job->type == SIM_JOB_APERIODIC) {
    c->num_aperiodic++;
  } else if (job->type == SIM_JOB_PERIODIC) {
    // this should not occur
    c->num_periodic_jobs++;
  } else {
    c->num_sporadic_jobs++;
  }
  sim_context_write_queue_info(c);
  fprintf(c->log_file, "%lf JOB_ARRIVAL ", sim_context_get_current_time(c));
  sim_job_print(job, c->log_file);
  fprintf(c->log_file, "\n");
}

void sim_context_inform_task_arrival(sim_context_t* c, sim_job_t* job) {
  if (job->first_arrival) {
    c->num_periodic_tasks++;
  }
  c->num_periodic_jobs++;
  sim_context_write_queue_info(c);
  fprintf(c->log_file, "%lf TASK_ARRIVAL ", sim_context_get_current_time(c));
  sim_job_print(job, c->log_file);
  fprintf(c->log_file, "\n");
}

void sim_context_inform_task_acceptance(sim_context_t* c, sim_job_t* job, sim_sched_acceptance_t rc) {
  if (!job->first_arrival) {
    return;
  }

  // This must be periodic
  if (rc == SIM_SCHED_REJECT) {
    c->num_periodic_tasksrejected++;
  }

  sim_context_write_queue_info(c);

  fprintf(c->log_file, "%lf %s ", sim_context_get_current_time(c),
          (rc == SIM_SCHED_REJECT ? " PERIODIC_TASK_REJECTED " : " PERIODIC_TASK_ACCEPTED "));
  sim_job_print(job, c->log_file);
  fprintf(c->log_file, "\n");

  fprintf(c->job_file, "%lf %s ", sim_context_get_current_time(c),
          (rc == SIM_SCHED_REJECT ? " PERIODIC_TASK_REJECTED " : " PERIODIC_TASK_ACCEPTED "));
  sim_job_print(job, c->job_file);
  fprintf(c->job_file, "\n");
}

void sim_context_inform_job_acceptance(sim_context_t* c, sim_job_t* job, sim_sched_acceptance_t rc) {
  char* acc = NULL;
  if (rc == SIM_SCHED_REJECT) {
    acc = "REJECTED";
    if (job->type != SIM_JOB_APERIODIC) {
      c->num_sporadic_jobsrejected++;
    }
  } else {
    acc = "ACCEPTED";
  }

  char* jt = NULL;
  if (job->type == SIM_JOB_APERIODIC) {
    jt = "APERIODIC";
  } else if (job->type == SIM_JOB_SPORADIC) {
    jt = "SPORADIC";
  } else {
    jt = "PERIODIC";
  }

  sim_context_write_queue_info(c);

  fprintf(c->log_file, "%lf %s_JOB_%s ", sim_context_get_current_time(c), jt, acc);
  sim_job_print(job, c->log_file);
  fprintf(c->log_file, "\n");

  fprintf(c->job_file, "%lf %s_JOB_%s # ", sim_context_get_current_time(c), jt, acc);
  sim_job_print(job, c->job_file);
  fprintf(c->job_file, "\n");
}

void sim_context_inform_timer_interrupt(sim_context_t* c) {
  c->num_timer_interrupts++;

  sim_context_write_queue_info(c);

  fprintf(c->log_file, "%lf TIMER\n", sim_context_get_current_time(c));
}


void sim_context_dispatch_event(sim_context_t* c, sim_event_t* e) {
  sim_event_dispatch(e);
  sim_event_complete(e);
  free(e);
}

double sim_context_get_current_time(sim_context_t* c) {
  return c->event_queue.curtime;
}

sim_event_t* sim_context_get_next_event(sim_context_t* context) {
  return sim_event_queue_get_earliest_event(&context->event_queue);
}

