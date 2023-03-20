#!/usr/bin/perl -w

#
# Queueing/Scheduling Lab
#
# Copyright (c) 2019 Peter Dinda
#
# Original Queuesim tool is Copyright (c) 2005 Peter Dinda
#
#


use Gen;

$#ARGV==3 or $#ARGV==4 or die <<END
usage: make_arrivals.pl maxtime [expexp mu_arr mu_size]|[exppareto mu_arr alpha_size] [graph]

maxtime - how long an interval of time to make arrivals

expexp mu_arr mu_size

Poisson arrivals of exponential sized jobs.  The two parameters are
the mean interarrival time and the mean size.

exppareto mu_arr alpha_size

Poisson arrivals of Pareto-sized jobs.  The two parameters are the
mean interarrival time and the alpha parameter for the size distribution.

In all cases, each job includes a static priority chosen via 
a uniform random distribution from [0,99]
    
    graph - shows contention plot at end (only meaningful for purely
            aperiodic job stream)					     

The following environment variables affect the results:

    SEED=int
       random number seed (otherwise the Perl default is used)

    APERIODIC=p:l:u
       Probability p that job is aperiodic (default 1.0)
       Static priority of aperiodic jobs  chosen as 
       integer from uniform distribution from [l,b) (default [0,100])
    
    SPORADIC=p:a:b 
       Probability p that job is sporadic (default 0.0)
       Deadline of job is randomly chosen from a
       uniform distribution from [a*size, b*size) (default 2:8)

    PERIODIC=p:a:b:c:d
       Probability p that job is periodic task (default 0.0)
       Period of the job is randomly chosen from a
       uniform distribution from [a*size, b*size) (default 2:8)
       Number of iterations is randomly chosen, from a
       uniform distribution from [c,d) (default 10:100)
END
;

$graph=$#ARGV==4;

$tmax=shift;
$type=shift;
$p1=shift;
$p2=shift;

if (defined $ENV{SEED}) { 
    $seed = $ENV{SEED};
} else {
    $seed = time();
}

if (defined($ENV{APERIODIC})) {
    ($ap, $al, $au) = split(/:/,$ENV{APERIODIC});
} else {
    ($ap, $al, $au) = (1.0, 0, 100);
}

if (defined($ENV{SPORADIC})) {
    ($sp, $sdl, $sdh) = split(/:/,$ENV{SPORADIC});
} else {
    ($sp, $sdl, $sdh) = (0.0,2.0,8.0);
}

if (defined($ENV{PERIODIC})) {
    ($pp, $ppl, $pph, $pil, $pih) = split(/:/,$ENV{PERIODIC});
} else {
    ($pp, $ppl, $pph, $pil, $pih) = (0.0,2.0,8.0,10,100);
}

($ap + $sp + $pp) == 1.0 or die "Probabilities do not sum to 1.0\n";

print STDERR "Generating $tmax seconds of arrivals using $type $p1 $p2 with seed $seed\n\n";

print STDERR "Periodic\n";
print STDERR "      probability          $pp\n";
print STDERR "      period multiplier    [$ppl, $pph)\n";
print STDERR "      iterations           [$pil, $pih)\n\n";
print STDERR "Sporadic\n";
print STDERR "      probability          $sp\n";
print STDERR "      deadline multiplier  [$sdl, $sdh)\n\n";
print STDERR "Aperiodic\n";
print STDERR "      probability          $ap\n";
print STDERR "      static priorities    [$al, $au)\n\n";

InitRand($seed);


if ($type eq "expexp") { 
  @arrivals = GenerateArrivalsExpExpRate($tmax,$p1,0,$p2,0);
} elsif ($type eq "exppareto") {
  @arrivals = GenerateArrivalsExpParetoRate($tmax,$p1,0,$p2,0);
} else {
  die "Unknown type $type\n";
}


$a_count=0;
$s_count=0;
$p_count=0;

foreach $arrivalref (@arrivals) {
    ($arrival_time, $size) = ($arrivalref->[0],$arrivalref->[1]);
    $draw = rand(1.0);
    if ($draw<$ap) {
	$priority = int($al) + int(rand(int($au)-int($al)));
	print "$arrival_time APERIODIC_JOB_ARRIVAL $size $priority\n";
	$a_count++;
    } elsif ($draw<($ap+$sp)) {
	$dmul = $sdl + rand($sdh-$sdl);
	$deadline = $arrival_time + $size*$dmul;
	print "$arrival_time SPORADIC_JOB_ARRIVAL $size $deadline\n";
	$s_count++;
    } else {
	$pmul = $ppl + rand($pph-$ppl);
	$period = $size*$pmul;
	$iters = int($pil) + int(rand(int($pih)-int($pil)));
	print "$arrival_time PERIODIC_TASK_ARRIVAL $period $size $iters\n";
	$p_count++;
    }
}

print STDERR "Generated\n";
print STDERR "      periodic tasks       $p_count\n";
print STDERR "      sporadic jobs        $s_count\n";
print STDERR "      aperiodic jobs       $a_count\n\n";


if ($graph) {
    if ($ap < 1.0) {
	print STDERR "contention graph is only meaningful for purely aperiodic job stream\n";
    } else {
	@contention=IntegrateArrivals($tmax*1.5,1,@arrivals);
	open (FILE, ">", "logs/arrival_contention_plot.out");
	map {print FILE $_, "\n"; } @contention;
	close(FILE);
	
	open (GNUPLOT, "|gnuplot");
	GNUPLOT->autoflush(1);
	print GNUPLOT "set xlabel 'time'\nset ylabel 'contention'\nplot 'logs/arrival_contention_plot.out' title 'contention' with lines\n";
	print STDERR "press return to continue\n";
	<STDIN>;
	close(GNUPLOT);
    }
}


