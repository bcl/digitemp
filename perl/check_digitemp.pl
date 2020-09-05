#!/usr/bin/perl -w

# check_digitemp.pl Copyright (C) 2002 by Brian C. Lane <bcl@brianlane.com>
# multi-sensor-support, perfdata, dictionary and additional bugs by Martin <marte> Tettke, 2020
# $Date: 2020-07-23 20:26:40 +0000 (Thu, 23 Jul 2020) $, $Rev: 118 $
#
# This is a NetSaint/nagios/icinga plugin script to check the temperature on a local
# machine. Remote usage may be possible with SSH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# ===========================================================================
# Howto Install (for all monitoring systems)
#
# 0. install digitemp if not already done. At example for debian:
#    apt install digitemp                                # or whatever command in different distros
#    ln -s /usr/bin/digitemp_DS9097 /usr/bin/digitemp    # link the command for the device you use
#    usermod -a -G dialout nagios                        # give nagios access to the serial ports
# 1. create digitemp-config:
#    digitemp -i -s /dev/ttyUSB0 -c /path/to/digitemp.conf
#    (hint: if you add sensors later on, create a new config file and add the new lines to the old config with incremented ROM ID,
#     so the sensors will not be rearrenged - especially important if you store perfdata)
# 2. check if reading the sensors is working:
#    digitemp -c /path/to/digitemp.conf -a -q
# 3. create sensor map (optional, I will call it digitemp.map for reference later) - create a file with lines like this
#    0:my_sensor_0
#    1:my_sensor_1
#    (important: if you change names later on, you will have a new sensor in perfdata, so choose names wisely!)
# 4. copy this nagios/icinga script to a place you like and test it with digitemp config (and map)
#    /path/to/check_digitemp.pl -f /path/to/digitemp.conf -w 25 -c 35 -C -m /path/to/digitemp.map
#    (skip "-C" for temperature in fahrenheit and skip "-m..." if you don't use a map)
#    (if you want to read only one sensor, add "-t <SensorID>")
# 5. if everything works, you are ready for nagios/icinga config
#
# ===========================================================================
# Howto Install NetSaint
#
# 1. Add a command to /usr/local/netsaint/etc/commands.cfg like this:
#    command[check-temp]=$USER1$/check_digitemp.pl -w $ARG1$ -c $ARG2$ \
#    -t $ARG3$ -f $ARG4$
#    (fold into one line)
#
# 2. Tell NetSaint to monitor the temperature by adding a service line like
#    this to your hosts.cfg file:
#    service[kermit]=Temperature;0;24x7;3;5;1;home-admins;120;24x7;1;1;1;; \
#    check-temp!65!75!1!/usr/local/netsaint/etc/digitemp.conf
#    (fold into one line)
#    65 is the warning temperature
#    75 is the critical temperature
#    1 is the sensor # (as reported by digitemp -a) to monitor
#    digitemp.conf is the path to the config file
#
# 3. If you use Centigrade instead of Fahrenheit, change the commands.cfg
#    line to include the -C argument. You can then pass temperature limits in
#    Centigrade in the service line.
#
# ===========================================================================
# Howto Install Nagios
#
# 1. Add a command to /usr/local/nagios/etc/checkcommands.cfg like this:
#
#    #DigiTemp temperature check command
#    define command{
#        command_name    check_temperature
#        command_line    $USER1$/check_digitemp.pl -w $ARG1$ -c $ARG2$ \
#        -t $ARG3$ -f $ARG4$
#    (fold above into one line)
#        }
#
# 2. Tell NetSaint to monitor the temperature by adding a service line like
#    this to your service.cfg file:
#
#    #DigiTemp Temperature check Service definition
#    define service{
#        use                         generic-service
#        host_name                       kermit
#        service_description             Temperature
#        is_volatile                     0
#        check_period                    24x7
#        max_check_attempts              3
#        normal_check_interval           5
#        retry_check_interval            2
#        contact_groups                  home-admins
#        notification_interval           240
#        notification_period             24x7
#        notification_options            w,u,c,r
#        check_command                   check_temperature!65!75!1!  \
#        /usr/local/nagios/etc/digitemp.conf
#        (fold into one line)
#        }
#
#    65 is the warning temperature
#    75 is the critical temperature
#    1 is the sensor # (as reported by digitemp -a) to monitor
#    digitemp.conf is the path to the config file
#
# 3. If you use Centigrade instead of Fahrenheit, change the checkcommands.cfg
#    line to include the -C argument. You can then pass temperature limits in
#    Centigrade in the service line.
#
# ===========================================================================
# Howto Install Icinga2
#
# 1. create COMMAND definition:
#    object CheckCommand "check_digitemp" {
#      import "plugin-check-command"
#      command = [ "/usr/local/scripts/check_digitemp.pl" ]
#      arguments = {
#        "-f" = {
#          value = "/usr/local/scripts/digitemp.conf"
#          required = false
#        }
#        "-m" = {
#          value = "/usr/local/scripts/digitemp.map"
#          required = false
#        }
#        "-w" = {
#          value = "$temp_warning$"
#          required = false
#        }
#        "-c" = {
#          value = "$temp_critical$"
#          required = false
#        }
#        "-C" = {
#          required = false
#        }
#      }
#    }
#
# 2. create SERVICE definition:
#    apply Service "digitemp" {
#      import "generic-service"
#      check_interval = 15m
#      check_command = "check_digitemp"
#      enable_perfdata = true
#      vars.notification_interval = 1h
#      vars.temp_critical = "35"
#      vars.temp_warning = "25"
#      command_endpoint = host.vars.client_endpoint
#      assign where host.vars.has_digitemp
#   }
#
# 3. add the following line to your HOST definitions for hosts with sensors:
#    vars.has_digitemp=1
#
# 4. check config and reload icinga2
#    icinga2 daemon -C
#    service icinga2 reload
#
# ===========================================================================

# Modules to use
use strict;
use warnings;
use Getopt::Std;

# Define all our variable usage
use vars qw($opt_c $opt_f $opt_t $opt_w $opt_F $opt_C $opt_m
            $temperature $conf_file $sensor $temp_fmt
            $crit_level $warn_level
            %exit_codes $exitcode $exitstate
            @sensors $iSensor $perf
            $percent $fmt_pct 
            $verb_err $command_line
            $mapfile %maphash $sensorname);

# Predefined exit codes for NetSaint
%exit_codes   = (   'OK'      , 0,
                    'WARNING' , 1,
                    'CRITICAL', 2,
                    'UNKNOWN' , 3,);

# Default to Fahrenheit input and result (use -C to change this)
$temp_fmt = 3;


# Get the options
if ($#ARGV le 0)
{
  &usage;
} else {
  getopts('f:t:FCc:w:m:');
}

# Shortcircuit the switches
if (!$opt_w or $opt_w == 0 or !$opt_c or $opt_c == 0)
{
  print "*** You must define WARN and CRITICAL levels!";
  &usage;
}

if ( $opt_m )
{
  $mapfile = $opt_m;
  if ( -f $mapfile && -r $mapfile ) {
    open(FILE, $mapfile) or die;
    while (<FILE>)
    {
       chomp;
       my ($key, $val) = split /:/;
       $maphash{$key}  = $val;
    }
    close(FILE);
  } else {
    die "ERROR: mapfile $mapfile not found";
  }
}

# Check if levels are sane
if ($opt_w >= $opt_c)
{
  print "*** WARN level must not be greater than CRITICAL when checking temperature!";
  &usage;
}

$warn_level   = $opt_w;
$crit_level   = $opt_c;

# Default read all sensors
if( !defined($opt_t) )
{
  $sensor = "all";
} else {
  $sensor = $opt_t;
}

# Default config file is /etc/digitemp.conf
if(!$opt_f)
{
  $conf_file = "/etc/digitemp.conf";
} else {
  $conf_file = $opt_f;
}

# Check for config file
if( !-f $conf_file ) {
  print "*** You must have a digitemp.conf file\n";
  &usage;
}


if($opt_C)
{
  $temp_fmt = 2;
}

# Read the output from digitemp
# Output in form 0\troom\tattic\tdrink
if ( $sensor eq "all" )
{
  open( DIGITEMP, "/usr/bin/digitemp -c $conf_file -a -q -o $temp_fmt |" );
} else {
  open( DIGITEMP, "/usr/bin/digitemp -c $conf_file -t $sensor -q -o $temp_fmt |" );
}

# we start with thinking everything is fine:
$exitstate = "OK";
$exitcode  = $exit_codes{'OK'};

# Process the output from the command
while( <DIGITEMP> )
{
  chomp;

  if( $_ =~ /^nanosleep/i )
  {
    print "Error reading sensor #$sensor\n";
    close(DIGITEMP);
    exit $exit_codes{'UNKNOWN'};
  } else {
    # Check for an error from digitemp, and report it instead
    if( $_ =~ /^Error.*/i ) {
      print $_;
      close(DIGITEMP);
      exit $exit_codes{'UNKNOWN'};
    } else {
      @sensors = split(/\t/);
      # strip first "null" entry:
      shift @sensors;
    }
  }
}
close( DIGITEMP );

# walk through sensors, check warn and crit
foreach(@sensors)
{
  if ( $_ >= $crit_level )
    {
      $exitstate = "CRITICAL";
      $exitcode  = $exit_codes{'CRITICAL'};
    } elsif ( $_ >= $warn_level && $exitcode ne $exit_codes{'CRITICAL'} ) {
      $exitstate = "WARNING";
      $exitcode  = $exit_codes{'WARNING'};
    }
}

# walk again, generate output.
$perf="";
print "Temperature ".$exitstate." - ";
for my $iSensor (0 .. $#sensors)
{
  if ( $sensor eq "all" )
  {
    $sensorname = $iSensor;
  } else {
    $sensorname = $sensor;
  }
  $sensorname = $maphash{$sensorname} if ( exists($maphash{$sensorname}) );
  $perf .= $sensorname."=".$sensors[$iSensor].";".$warn_level.";".$crit_level." ";
  print "Sensor ".$sensorname." = ".$sensors[$iSensor]." ";
  if( $temp_fmt == 3 ) { print "F "; } else { print "C "; }
}
print "| ".$perf."\n";
exit $exitcode;

# Show usage
sub usage()
{
  print "\ncheck_digitemp.pl v1.0 - NetSaint Plugin\n";
  print "Copyright 2002 by Brian C. Lane <bcl\@brianlane.com>\n";
  print "changes 2020 by Martin Tettke <marte\@xmn-berlin.de>\n";
  print "See source for License\n";
  print "usage:\n";
  print " check_digitemp.pl [-t <sensor>]  [-m <mapfile>] [-F] [-C] -f <config file> -w <warnlevel> -c <critlevel>\n\n";
  print "options:\n";
  print " -t               Option DigiTemp Sensor # (defaults to all sensors)\n";
  print " -m mapfile       Optional map file with lines \"<#sensorID>:<description>\" to translate sensor IDs to names\n";
  print " -F               Temperature in Fahrenheit\n";
  print " -C               Temperature in Centigrade\n";
  print " -f               DigiTemp Config File\n";
  print " -w temperature   temperature >= to warn\n";
  print " -c temperature   temperature >= when critical\n";

  exit $exit_codes{'UNKNOWN'}; 
}
