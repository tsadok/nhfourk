#!/usr/bin/perl
# -*- cperl -*-
# This file (update-last-updated.pl) is in the public domain.

my @safelicense = (# Most licenses, in fact, are safe, i.e., they don't require what this script does.
                   # Here we list only ones that are used in our project.
                   "GNU General Public License",
                   "public domain",
                  );
my @skipfilere = (# Any filename matching this regular expression pattern will be skipped by this script.
                  qr/[.]png$/,    # We're not going to try to parse image files, no.
                  qr/[.]ico$/,    # Ditto.
                  qr/[.]bz2$/,    # No, just, no.
                  qr/~$/,         # Automatic backup files created when editing.
                  qr/\bcatalogues\b/, # These are too simple and factual to be eligible for copyright protection.
                  qr/\b(COPYING|LICENSE|README.prebuilt|license|gpl)$/,
                 );

my @file = @ARGV;
push @file, "." if not @file;
processfile($_) for @file;

sub updateline {
  my ($oline, $filename, $oldname, $olddate) = @_;
  my ($newname, $newdate);
  my $line = $oline;
  open GIT, "git log -n 1 $filename |";
  while (<GIT>) {
    if (/Author[:]\s+(.*?)\s*$/) {
      $newname = $1;
    } elsif (/Date[:]\s+(Sun|Mon|Tue|Wed|Thu|Fri|Sat)\s+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s*(\d+)\s+(\d+)[:](\d+)[:](\d+)\s+(\d{4})\s+/) {
      my ($dow, $monabbr, $mday, $h, $m, $s, $year) = ($1, $2, $3, $4, $5, $6, $7);
      $newdate = join "-", ($year, $monabbr, sprintf("%02d", $mday));
    }}
  if ($newname =~ /(\w+.*?)\s*[<][^<>]+[>]\s*$/) {
    $newname = $1;
  }
  if ($newname and $newdate) {
    my $i = index($line, $oldname);
    if ($i > 0) {
      substr($line, $i, length($oldname), $newname);
    } else {
      warn "Failed to substitute $newname for $oldname in $filename: $line";
      return $oline;
    }
    $i = index($line, $olddate);
    if ($i > 0) {
      substr($line, $i, length($olddate), $newdate);
    } else {
      warn "Failed to substitute $newdate for $olddate in $filename: $line";
      return $oline;
    }
    return $line;
  }
  use Data::Dumper;
  warn "Fallthrough in updateline(): "
    . Dumper(+{ newname => $newname, newdate => $newdate, oline => $oline, line => $line,
                filename => $filename, oldname => $oldname, olddate => $olddate });
  return $oline;
}

sub processfile {
  my ($filename) = @_;
  for my $re (@skipfilere) {
    return if $filename =~ $re;
  }
  if (-d $filename) {
    return processdir($filename);
  } else {
    open FILE, "<", $filename or die "Cannot read $filename: $!";
    my @line = <FILE>; # Sluuurp
    close FILE;
    for my $n (0 .. 12) {
      if ($line[$n] =~ /[Ll]ast (?:modified|edited|updated) by (.*?), (\d{4}-(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec|\d{2})-\d{2})/) {
        my ($name, $date) = ($1, $2);
        #print "OLD: $line[$n]";
        $line[$n] = updateline($line[$n], $filename, $name, $date);
        #print "NEW: $line[$n]";
        # Actually write the contents of the file back out:
        open FILE, ">", $filename or die "Cannot write $filename: $!";
        print FILE $_ for @line;
        close FILE;
        return $line[$n];
      }
      for my $license (@safelicense) {
        if (index($line[$n], $license) > 0) {
          #print "Skipping $filename: $license does not require last-edited comments\n";
          return "safe";
        }}
    }
    warn "Failed to find last-edited comment to update: $filename\n";
    return;
  }
}

sub processdir {
  my ($dirname) = @_;
  my ($success, $failure) = (0, 0);
  for my $f (<$dirname/*>) {
    if (processfile($f)) {
      $success++;
    } else {
      $failure++;
    }}
  #print "Directory $dirname: $success files updated; $failure failed.\n";
  return ($success / ($failure || 1));
}
