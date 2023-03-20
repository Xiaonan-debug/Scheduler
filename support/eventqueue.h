/*
 *  Queueing/Scheduling Lab
 *
 *  Copyright (c) 2022 Peter Dinda, Branden Ghena
 *
 *  Original Queuesim tool is Copyright (c) 2005 Peter Dinda
 */
#pragma once

#include "list.h"


// forward declaration to avoid header dependency
typedef struct sim_event sim_event_t;


// nothing in this struct may be modified by schedulers
typedef struct sim_event_queue {
  // current point in time that the event queue has reached
  double curtime;

  // Will be maintained in sorted order (O(n) insert, O(1) remove)
  // A much smarter structure would be priority heap (O(lg n) insert, O(1) remove)
  struct list_head list;

} sim_event_queue_t;


// initialize the event queue
void sim_event_queue_init(sim_event_queue_t* eq);

// place an event into the queue for the first time
void sim_event_queue_post(sim_event_queue_t* eq, sim_event_t* e);

// change an event that is already in the queue (for example, timestamp change)
void sim_event_queue_update(sim_event_queue_t* eq, sim_event_t* e);

// delete an event from the queue
void sim_event_queue_delete(sim_event_queue_t* eq, sim_event_t* e);

// print the entire event queue
void sim_event_queue_print(sim_event_queue_t* eq, FILE* dest);

// return earliest event in the queue
sim_event_t* sim_event_queue_get_earliest_event(sim_event_queue_t* eq);

