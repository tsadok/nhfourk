#!/usr/bin/perl
# -*- cperl -*-
# This Perl script, tiletool.pl, is in the public domain.

# At your option, you may choose instead to treat it as being under
# the NetHack license (see libnethack/dat/license for terms).

use File::Spec::Functions;
my $reset = qq{[0m};

die "This script is designed to be run from the mail tilesets directory in the source repository for NetHack4 or a variant thereof."
  if not -e catfile("dat", "catalogues");
my $tileset = 'slashem-16'; # by default
my @arg = @ARGV;
my @show;
while (@arg) {
  my $x = shift @arg;
  if ($x eq 'tileset') {
    $tileset = shift @arg;
  } elsif ($x eq 'show') {
    push @show, shift @arg;
  }
}

my $catalog = catfile("dat", "catalogues", "$tileset.txt");
die "Cannot find catalogue file: $catalog" if not -e $catalog;
open CAT, "<", $catalog or die "Cannot read catalogue file ($catalog): $!";
my $dimensions = <CAT>; # Might as well read this info, since it's here.
my ($dimy, $dimx) = $dimensions =~ /(\d+)\s+(\d+)/;
my $setname = <CAT>; # In-game display name for this tileset.
chomp $setname;
my @tilefile;
while (<CAT>) {
  chomp;
  push @tilefile, $_;
}
my (%palette, @tile);
# A note about the palette: we currently assume that the palette is
# the same for all the tiles in any given tileset.  This is currently
# the case for both slashem-16 and dawnlike, which are the tilesets we
# are currently interested in working with using this utility.  (The
# RL-tiles and Geoduck tilesets use 32-bit color and use PNG images,
# and the ASCII and Unicode tilesets are not pixel-based.)  Because
# the tiles files actually define the palette at the top of each file,
# it would be possible to define a tileset wherein different tiles use
# different sets of colors; but this tiletool script currently does
# not support that (and I'm not entirely sure the tileset compiler
# supports it either).
for my $tf (@tilefile) {
  my $filespec = catfile("dat", "tiles", $tf);
  warn "Cannot find tiles file: $filespec" if not -e $filespec;
  open TF, "<", $filespec or die "Cannot read tiles file ($tf): $!";
  my ($tilenum, $tilename, @tileline);
  while (<TF>) {
    my $line = $_;
    if ($line =~ /^[!]/) {
      # comment, ignore
    } elsif ($line =~ /^([A-Z\$])\s+[=]\s+[(](\d+),\s*(\d+),\s*(\d+)\s*[)]$/) {
      my ($char, $r, $g, $b) = ($1, $2, $3, $4);
      $palette{$char} = [$r, $g, $b];
    } elsif ($line =~ /^\s*#\s*tile\s+(\d+)\s+[(]([^)]+)[)]/) {
      ($tilenum, $tilename) = ($1, $2);
    } elsif ($line =~ /^[{]/) {
      # This just indicates we're about to start getting tile lines.
    } elsif ($line =~ /^\s+([A-Z\$]{$dimx})$/) {
      push @tileline, $1;
    } elsif ($line =~ /^[}]/) {
      warn "Incorrect number of lines in tile $tilenum ($tilename): expected $dimy found " . scalar @tileline
        if $dimy ne scalar @tileline;
      push @tile, +{ num  => $tilenum,
                     name => $tilename,
                     file => $tf,
                     tile => [@tileline],
                   };
      #print "tile $tilenum, $tilename\n";
      ($tilenum, $tilename, @tileline) = (undef, undef);
    } elsif ($line =~ /^\s*$/) {
      # Don't warn about blank lines.
    } else {
      warn "$tf: failed to parse line $lnum: $line";
    }
  }
}

my @pkey = sort { $a cmp $b } keys %palette;
print "$setname palette: " . (join "", map {
  my $c = $_;
  my ($r, $g, $b) = @{$palette{$c}};
  rgb($r, $g, $b, "bg") . "   ";
} @pkey)
  . $reset . "\n" . (" " x length $setname) . "          "
  . join ("", map {
    " $_ "
  } @pkey) . "\n";
print $reset . "$setname defines " . @tile . " tiles.\n";

for my $s (@show) {
  my @t = grep {
    ($$_{num} eq $s) or ($$_{name} =~ $s)
  } @tile;
  if (@t) {
    showtile($_) for @t;
  } else {
    warn "Did not find tile: $s\n";
  }
}

exit 0; # Subroutines follow

sub showtile {
  my ($t) = @_;
  print rgb(0, 0, 0, "bg") . rgb(255,255,255) . $setname . $reset . " tile "
    . rgb(0,0,0,"bg") . rgb(255,255,127) . $$t{num} . " (" . $$t{name} . ")"
    . $reset . "\n (defined in $$t{file})\n\n";
  for my $line (@{$$t{tile}}) {
    #print rgb(0,0,0,"bg") . rgb(193,255,255);
    for my $char (split //, $line) {
      print $char;
    }
    print $reset . "  ";
    for my $char (split //, $line) {
      print $reset;
      my $p = $palette{$char};
      if (defined $p) {
        print rgb($$p[0], $$p[1], $$p[2]);
      }
      print $char;
    }
    print $reset . "  ";
    for my $char (split //, $line) {
      my $p = $palette{$char};
      if (defined $p) {
        print $reset . rgb($$p[0], $$p[1], $$p[2], "bg") . "  ";
      } else {
        print rgb(63,0,0,"bg") . rgb(255,255,128) . $char . $char;
      }
    }
    print $reset . "\n";
  }
  print $reset . "\n\n";
}

sub rgb { # Return terminal code for a 24-bit color.
  my ($red, $green, $blue, $isbg) = @_;
  my $fgbg = ($isbg) ? 48 : 38;
  my $delimiter = ";";
  return $fallback . "\x1b[$fgbg$ {delimiter}2$ {delimiter}$ {red}$ {delimiter}$ {green}$ {delimiter}$ {blue}m";
}


