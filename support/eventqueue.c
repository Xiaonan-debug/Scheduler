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

#include "debug.h"
#include "event.h"
#include "eventqueue.h"


// control debugging prints throughout this file
#if DEBUG_EVENT_QUEUE
#define DEBUG(fmt, args...) DEBUG_PRINT("eventqueue: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define ERROR(fmt, args...) ERROR_PRINT("eventqueue: " fmt, ##args)
#define INFO(fmt, args...)  INFO_PRINT("eventqueue: " fmt, ##args)


/* Public functions */

void sim_event_queue_init(sim_event_queue_t* eq) {
  memset(eq, 0, sizeof(*eq));

  INIT_LIST_HEAD(&eq->list);
}

void sim_event_queue_post(sim_event_queue_t* eq, sim_event_t* e) {
  if (list_empty(&eq->list)) {
    // DEBUG("empty list insert new %lf\n",e->timestamp);
    list_add(&e->node, &eq->list);
    return;
  } else {
    struct list_head* cur;
    list_for_each(cur, &eq->list) {
      sim_event_t* cur_event = list_entry(cur, sim_event_t, node);
      // DEBUG("compare ts new %lf to old %lf\n", e->timestamp,cur_event->timestamp);
      if (e->timestamp < cur_event->timestamp) {
        // DEBUG("non-empty list insert new %lf before %lf\n",e->timestamp,cur_event->timestamp);
        list_add_tail(&e->node, cur);
        return;
      }
    }
    // DEBUG("non-empty list insert new %lf at tail\n",e->timestamp);
    list_add_tail(&e->node, &eq->list);
    return;
  }
}

void sim_event_queue_delete(sim_event_queue_t* eq, sim_event_t* e) {
  list_del_init(&e->node);
}

void sim_event_queue_update(sim_event_queue_t* eq, sim_event_t* e) {
  sim_event_queue_delete(eq, e);
  sim_event_queue_post(eq, e);
}

void sim_event_queue_print(sim_event_queue_t* eq, FILE* f) {
  fprintf(f, "event queue curtime %lf, events follow:\n", eq->curtime);
  struct list_head* cur;
  list_for_each(cur, &eq->list) {
    sim_event_t* cur_event = list_entry(cur, sim_event_t, node);
    sim_event_print(cur_event, f);
    fprintf(f, "\n");
  }
}

sim_event_t* sim_event_queue_get_earliest_event(sim_event_queue_t* eq) {
  if (list_empty(&eq->list)) {
    return NULL;
  } else {
    sim_event_t* cur_event = list_entry(eq->list.next, sim_event_t, node);
    list_del_init(eq->list.next);
    eq->curtime = cur_event->timestamp;
    return cur_event;
  }
}

