#!/usr/bin/perl -w

#
# Queueing/Scheduling Lab
#
# Copyright (c) 2019 Peter Dinda
#
# Original Queuesim tool is Copyright (c) 2005 Peter Dinda
#
#


use FileHandle;

open(GNUPLOT,"|gnuplot");
GNUPLOT->autoflush(1);

print GNUPLOT <<END
set xlabel "Time"
set ylabel "Number of jobs, remaining time"
plot "logs/queuesim.queuelen.out" using 1:2 title "num edf jobs" with linespoints, "logs/queuesim.queuelen.out" using 1:3 title "total size edf jobs" with linespoints, "logs/queuesim.queuelen.out" using 1:4 title "remaining size edf jobs" with linespoints, "logs/queuesim.queuelen.out" using 1:5 title "num aperiodic jobs" with linespoints, "logs/queuesim.queuelen.out" using 1:6 title "total size aperiodic jobs" with linespoints, "logs/queuesim.queuelen.out" using 1:7 title "remaining size aperiodic jobs" with linespoints

END
;

print STDERR "Hit Enter To Continue\n";
<STDIN>;

exit;


