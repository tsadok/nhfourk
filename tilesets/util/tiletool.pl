#!/usr/bin/perl
# -*- cperl -*-
# This Perl script, tiletool.pl, is in the public domain.

# At your option, you may choose instead to treat it as being under
# the NetHack license (see libnethack/dat/license for terms).

use File::Spec::Functions;

my $reset = qq{[0m};

die "This script is designed to be run from the main tilesets directory in the source repository for NetHack4 or a variant thereof."
  if not -e catfile("dat", "catalogues");
my $tileset = 'slashem-16'; # by default
my @arg = @ARGV;
my (@show, $edit, $debug);
my ($autoadvance, $wrap) = (0, 0);
while (@arg) {
  my $x = shift @arg;
  if ($x eq 'tileset') {
    $tileset = shift @arg;
  } elsif ($x eq 'show') {
    push @show, shift @arg;
  } elsif ($x eq 'edit') {
    $edit = shift @arg;
  } elsif ($x eq 'debug') {
    $debug++;
  } elsif ($x eq 'wrap') {
    $wrap = 1;
  } elsif ($x eq 'autoadvance') {
    $autoadvance = 1;
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
my (%palette, @tile, $currenttile, $maxtilenum, %numidx);
my ($cursorx, $cursory) = (-1, -1);
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
      $maxtilenum ||= $tilenum; $maxtilenum = $tilenum if $tilenum > $maxtilenum;
      if ($numidx{$tilenum}) {
        warn "Duplicate tile number: $tilenum ($numidx{$tilenum}) ($tilename)\n"
          if $debug;
      } else {
        $numidx{$tilenum} = $tilename;
      }
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

showpalette();
print $reset . "$setname defines " . @tile . " tiles.\n";

for my $s (@show) {
  my @t = grep {
    ($$_{num} eq $s) or ($$_{name} =~ $s)
  } @tile;
  if (@t) {
    for my $tile (@t) {
      $currenttile = $tile;
      showtile($tile);
    }
  } else {
    warn "Did not find tile: $s\n";
  }
}

edittile($edit) if $edit;

exit 0; # Subroutines follow

sub edittile {
  my %dirkey = ( h => [-1,  0], j => [ 0,  1], k => [0, -1], l => [1, 0],
                 y => [-1, -1], u => [ 1, -1], b => [-1, 1], n => [1, 1],);
  my $defaultcolor = (sort { $a cmp $b } keys %palette)[0];
  my $tile = $currenttile || +{ file => "[no file]",
                                tile => [ map {
                                  join "", map { $defaultcolor } 1 .. $dimx
                                } 1 .. $dimy],
                              };
  $$tile{num}  = ($maxtilenum += 1);
  $$tile{name} = $edit || $$tile{name} || "newtile";
  use Term::ReadKey;
  END { ReadMode 'restore'; } ReadMode 'cbreak'; # Don't auto-echo typed characters.
  $cursorx = $cursory = 0;
  print rgb(63,191,191) . "hjklyubn to move cursor, capital letters to change color (see palette)" . $reset . "\n";
  print rgb(63,191,191) . "a - turn autoadvance " . ($autoadvance ? "off" : "on")
    . "; w - turn wraparound " . ($wrap ? "off" : "on") . "; x - exit" . $reset . "\n";
  while (1) {
    showpalette();
    showtile($tile);
    my $k = ReadKey 0;
    if ($palette{$k}) {
      my $line        = $$tile{tile}[$cursory];
      my @char        = split //, $line;
      $char[$cursorx] = $k;
      $$tile{tile}[$cursory] = join '', @char;
      if ($autoadvance) {
        $cursorx++;
        if ($cursorx >= $dimx) {
          $cursorx = 0;
          $cursory++;
          if ($cursory >= $dimy) {
            $cursory = $wrap ? 0 : ($cursory - 1);
          }}}
    } elsif ($k eq "a") {
      $autoadvance = ($autoadvance) ? 0 : 1;
    } elsif ($k eq "w") {
      $wrap = $wrap ? 0 : 1;
    } elsif ($k eq "x") {
      print $reset . qq[\n\n# tile $$tile{num} ($$tile{name})\n{\n] .
        (join "\n", map { "  " . $_ } @{$$tile{tile}}). qq[\n}\n\n];
      exit 0;
    } elsif ($dirkey{$k}) {
      $cursorx += $dirkey{$k}[0];
      $cursory += $dirkey{$k}[1];
      if ($cursorx >= $dimx) {
        $cursorx = $wrap ? 0 : ($cursorx - 1);
      }
      if ($cursorx < 0) {
        $cursorx = $wrap ? ($dimx - 1) : $cursorx + 1;
      }
      if ($cursory >= $dimy) {
        $cursory = $wrap ? 0 : ($cursory - 1);
      }
      if ($cursory < 0) {
        $cursory = $wrap ? ($dimy - 1) : $cursory + 1;
      }
    }
    else {
      warn rgb(63,0,0,"bg") . rgb(255,255,127) . "Unrecognized key: '$k'" . $reset . "\n";
    }
  }
}

sub showtile {
  my ($t) = @_;
  print rgb(0, 0, 0, "bg") . rgb(255,255,255) . $setname . $reset . " tile "
    . rgb(0,0,0,"bg") . rgb(255,255,127) . $$t{num} . " (" . $$t{name} . ")"
    . $reset . "\n (defined in $$t{file})\n\n";
  my $y = 0;
  for my $line (@{$$t{tile}}) {
    #print rgb(0,0,0,"bg") . rgb(193,255,255);
    for my $char (split //, $line) {
      print $char;
    }
    print $reset . "  ";
    my $x = 0;
    for my $char (split //, $line) {
      print $reset;
      if (($y == $cursory) and ($x == $cursorx)) {
        print rgb(16,16,16,"bg");
      }
      my $p = $palette{$char};
      if (defined $p) {
        print rgb($$p[0], $$p[1], $$p[2]);
      }
      $x++;
      print $char;
    }
    print $reset . "  ";
    $x = 0;
    for my $char (split //, $line) {
      my $p = $palette{$char};
      if (defined $p) {
        print $reset . rgb($$p[0], $$p[1], $$p[2], "bg")
          . ((($x == $cursorx) and ($y == $cursory))
             ? (rgb(0,0,0) . "?" . rgb(255,255,255) . "?") : "  ");
      } else {
        print rgb(63,0,0,"bg") . rgb(255,255,127) . $char . $char;
      }
      $x++;
    }
    print $reset . "\n";
    $y++;
  }
  print $reset . "\n\n";
}

sub showpalette {
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
}

sub rgb { # Return terminal code for a 24-bit color.
  my ($red, $green, $blue, $isbg) = @_;
  my $fgbg = ($isbg) ? 48 : 38;
  my $delimiter = ";";
  return $fallback . "\x1b[$fgbg$ {delimiter}2$ {delimiter}$ {red}$ {delimiter}$ {green}$ {delimiter}$ {blue}m";
}


