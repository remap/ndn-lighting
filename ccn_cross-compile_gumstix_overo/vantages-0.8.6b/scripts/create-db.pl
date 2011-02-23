#!/usr/bin/perl -w

use strict;

sub _usage
{
  print("create-db.pl <DB srouce root dir> <SQLite3 file> | [ -h ]\n");
}

my $sSrcDir = shift;
my $sDB = shift;

if (!defined($sSrcDir))
{
  warn("Unable to create w/ no src dir.");
  _usage();
}
elsif ("-h" eq $sSrcDir)
{
  _usage();
}
elsif (!defined($sDB))
{
  warn("Must specify DB file.");
  _usage();
}
elsif (! -e $sSrcDir)
{
  warn("Src Dir: '$sSrcDir' DNE.");
}
elsif (! -e "$sSrcDir/ddl")
{
  warn("Src Dir: '$sSrcDir/ddl' DNE.");
}
elsif (! -e "$sSrcDir/dml")
{
  warn("Src Dir: '$sSrcDir/dml' DNE.");
}
else
{
  unlink($sDB);

  my $sFile = undef;
  foreach $sFile (`ls $sSrcDir/ddl/*.sql`)
  {
    chomp($sFile);
    system("cat $sFile | sqlite3 $sDB");
  }

  foreach $sFile (`ls $sSrcDir/dml/*.sql`)
  {
    chomp($sFile);
    system("cat $sFile | sqlite3 $sDB");
  }
}
