#!/usr/bin/perl -w
#
# Copyright (c) 2003 Hin-Tak Leung
#
## Just a reimplementation of the ps2epl shell script in perl with the 
## added flexibility of command-line override.

use strict;

my $papersize='a4';

my $model='EPL5700L';

my %IJSPARAMS = (
    'EplDpi'         => undef,  # 300, Class600, 600, Class1200           - default 600 
    'EplRitech'      => undef,  # on, off                                 - default on
    'EplDensity'     => undef,  # 1,2,3,4,5                               - default 3
    'EplTonerSave'   => undef,  # on, off                                 - default off
    'EplFlowControl' => undef   # off, libusb, libieee1284, "/dev/lp0"... - default off 
    );

my $infile;
my $outfile;

sub print_usage {
print <<'EOF';
Usage: 
   ps2epl.pl <postscript file>  
   ps2epl.pl <postscript file> <outputfile name>
   ps2epl.pl <postscript file> <outputfile name> <option lists>

Example:
   ps2epl.pl myfile.ps /dev/null model=EPL5700L flowcontrol=/dev/usb/lp0

Available options:
   model        = EPL5700L, EPL5800L, EPL5900L, EPL6100L
   papersize    = a4, letter, (anything that ghostscript accepts)

   dpi          = 300, Class300, 600, Class1200
   ritech       = on, off
   density      = 1, 2, 3, 4, 5
   tonersave    = on, off
   flowcontrol  = libusb, libiee1284, /dev/usb/lp0, /dev/lp0 (and other devices) 

EOF
}


#1st argument - mandatory input file name, or print usage and exit

unless ($infile = shift @ARGV) {
    &print_usage();
    exit;
}

#2nd argument - optional output file name, or generate from input file name  
unless ($outfile = shift @ARGV) {
    my $base = `basename $infile`;
    $base =~ s/\.eps$//;
    $base =~ s/\.ps$//;
    $outfile = "${base}.epl";
}

#The rest of the arguments
while(my $pairs = shift @ARGV) {
    if ($pairs =~ /=/) {
	my ($option, $value) = split /=/, $pairs;
	if ($option =~ /^papersize$/i) {
	    $papersize = $value;
	} elsif ($option =~ /^model$/i) {
	    $model = $value;
	} else {
	    foreach my $key (keys %IJSPARAMS) {
		if (($key =~ /${option}$/i) && ((length $option) > 3)) {
		    $IJSPARAMS{$key} = $value ;
		}
	    }
	}
    }
}

# override whatever the user put in as output file name
if ((defined($IJSPARAMS{'EplFlowControl'})) && ($IJSPARAMS{'EplFlowControl'} ne 'off')) {
    $outfile = "/dev/null";
}

my $ijsparams = "";

foreach my $key (keys %IJSPARAMS) {
    if (defined($IJSPARAMS{$key})) {
	$ijsparams .= $key . '=' . $IJSPARAMS{$key} . ',';	
    }
}

if (length $ijsparams) {
    chop $ijsparams;  # remove the last extra comma
}

print "Model = ${model} , Papersize = ${papersize}, IjsParams=${ijsparams}\n";

$ENV{'PATH'} = ".:" . $ENV{'PATH'};

exec ("gs",
      "-sPAPERSIZE=${papersize}",
      "-dFIXEDMEDIA",	  
      "-sProcessColorModel=DeviceGray",
      "-dBitsPerSample=1",	  
      "-sDEVICE=ijs",
      "-sIjsServer=ijs_server_epsonepl",
      "-sDeviceManufacturer=Epson",
      "-sDeviceModel=${model}",
      "-sIjsParams=${ijsparams}",
      "-dIjsUseOutputFD", 
      "-dNOPAUSE", 
      "-dSAFER", 
      "-dBATCH", 
      "-sOutputFile=${outfile}",
      "$infile"
      );
