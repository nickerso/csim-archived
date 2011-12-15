#!/usr/bin/perl -w
#eval 'exec perl -w -S $0 ${1+"$@"}'
#    if 0;

use strict;

if (scalar @ARGV < 4) { exit(1); }
my $input = shift;
my $output = shift;
my $src = shift;
my $cvodes_inc_dir = shift;
my $incSvn = 1;
if (scalar @ARGV > 0) { $incSvn = shift; }
my $incDate = 1;
if (scalar @ARGV > 0) { $incDate = shift; }

my $svnversion = `cd $src && svnversion`;
chomp $svnversion;
my $date = `date`;
chomp $date;

# find the SUNDIALS version
my $sundials_version_string = "UNKNOWN";
open (SC,"<$cvodes_inc_dir/sundials/sundials_config.h") or die "Unable to open sundials_config.h: $!\n";
while (<SC>) {
  if (/^\#define SUNDIALS_PACKAGE_VERSION \"(\S+)\"/) {
    $sundials_version_string = $1;
    last;
  }
}
close SC;

open(INPUT,"<$input") or die "Unable to open input file ($input): $!\n";
open(OUTPUT,">$output") or die "Unable to open output file ($output): $!\n";
while (<INPUT>) {
  s%\/\*SUNDIALS_VERSION\*\/%$sundials_version_string%;
  if ($incSvn) {
    s%\/\*SVN_VERSION_STRING\*\/%\#define SVN_VERSION_STRING \"$svnversion\"%;
  }
  if ($incDate) {
    s%\/\*DATE_STRING\*\/%\#define DATE_STRING \"$date\"%;
  }
  print OUTPUT $_;
}
close INPUT;
close OUTPUT;

