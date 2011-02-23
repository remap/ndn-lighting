insert into PS_SCRAPER (ID, TYPE_ID, DESCRIPTION, SIG, SCRAPER) values (1, 1, 'DNSKEY Perl Processor', '', 'select(STDOUT);$|++;while($s=<>){chomp($s);if($s=~m/\S+\s+\S+\s+\S+\s+\S+\s+(\S+)\s+(\S+)\s+(\S+)\s+\(?([0-9a-zA-Z+\/=]+)\)?\s*;?$/){push(@l,"$1 $2 $3 $4")}}@l2=sort(@l);foreach $s (@l2){print "$s\n"}');
--insert into PS_SCRAPER (ID, TYPE_ID, DESCRIPTION, SIG, SCRAPER) values (1, 1, 'DNSKEY Processor', '', 'select(STDOUT);$|++;while($s=<>){chomp($s);print "IT IS: $s\n\n";if($s=~m/\S+\s+\S+\s+\S+\s+\S+\s+\S+\s+\(?([0-9a-zA-Z+\/=]+)\)?\s*;?$/){print("$1 - DOOKIE\n")}};close(STDOUT);sleep 3;');
insert into PS_SCRAPER (ID, TYPE_ID, DESCRIPTION, SIG, SCRAPER) values (2, 2, 'DNSKEY Native Processor', '', '');
insert into PS_SCRAPER (ID, TYPE_ID, DESCRIPTION, SIG, SCRAPER) values (3, 1, 'SecSpider DNSKEY Web Perl Processor', '', 'select(STDOUT);$|++;while($s=<>){chomp($s);@l=split(/\s+/,$s);if(scalar(@l)>=5){$k="";if(uc($l[2]) eq "ZSK"){$k="256 3 "}else{$k="257 3 "}if(uc($l[3]) eq "RSASHA1"){$k.="5 "}elsif(uc($l[3]) eq "RSASHA1-NSEC3-SHA1"){$k.="7 "}else{$k.="? "};$k.=$l[4];push(@l2,$k);}}foreach $s (sort(@l2)){print "$s\n"}');
insert into PS_SCRAPER (ID, TYPE_ID, DESCRIPTION, SIG, SCRAPER) values (4, 1, 'RIPE TAR Web Perl Processor', '', '$z=shift;
if(!defined($z))
{
  $z="";
}
$k="";
$b="0";
while($s=<>)
{
  chomp($s);
  if("$b" eq "1")
  {
    if($s =~ /.+";/)
    {
      $b="0";
    }
    $s=~s/[\";\s]+//g;
    $k.=$s;
  }
  elsif($s=~/^"$z"\s+(257\s+3.+$)/)
  {
    $k=$1;
    $k=~s/\s+/ /g;
    $k.=" ";
    $b="1";
  }
}
print "$k\n";');
