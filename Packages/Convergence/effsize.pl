#!/usr/bin/perl -w
#
# PERL front-end to the effective-sample size tools
#
# NOTE: Requires all of the LOOS tools to be in your shell's $PATH
# variable


#  This file is part of LOOS.
#
#  LOOS (Lightweight Object-Oriented Structure library)
#  Copyright (c) 2010, Tod D. Romo
#  Department of Biochemistry and Biophysics
#  School of Medicine & Dentistry, University of Rochester
#
#  This package (LOOS) is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation under version 3 of the License.
#
#  This package is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.



use FileHandle;
use Getopt::Long;
use Env qw/@PATH/;

&loosCheck(qw/trajinfo ufidpick assign_frames hierarchy neff/);   # Check loos installation


my $prefix;          # Prefix for intermediate files
my $nbins = 20;      # Number of bins to use for the structural histogram
my $nreps = 10;      # Number of times to repeat the analysis
my $model_name;      # Model to use
my $traj_name;       # Trajectory
my $selection;       # Which atoms to consider
my $verbosity = 1;   # Higher values means more output (primarily for logging)
my $seed = 0;        # Generate a new random seed (!= 0 means use explicit seed)


# Suffixes generated by the various tools we'll call...
my @suffixes = qw/.log _assignments.asc .states .dcd .pdb .samples/;
my $backup = '.bak';

# Store the header for logging
my $hdr = &header($0, \@ARGV);

# Process command-line options
my $ok = GetOptions(
		    "prefix=s" => \$prefix,
		    "nbins=i" => \$nbins,
		    "nreps=i" => \$nreps,
		    "verbosity=i" => \$verbosity,
		    "seed=i" => \$seed,
		    "help" => sub { &showHelp; }
		   );

&showHelp if (!$ok || $#ARGV != 2);

$model_name = shift;
$traj_name = shift;
$selection = shift;

my $seedval = ($seed == 0) ? '' : $seed;

# If no prefix is specified, generate one from the model-name
# (stripping off file-type extender (i.e. .pdb of .psf) and adding
# ".neff"
if (!defined($prefix)) {
  if ($model_name =~ /\./) {
    $model_name =~ /^(.+)\..+?$/;
    $prefix = $1;
  } else {
    $prefix = $model_name;
  }

  $prefix .= '.neff';
}

# Determine bin-size (since ufidpick expects a fraction of the
# population to be in each bin) ***IMPORTANT NOTE*** To keep the last
# bin from having fewer structures in it, the trajectory length is
# clipped so that it is a multiple of the bin-size (by using the range
# option to the tools)
my $nframes = &getNumberOfFrames($model_name, $traj_name);
my $frac = 1.0 / $nbins;
my $binsize = int($nframes / $nbins);
$binsize = 1 if ($binsize < 1);
my $range = $binsize * $nbins - 1;

# Log command-line options
print "# $hdr\n# binsize=$binsize, frac=$frac, range=$range\n";
print "# iter\tneff\tneff Total\n";


# Stores the Neff for each iteration of the analysis...
my @neffs;

for (my $i=0; $i<$nreps; ++$i) {
  
  # First, partition the trajectory's configuration space using a
  # uniform structural histogram structural.  Creates a trajectory
  # (DCD) containing the reference structures (aka fiducials)
  &runCommand("ufidpick $model_name $traj_name 0:$range '$selection' $prefix $frac $seedval >$prefix.log 2>&1");

  # Next, go back through the trajectory and assign each structure to
  # the nearest reference structure.
  &runCommand("assign_frames $model_name $traj_name 0:$range '$selection' $prefix.dcd >${prefix}_assignments.asc 2>>$prefix.log");

  # Hierchy takes the assignments and constructs a hierarchical
  # clustering that is written into the states file
  &runCommand("hierarchy ${prefix}_assignments.asc >$prefix.states");

  # This takes the assignemnts and the clustered states and determines
  # the effective sample size
  &runCommand("neff ${prefix}_assignments.asc $prefix.states $binsize >$prefix.samples");

  # Extract the output and store
  my $neff = &extractNeff("$prefix.samples");
  defined($neff) || die "Error- cannot find effective sample size in neff output ($prefix.samples)";
  if ($verbosity) {
    printf "%-3d\t%f\t%f\n", $i, $neff, $neff * $nbins;
  }
  push(@neffs, $neff);
}

# Compute basic statistics for the Neff's
# These are written as "metadata" in the output (i.e. comments)
my $mean = &average(\@neffs);
my $std = &stddev(\@neffs, $mean);

print "# Average effective sample size = $mean += $std\n";

my $total_samples = $mean * $nbins;
my $total_std = $std * $nbins;

print "# Total samples = $total_samples += $total_std\n";

my $low_td = $range/($total_samples + $total_std);
my $high_td = $range/($total_samples - $total_std);
print "# Effective Td = $low_td - $high_td\n";



#############################################################


sub average {
  my $ra = shift;

  my $avg = 0.0;
  foreach (@$ra) {
    $avg += $_;
  }

  return($avg / ($#$ra + 1));
}


sub stddev {
  my $ra = shift;
  my $m = shift;

  my $s = 0.0;
  foreach (@$ra) {
    my $d = $_ - $m;
    $s += $d*$d;
  }

  return(sqrt($s / $#$ra));
}



# Run a shell command while checking for error and logging the command
# (if requested)
sub runCommand {
  my $cmd = shift;

  print STDERR "# $cmd\n" if ($verbosity > 1);
  my $failed = system($cmd);
  die "Error- '$cmd' failed with code ", $failed >> 8 if ($failed);
}



# Parse neff's output
sub extractNeff {
  my $fn = shift;
  my $fh = new FileHandle $fn;
  defined($fh) || die "Error- cannot open neff output file $fn";

  while (<$fh>) {
    chomp;
    if (/^Segment effective sample size = (.+)$/) {
      return($1);
    }
  }

  return(undef);
}


# Use trajinfo to get the number of frames in a trajectory
sub getNumberOfFrames {
  my $model = shift;
  my $traj = shift;

  my $fh = new FileHandle "trajinfo -B1 $model $traj 2>&1|";
  defined($fh) || die "Error- cannot open pipe from trajinfo command";

  my $dummy = <$fh>;
  my @ary = split(/\s+/, $dummy);
  $#ary == 3 || die "Error- trajinfo command gave bad results";
  return($ary[1]);
}


# Generates a log header
sub header {
  my $prog = shift;
  my $ra = shift;

  my $hdr = $prog . ' ';
  foreach (@$ra) {
    $hdr .= '\'' . $_ . '\' ';
  }
  chop($hdr);
  return($hdr);
}


sub showHelp {
  print <<EOF;
Usage- effsize.pl [options] model traj selection

Options:
        --help        This message
    --prefix=s        Prefix for intermmediate files
     --nbins=i (20)   Number of bins to use to partition space
     --nreps=i (10)   Number of times to repeat analysis
 --verbosity=i (1)    How verbose output is during operation
      --seed=i (0)    Explicitly set the random number seed (0 = auto)
EOF
  exit(0);
}



sub loosCheck {
  my @exes = @_;

  foreach my $name (@exes) {
    my $flag = 0;
    foreach my $dir (@PATH) {
      if (-e "$dir/$name") {
	$flag = 1;
	last;
      }
    }
    if (!$flag) {
      print STDERR "Error- $name not found.  Are you sure LOOS is correctly installed and in your shell's path?\n";
      die;
    }
  }
}