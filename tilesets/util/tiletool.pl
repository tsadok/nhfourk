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
my ($autoadvance, $wrap, $check, $palettemode) = (0, 0, 0);
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
  } elsif ($x eq 'checknums') {
    $check = 1;
  }
}

my $catalog = catfile("dat", "catalogues", "$tileset.txt");
die "Cannot find catalogue file: $catalog" if not -e $catalog;
open CAT, "<", $catalog or die "Cannot read catalogue file ($catalog): $!";
my $dimensions = <CAT>; # Might as well read this info, since it's here.
my ($dimy, $dimx) = $dimensions =~ /(\d+)\s+(\d+)/;
my $dimxtwo = $dimx * 2;
my $setname = <CAT>; # In-game display name for this tileset.
chomp $setname;
my @tilefile;
while (<CAT>) {
  chomp;
  push @tilefile, $_;
}
my (%palette, @tile, $currenttile, $maxtilenum, %numidx, @badnum);
my ($cursorx, $cursory) = (-1, -1);
# A note about the palette: we originally assumed that the palette was
# the same for all the tiles in any given tileset, a conclusion we
# drew from looking mostly at slashem-16.  That assumption was invalid,
# a fact that became obvious when we started looking at slashem-32 and
# at the dragons in dawnlike-16.  We now treat palette as file-specific.
for my $tf (@tilefile) {
  my $filespec = catfile("dat", "tiles", $tf);
  warn "Cannot find tiles file: $filespec" if not -e $filespec;
  open TF, "<", $filespec or die "Cannot read tiles file ($tf): $!";
  my ($lastnum, $tilenum, $tilename, @tileline) = (0);
  while (<TF>) {
    my $line = $_;
    if ($line =~ /^[!]/) {
      # comment, ignore
    } elsif ($line =~ /^([A-Za-z0-9_\$][A-Za-z0-9_\$])\s+[=]\s+[(](\d+),\s*(\d+),\s*(\d+)\s*(?:,\s*(\d+)\s*)?[)]$/) {
      my ($chars, $r, $g, $b, $alpha) = ($1, $2, $3, $4, $5);
      if ((defined $palettemode) and ($palettemode ne 2)) {
        die "Cannot mix single-width and double-width palette modes.\n$line\n";
      }
      $palettemode = 2;
      $palette{$tf}{$chars} = [$r, $g, $b, $alpha];
    } elsif ($line =~ /^([A-Z0-9\$])\s+[=]\s+[(](\d+),\s*(\d+),\s*(\d+)(?:,\s*\d+)?\s*[)]$/) {
      my ($char, $r, $g, $b) = ($1, $2, $3, $4);
      if ((defined $palettemode) and ($palettemode ne 1)) {
        die "Cannot mix double-width and single-width palette modes.\n$line\n";
      }
      $palettemode = 1;
      $palette{$tf}{$char} = [$r, $g, $b];
    } elsif ($line =~ /^\s*#\s*tile\s+(\d+)\s+[(]([^)]+)[)]/) {
      ($tilenum, $tilename) = ($1, $2);
      $maxtilenum ||= $tilenum; $maxtilenum = $tilenum if $tilenum > $maxtilenum;
      if ($numidx{$tilenum}) {
        push @badnum, $tilenum;
        warn "Duplicate tile number: $tilenum ($numidx{$tilenum}) ($tilename)\n"
          if $debug or $check;
      } elsif ($tilenum <= $lastnum) {
        push @badnum, $tilenum;
        warn "Decreasing tile number: $tilenum ($numidx{$tilenum}) ($tilename)\n"
          if $debug or $check;
      } else {
        $numidx{$tilenum} = $tilename;
        $lastnum = $tilenum;
      }
    } elsif ($line =~ /^[{]/) {
      # This just indicates we're about to start getting tile lines.
    } elsif (($palettemode eq 1) and ($line =~ /^\s+([A-Z0-9_\$]{$dimx})$/)) {
      # TODO: report incorrect line widths
      push @tileline, $1;
    } elsif (($palettemode eq 2) and ($line =~ /^\s+([A-Za-z0-9_\$]{$dimxtwo})$/)) {
      # TODO: report incorrect line widths
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

showpalettes();
print $reset . "$setname defines " . @tile . " tiles.\n";
if ($check) { # report problematic and skipped tile numbers
  @tile = sort { $$a{num} <=> $$b{num} } @tile;
  my @skipped = ();
  my $expected = 0;
  for my $t (@tile) {
    while ($$t{num} > ++$expected) {
      push @skipped, $expected;
    }
  }
  if (@badnum) {
    print "Problematic tile numbers: " . (join ", ", @badnum) . "\n";
  }
  if (@skipped) {
    print "Skipped tile numbers: " . (join ", ", @skipped) . "\n";
  }
}

for my $s (@show) {
  my @t = grep {
    ($$_{num} eq $s) or ($$_{name} =~ $s)
  } @tile;
  if (@t) {
    for my $tile (@t) {
      $currenttile = $tile;
      showpalette($$tile{file});
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
  my $file = $currenttile ? $$currenttile{file} : $tilefile[0];
  my $defaultcolor = (sort { $a cmp $b } keys %{$palette{$file}})[0];
  my $tile = $currenttile || +{ file => $file,
                                tile => [ map {
                                  join "", map { $defaultcolor } 1 .. $dimx
                                } 1 .. $dimy],
                              };
  $$tile{num}  = ($maxtilenum += 1);
  $$tile{name} = $edit || $$tile{name} || "newtile";
  my $lastcolor = undef;
  use Term::ReadKey;
  END { ReadMode 'restore'; } ReadMode 'cbreak'; # Don't auto-echo typed characters.
  $cursorx = $cursory = 0;
  print rgb(63,191,191) . "hjklyubn to move cursor, * see palette for colors" . $reset . "\n";
  print rgb(63,191,191) . "* - enter a palette color that starts with a conflicted key" . $reset . "\n";
  print rgb(63,191,191) . ". - use the same color you just used." . $reset . "\n";
  print rgb(63,191,191) . "a - turn autoadvance " . ($autoadvance ? "off" : "on")
    . "; w - turn wraparound " . ($wrap ? "off" : "on") . "; x - exit" . $reset . "\n";
  while (1) {
    showpalette($$tile{file});
    showtile($tile);
    my $k = ReadKey 0;
    if ($k eq "a") {
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
    } elsif (($k eq ".") and defined $lastcolor) {
      my $line        = $$tile{tile}[$cursory];
      my @char;
      if ($palettemode eq 2) {
        my @c = split //, $line;
        while (1 < scalar @c) {
          my $one = shift @c;
          my $two = shift @c;
          push @char, "$one$two";
        }
      } else {
        @char        = split //, $line;
      }
      $char[$cursorx] = $lastcolor;
      $$tile{tile}[$cursory] = join '', @char;
      if ($autoadvance) {
        $cursorx++;
        if ($cursorx >= $dimx) {
          $cursorx = 0;
          $cursory++;
          if ($cursory >= $dimy) {
            $cursory = $wrap ? 0 : ($cursory - 1);
          }}}
    } elsif ($palette{$$tile{file}}{$k}) {
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
      $lastcolor = $k;
    } else {
      $k = ReadKey 0 if $k eq "*";
      $k .= ReadKey 0 if $palettemode eq 2;
      if ($palette{$$tile{file}}{$k}) {
        my $line        = $$tile{tile}[$cursory];
        my @char;
        if ($palettemode eq 2) {
          my @c = split //, $line;
          while (1 < scalar @c) {
            my $one = shift @c;
            my $two = shift @c;
            push @char, "$one$two";
          }
        } else {
          @char        = split //, $line;
        }
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
        $lastcolor = $k;
      } else {
        warn rgb(63,0,0,"bg") . rgb(255,255,127) . "Unrecognized color: '$k'" . $reset . "\n";
      }}
  }
}

sub showtile {
  my ($t) = @_;
  print rgb(0, 0, 0, "bg") . rgb(255,255,255) . $setname . $reset . " tile "
    . rgb(0,0,0,"bg") . rgb(255,255,127) . $$t{num} . " (" . $$t{name} . ")"
    . $reset . "\n (defined in $$t{file})\n\n";
  my $y = 0;
  for my $line (@{$$t{tile}}) {
    if ($palettemode eq 2) {
      my $chars = "$line"; my $x = 0;
      while (1 < length $chars) {
        ($key, $chars) = $chars =~ /(..)(.*)/;
        my ($r, $g, $b) = (255, 128, 212); # Make palette misses very noticeable.
        my $p = $palette{$$t{file}}{$key};
        ($r, $g, $b) = @$p if ref $p;
        #my $labelshade = ($r + $b + $g < (128 * 3)) ? 160 : 96;
        my $contrast = 96;
        if ((($x == $cursorx) and ($y == $cursory))) {
          print $reset . rgb($r, $g, $b, "bg") . rgb(0,0,0) . "?" . rgb(255,255,255) . "?";
        } else {
          print $reset . rgb($r, $g, $b, "bg") . rgb((($r > 128) ? $r - $contrast : $r + $contrast),
                                                     (($g > 128) ? $g - $contrast : $g + $contrast),
                                                     (($b > 128) ? $b - $contrast : $b + $contrast)) . $key;
        }
        $x++;
      }
      if (1) {
        print $reset . "  ";
        $chars = "$line"; $x = 0;
        while (1 < length $chars) {
          ($key, $chars) = $chars =~ /(..)(.*)/;
          my ($r, $g, $b) = (196, 255, 128); # Make palette misses very noticeable.
          my $p = $palette{$$t{file}}{$key};
          ($r, $g, $b) = @$p if ref $p;
          print $reset . rgb($r, $g, $b, "bg") . "  ";
          $x++;
        }
      }
    } else {
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
        my $p = $palette{$$t{file}}{$char};
        if (defined $p) {
          print rgb($$p[0], $$p[1], $$p[2]);
        }
        $x++;
        print $char;
      }
      print $reset . "  ";
      $x = 0;
      for my $char (split //, $line) {
        my $p = $palette{$$t{file}}{$char};
        if (defined $p) {
          print $reset . rgb($$p[0], $$p[1], $$p[2], "bg")
            . ((($x == $cursorx) and ($y == $cursory))
               ? (rgb(0,0,0) . "?" . rgb(255,255,255) . "?") : "  ");
        } else {
          print rgb(63,0,0,"bg") . rgb(255,255,127) . $char . $char;
        }
        $x++;
      }
    }
    print $reset . "\n";
    $y++;
  }
  print $reset . "\n";
}

sub showpalettes {
  showpalette($_) for keys %palette;
}

sub showpalette {
  my ($tf) = @_;
  my @pkey = sort { $a cmp $b } keys %{$palette{$tf}};
  if ($palettemode eq 2) {
    # Double-wide-key palettes get large, so we have to do it a different way.
    print "$tf palette: " . (join "", map {
      my $c = $_;
      my ($r, $g, $b) = @{$palette{$tf}{$c}};
      rgb($r, $g, $b, "bg") . rgb(0,0,0) . "$c " . rgb(255,255,255) . $c . $reset #. "($r,$g,$b)"
        . " ";
    } @pkey) . "\n";
  } else {
    print "$tf palette:\n " . (join "", map {
      my $c = $_;
      my ($r, $g, $b) = @{$palette{$tf}{$c}};
      rgb($r, $g, $b, "bg") . "   ";
    } @pkey)
      . $reset . "\n " #. (" " x length $setname) . "          "
      . join ("", map {
        " $_ "
      } @pkey) . "\n";
  }
}

sub rgb { # Return terminal code for a 24-bit color.
  my ($red, $green, $blue, $isbg) = @_;
  my $fgbg = ($isbg) ? 48 : 38;
  my $delimiter = ";";
  return $fallback . "\x1b[$fgbg$ {delimiter}2$ {delimiter}$ {red}$ {delimiter}$ {green}$ {delimiter}$ {blue}m";
}


