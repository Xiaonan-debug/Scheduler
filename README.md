CS343 Operating system project course at Northwestern


# Queueing/Scheduling Lab

Simple event-driven simulator for single queue scheduling.
Periodic, sporadic, and aperiodic jobs.  Single aperiodic
server model.

Copyright (c) 2022 Peter Dinda, Branden Ghena

Original Queuesim tool is Copyright (c) 2005 Peter Dinda


To build out of box:

```
$ make
```

To try an out of the box example:

```
$ ./queuesim fifo_all workloads/example.txt
```

This will show a plot and statistics for the job arrivals in `example.txt`.
All job types will be scheduled using FIFO/FCFS.

To see it event by event:

```
$ ./queuesim fifo_all workloads/example.txt single
```

Environment variables:

```
QUEUESIM_SEED=int         : random number seed (default is time(0))
QUEUESIM_QUANTUM=float    : scheduling quantum (default is 0.01 (10 ms))
```

# Scheduler
