#
# Queueing/Scheduling Lab
#
# Copyright (c) 2019 Peter Dinda
#
# Original Queuesim tool is Copyright (c) 2005 Peter Dinda
#
#


package Gen;
require Exporter;

@ISA=qw(Exporter);
@EXPORT=qw(InitRand
           MakeTestcase
	   AppendTestcase
	   LoadTestcase
	   LoadTestcaseNumbers
	   GenerateArrivalsExpExpRate
	   GenerateArrivalsExpParetoRate
	   IntegrateArrivals);

use FileHandle;
use IPC::Open2;
use POSIX;

$matlabinited=0;

1;

sub InitRand {
    srand($_[0]);
}

#
# Return number from (0..1)
#
#
sub UniformRandNonZero {
  my $num=0;
  while ($num==0) { 
    $num=rand();
  }
  return $num;
}

#
# Return an exponentially distributed random number
# from a distribution with mean mu
#
sub ExpRand {
  my $mu=shift;
  die "FAIL: ExpRand with mu=$mu\n" if ($mu<=0);
  return -$mu*log(UniformRandNonZero());
}

#
# Return an pareto distributed random number
# from a distribution with shape parameter alpha
#
sub ParetoRand {
  my $alpha=shift;
  die "FAIL: ParetoRand with alpha=$alpha\n" if ($alpha<=0);
#  return (1.0/(1.0-UniformRandNonZero()))**(1.0/$alpha);
  return (1/(UniformRandNonZero()**(1.0/$alpha)));
}



#
# Generate arrivals from a poisson process
#  - exponentially distributed interarrival
#     mean starts at mu_arr_start and changes at
#     a rate mu_arr_rate
#  - exponentially distributed service times
#     mean starts at mu_size_start and changes at
#     a rate mu_size_rate
#  - uniform random distribution of priority 0-99
#
# The arrivals are returned in the form of 
# a list of references to 2-element (arrival time, size) lists
sub GenerateArrivalsExpExpRate {
  my ($end_time, $mu_arr_start, $mu_arr_rate, $mu_size_start, $mu_size_rate)=@_;
  my ($curtime, $mu_arr, $mu_size);
  my @arrivals;

  $curtime=0;
  
  while ($curtime<$end_time) { 
    $mu_arr=$mu_arr_start+$curtime*$mu_arr_rate;
    $mu_size=$mu_size_start+$curtime*$mu_size_rate;
    $curtime+=ExpRand($mu_arr);
    push @arrivals, [$curtime, ExpRand($mu_size)];
  }
  
  return @arrivals;
}


#
# Generate arrivals from a process which has:
#  - exponentially distributed interarrival
#     mean starts at mu_arr_start and changes at
#     a rate mu_arr_rate
#  - pareto-distributed service times
#     alpha starts at alpha_size_start and changes at
#     a rate alpha_size_rate
#  - uniform random distribution of priority 0-99
#
# The arrivals are returned in the form of 
# a list of references to 2-element (arrival time, size) lists
sub GenerateArrivalsExpParetoRate {
  my ($end_time, $mu_arr_start, $mu_arr_rate, $alpha_size_start, $alpha_size_rate)=@_;
  my ($curtime, $mu_arr, $alpha_size);
  my @arrivals;

  $curtime=0;
  
  while ($curtime<$end_time) { 
    $mu_arr=$mu_arr_start+$curtime*$mu_arr_rate;
    $alpha_size=$alpha_size_start+$curtime*$alpha_size_rate;
    $curtime+=ExpRand($mu_arr);
    push @arrivals, [$curtime, ParetoRand($alpha_size)];
  }
  
  return @arrivals;
}


#
#
# Integrate arrivals to generate a contention function
# using priorityless processor sharing
#
#
# the endtime, sample rate for the contention function, and the arrivals are input arguments
#
# The arrivals are provided in the form of 
# a list of references to 2-element (arrival time, size) lists
#
# The assumption is that the arrivals are in time order
#
# What is returned is a list of contention values (the function
#
#
sub IntegrateArrivals {
  my ($end_time,$sample_rate,@arrivals)=@_;
  my @contention;
  my ($curtime, $delta, $numreq, $n, $numfinished, $timestep);

  $curtime=0;
  $delta=1/$sample_rate;
  $numreq=ceil($end_time/$delta);

  $numfinished=0;
  $n=$#arrivals+1;

#  print STDERR "incoming tasks: ", (map { "($_->[0],$_->[1]), " } @arrivals), "\n";

  $timestep=0;
  while ($numfinished<$n && $timestep<$numreq) { 
    $curtime=($timestep)*$delta;
    
    # Find active tasks
    my @active=();
    my $numactive=0;
    foreach my $task (@arrivals) { 
      if ($task->[0] <= $curtime && $task->[1]>0) { 
	push @active, $task;
	++$numactive;
      }
    }
    
#    print STDERR "active tasks at $curtime: ", (map { "($_->[0],$_->[1]), " } @active), "\n";

    
    my $contention=$numactive;

    if ($numactive>0) { 
      
      # execution rate for this timestep
      # Note - processor sharing
      my $rate = 1/$numactive;
      
      # advance each active task according to the execution
      # rate and retire tasks that are done
      
      my @finished=();
      foreach my $task (@active) {
	$task->[1]-=$rate*$delta;
	if ($task->[1]<=0) { 
	  ++$numfinished;
	  push @finished, $task;
	  if ($task->[1]<0) { 
	    # this is the portion of time that was overspent on this task
	    $contention-= (-($task->[1]))  /  ($rate*$delta);
	    if ($contention<0) { 
	      $contention=0;
	      print STDERR "Under"; # this should never happen
	    }
	  }
	}
      }
      @arrivals = grep { my $include=1; my $ref=$_; map {$include=0 if $_==$ref} @finished; if (0 && !$include) { print STDERR "Ejecting ($_->[0], $_->[1])\n"}; $include  } @arrivals;
    }

    push @contention, $contention;


    ++$timestep;
  }

  push @contention, 0 until $#contention>=($numreq-1);

#  my $average=0;
#  map {$average+=$_;} @contention[0..($numreq-1)];
#  $average/=$numreq;
#  print "Average Contention: $average\n";
  
  return @contention[0..($numreq-1)];
}



sub Init {
  $pid = open2(MATIN,MATOUT,"matlab -display none -nojvm");
  MATOUT->autoflush(1);
  MATIN->autoflush(1);
}

sub DeInit {
  my ($kid, $i);

  close(MATOUT);
  close(MATIN);
  for ($i=0;$i<60;$i++) { 
    $kid=waitpid(-1,WNOHANG);
    last if $kid>0;
    sleep 1;
  }
  if ($kid<=0) { 
    `kill -9 $pid`;
  }
}


sub ExecuteMatlabRequest {
  my $req=shift;
#  my $cmd="clear all\n".$req."\n!touch .matwait\nOKGENBLAHBLAH=1";
  my $cmd="clear all\n".$req."\nOKGENBLAHBLAH=1\nquit\n";

  if (1) { 
    Init();
    print MATOUT $cmd;
    while (<MATIN>) { 
      last if /OKGENBLAHBLAH/;
    }
    DeInit();
  } else {
    if (0) { 
      
      #  print STDERR $cmd;
      `rm -f .matwait`; sleep(1);
      if ($matlabinited==0) { 
	Init();
      }
      print MATOUT $cmd;
      while (not -e ".matwait") { 
	sleep 1;
      }
      `rm -f .matwait`;
    } else {
      Init();
      print MATOUT $cmd;
      DeInit();
    }
  }
}

sub LoadOneColumnMatlabOutputFile {
  my $file=shift;
  my @lines;
  open(IN,$file);
  @lines=<IN>;
  chomp(@lines);
  close(IN);
  return @lines;
}


sub MakeMatlabContention {
  my ($end_time, $sample_rate, $generatingfunction, @args) = @_;

  my $matlabreq = 
"[t,s] = $generatingfunction($end_time,".join(",",@args).");\n".
"n = length(t);\n".
"[c,st,ft] = integrate_arrivals($end_time,$sample_rate,t,s);\n".
"dlmwrite('_temp_contention.txt',c,'\\n');\n";

  ExecuteMatlabRequest($matlabreq);

  my @c=LoadOneColumnMatlabOutputFile("_temp_contention.txt");

  `rm -f _temp_contention.txt`;

  return @c;
}


#sub MakeExpExpContention {
#  my ($end_time, $sample_rate, $mu_arr_start, $mu_arr_rate, $mu_size_start, $mu_size_rate) = @_;
#  
#  return MakeMatlabContention($end_time,$sample_rate,
#			      "generate_arrivals_exp_exp_rate",
#			      $mu_arr_start, $mu_arr_rate, $mu_size_start, $mu_size_rate);
#}
#
#sub MakeExpParetoContention {
#  my ($end_time, $sample_rate, $mu_arr_start, $mu_arr_rate, $alpha_size_start, $alpha_size_rate) = @_;
#  
#  return MakeMatlabContention($end_time,$sample_rate,
#			      "generate_arrivals_exp_pareto_rate",
#			      $mu_arr_start, $mu_arr_rate, $alpha_size_start, $alpha_size_rate);
#}



sub MakeExpExpContention {
  my ($end_time, $sample_rate,@rest)=@_;

  return IntegrateArrivals($end_time,$sample_rate,
			   GenerateArrivalsExpExpRate($end_time,@rest));
}

sub MakeExpParetoContention {
  my ($end_time, $sample_rate,@rest)=@_;

  return IntegrateArrivals($end_time,$sample_rate,
			   GenerateArrivalsExpParetoRate($end_time,@rest));
}


sub MakeRampContention { 
  my ($end_time, $sample_rate, $max,$staytime) = @_;
  my @vals;
  my $step;
  my $i;

  $step = ($max/$end_time)/$sample_rate;

  for ($i=0;$i<$end_time*$sample_rate;$i++) { 
    push @vals, $i*$step;
  }
  if (defined $staytime) { 
  for ($i=0;$i<$staytime*$sample_rate;$i++) { 
    push @vals, $max;
  }
  }

  return @vals;
}

sub MakeStepRampContention { 
  my ($end_time, $sample_rate, $timeinc, $max) = @_;
  my @vals;
  my $step;
  my $i;

  $step = ($max/$end_time)/$sample_rate;

  for ($i=0;$i<$end_time*$sample_rate;$i++) { 
    push @vals, int(($i*$sample_rate)/$timeinc)*$timeinc*$step;
  }
  return @vals;
}

sub MakeSawToothContention { 
  my ($end_time, $sample_rate, $max, $freq) = @_;
  my @vals;
  my $i;
  my $slope = $max/(1/$freq);

  for ($i=0;$i<$end_time*$sample_rate;$i++) { 
    my $t=$i/$sample_rate;
    my $offset = ($t*$freq - int($t*$freq))/$freq;
    push @vals, $offset*$slope;
  }
  return @vals;
}

sub MakeSineContention { 
  my ($end_time, $sample_rate, $max, $freq) = @_;
  my @vals;
  my $i;

  for ($i=0;$i<$end_time*$sample_rate;$i++) { 
    push @vals, $max/2 + ($max/2)*sin($freq*$i/$sample_rate);
  }
  return @vals;
}

sub MakeExpContention { 
  my ($end_time, $sample_rate, $tau) = @_;
  my @vals;
  my $i;

  for ($i=0;$i<$end_time*$sample_rate;$i++) { 
    push @vals, exp(($i/$sample_rate)/$tau);
  }
  return @vals;
}

sub MakeLogContention { 
  my ($end_time, $sample_rate, $linearrate) = @_;
  my @vals;
  my $i;

  for ($i=1;$i<$end_time*$sample_rate;$i++) { 
    push @vals, log(exp(1)+($i/$sample_rate)*$linearrate)-1;
  }
  return @vals;
}


sub MakeStepContention { 
  my ($end_time, $sample_rate, $stepstarttime, $max) = @_;
  my @vals;
  my $i;

  for ($i=0;$i<$end_time*$sample_rate;$i++) { 
    if ($i/$sample_rate >= $stepstarttime) { 
      push @vals, $max;
    } else {
      push @vals, 0;
    }
  }
  return @vals;
}

sub MakeContention { 
  my ($end_time, $sample_rate, $type, @rest) = @_;

  if ($type eq "singleexpexp") { 
    return MakeExpExpContention($end_time,$sample_rate,@rest);
  } elsif ($type eq "singleexppareto") {
    return MakeExpParetoContention($end_time,$sample_rate,@rest);
  } elsif ($type eq "ramp") {
    return MakeRampContention($end_time,$sample_rate,@rest);
  } elsif ($type eq "stepramp") {
    return MakeStepRampContention($end_time,$sample_rate,@rest);
  } elsif ($type eq "sawtooth") {
    return MakeSawToothContention($end_time,$sample_rate,@rest);
  } elsif ($type eq "sine") {
    return MakeSineContention($end_time,$sample_rate,@rest);
  } elsif ($type eq "exp") {
    return MakeExpContention($end_time,$sample_rate,@rest);
  } elsif ($type eq "log") {
    return MakeLogContention($end_time,$sample_rate,@rest);
  } elsif ($type eq "step") {
    return MakeStepContention($end_time,$sample_rate,@rest);
  } else {
    print STDERR "Unknown contention type $type\n";
    return ();
  }
}

sub MakeTestcase { 
  my ($end_time,$sample_rate,$reqhash) = @_;
  my %results;
  my $r;

  foreach $r (sort keys %{$reqhash}) { 
    $results{$r} = [ MakeContention($end_time,$sample_rate,@{$reqhash->{$r}}) ];
  }
  return %results;
}


sub LoadTestcase {
  my ($file,$num)=@_;
  my $line;
  my $found=0;
  my %hash;
  open (TEST,$file);
  # scan for testcase
  while ($line=<TEST>) { 
    next if $line=~/\s*\#.*/;
    $line=~s/\r//g;
    if ($line=~/\[$num\]/) {
      $found=1;
      last;
    }
  }
  if ($found) { 
    # now return a hash of each part of testcase to a array of the contents
    my $indata=0;
    my $name;
    my $comments="";
    while($line=<TEST>) { 
      if ($line=~/\s*\#.*/) { 
	chomp($line);
	$comments.=$line;
	next;
      }
      $line=~s/\r//g;
      chomp $line;
      if ($line=~/end/) { 
	$hash{comments}=$comments;
	last;
      }
      if ($indata==0) { 
	$name = $line;
	$indata=1;
      } else {
	my @vals = split(/\,/,$line);
	$hash{$name} = \@vals;
	$indata=0;
      }
    }
  }
  close(TEST);
  return %hash;
}

sub LoadTestcaseNumbers {
  my ($file)=@_;
  my $line;
  my @nums;
  open (TEST,$file);
  # scan for testcase
  while ($line=<TEST>) { 
    next if $line=~/\s*\#.*/;
    $line=~s/\r//g;
    if ($line=~/\[(\d+)\]/) {
      push @nums, $1;
    }
  }
  return @nums;
}


sub AppendTestcase {
  my ($file,$num,$hashref)=@_;
  my $r;
  open(TEST,">>$file");
  print TEST "[$num]\n";
  print TEST "# ".$hashref->{comments}."\n";
  foreach $r (sort keys %{$hashref}) {
        next if $r eq "comments";
        $outstring=$r."\n";
        foreach $val (@{$hashref->{$r}})
        {
        $outstring=$outstring.(sprintf "%0.2f",$val).",";   #format numeric values using sprintf
        }
        $outstring=substr $outstring,0,-1; # delete the last comma
        $outstring=$outstring."\n";
        print TEST $outstring;

  }
  print TEST "end\n";
}

