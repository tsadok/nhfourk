#!/usr/bin/perl

my %cmdarg = @ARGV;

my $num = $cmdarg{start_numbers_at} || 1000;

my @monsterfile = map {
  "/home/jonadab/git/fourk/tilesets/dat/tiles/$_.txt"
} ("fmob-animals",
   "fmob-monsters-misc",
   "fmob-humanoids",
   "fmob-imps",
   "fmob-demons",
  );
my $statuefile = "/home/jonadab/fmob-statues-generated.txt";

open OUT, ">", $statuefile or die "Cannot write statue file ($statuefile): $!";

for my $mfile (@monsterfile) {
  open IN, "<", $mfile  or die "Cannot read monster file ($mfile): $!";
  while (<IN>) {
    my $line = $_;
    if ($line =~ /^\s*[#]\s*tile\s+\d+\s*[(](.*?)[)]/) {
      my $mname = $1;
      $num++;
      $line = qq[# tile $num (sub statue $mname)\n];
    } elsif ($line =~ /^\s*([B-O][Bb-z]){24}\s*$/) {
      $line =~ s/[C-O][b-z]/Oe/g;
      $line =~ s/B[c-z]/Oe/g;
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
    print OUT $line;
  }
}
