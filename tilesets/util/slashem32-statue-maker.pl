#!/usr/bin/perl
# This file is in the public domain.

my %cmdarg = @ARGV;

my $num = $cmdarg{start_numbers_at} || 5000;

my @monsterfile = map {
  "/home/jonadab/git/fourk/tilesets/dat/tiles/$_.txt"
} ("s-mon32mi",
   "s-mon32se",
   "s-mon32alg",
   "s-mon32aw",
  );

my $statuefile = "/home/jonadab/git/fourk/tilesets/dat/tiles/s-statue32.txt";
open OUT, ">", $statuefile or die "Cannot write statue file ($statuefile): $!";

print OUT qq[!Generated automatically by tileset/utils/slashem32-statue-maker.pl
! NetHack may be freely redistributed.  See license for details.
__ = (71, 108, 108)
XX = (255, 0, 0)\n];

my @pchar = ("a" .. "z");
my %gray = map {
  my $value = $_;
  my $pindex = "A" . shift @pchar;
  print OUT qq[$pindex = ($value, $value, $value)\n];
  $value => $pindex,
} 0, 16, 32, 64, 96, 128, 160, 192, 224, 239, 247, 255;

for my $mfile (@monsterfile) {
  open IN, "<", $mfile or die "Cannot read monster file ($mfile): $!";
  my %convert = ( "__" => "__" );
  my $hasalpha = 0;
  while (<IN>) {
    my $line = $_;
        my $line = $_;
    if ($line =~ /^\s*[#]\s*tile\s+\d+\s*[(](.*?)[)]/) {
      my $mname = $1;
      $num++;
      $line = qq[# tile $num (sub statue $mname)\n];
    } elsif ($line =~ /^\s*([A-Z_][A-Za-z0-9_\$]){32}\s*$/) {
      $line =~ s/([A-Z_][A-Za-z0-9_\$])/$convert{$1} || "XX"/eg;
    } elsif ($line =~ /^\s*[{}]\s*$/) {
      # No changes
    } elsif ($line =~ /^\s*[!]/) {
      # Comment, skip:
      $line = "";
    } elsif ($line =~ /^\s*$/) {
      # Blank line, skip:
      $line = "";
    } elsif ($line =~ /^\s*([A-Z_][A-Za-z0-9_\$])\s*[=]\s*[(]\s*(\d+),\s*(\d+),\s*(\d+)(?:,\s*(\d+))?[)]\s*$/) {
      # Palette definition:
      my ($idx, $r, $g, $b, $a) = ($1, $2, $3, $4, $5);
      if ($a) { $hasalpha++; } # For now just count these, so I can decide if it's worth implementing support for them.
      if ($idx ne "__") {
        my $v = int(((4 * $r) + (5 * $g) + (4 * $b) + (4 * 100)) / 17);
        while (not exists $gray{$v}) { $v--; }
        $convert{$idx} = $gray{$v};
      }
      $line = "";
    } else {
      warn "Unable to parse line: $line";
    }
    print OUT $line;
  }
  print "$hasalpha colors with alpha defined in $mfile\n" if $hasalpha > 0;
}

close OUT;
print "Wrote $statuefile\n";
