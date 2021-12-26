#!/usr/bin/perl
# This file is in the public domain.

my %cmdarg = @ARGV;

my $num = $cmdarg{start_numbers_at} || 2020; # Leave room for hand-edited ones at 2000-2019.

my @monsterfile = map {
  "/home/jonadab/git/fourk/tilesets/dat/tiles/$_.txt"
} ("fmob-animals",
   "fmob-dragons",
   "fmob-monsters-misc",
   "fmob-humanoids",
   "fmob-imps",
   "fmob-undead",
   "fmob-demons",
   "fmob-roles",
  );
my $statuefile = "/home/jonadab/fmob-statues-generated.txt";

open OUT, ">", $statuefile or die "Cannot write statue file ($statuefile): $!";

for my $mfile (@monsterfile) {
  open IN, "<", $mfile  or die "Cannot read monster file ($mfile): $!";
  my $skip = 0;
  while (<IN>) {
    my $line = $_;
    if ($line =~ /^\s*[#]\s*tile\s+\d+\s*[(](.*?)[)]/) {
      my $mname = $1;
      if ($mname =~ /were/) { # werefoo statues are hand-edited
        $skip = 1;
      } else {
        $num++;
        $skip = 0;
        $line = qq[# tile $num (sub statue $mname)\n];
      }
    } elsif ($line =~ /^\s*([B-O][Bb-z]){24}\s*$/) {
      $line =~ s/[C-O][b-z]/Oe/g;
      $line =~ s/B[c-z]/Oe/g;
      $line =~ s/Bb/Be/g;
    } elsif ($line =~ /^\s*[{}]\s*$/) {
      # No changes
    } elsif ($line =~ /^\s*[!]/) {
      # Comment, skip:
      $line = "";
    } elsif ($line =~ /^\s*$/) {
      # Blank line, skip:
      $line = "";
    } elsif ($line =~ /^\s*[B-O][Bb-z]\s*[=]\s*[(]\s*\d+,\s*\d+,\s*\d+(?:,\s*\d+)?[)]\s*$/) {
      # Palette definition, skip.
      $line = "";
    } else {
      warn "Unable to parse line: $line";
    }
    print OUT $line if ($line and not $skip);
  }
}
close OUT;
print "Wrote $statuefile\n";
