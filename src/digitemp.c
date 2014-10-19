/* -----------------------------------------------------------------------
   DigiTemp

   Copyright 1996-2007 by Brian C. Lane <bcl@brianlane.com>
   All Rights Reserved

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

			digitemp -w											Walk the LAN & show all
			digitemp -i											Initalize .digitemprc file
			digitemp -I											Initialize .digitemprc w/sorted serial #s
			digitemp -s/dev/ttyS0						Set serial port (required)
			digitemp -cdigitemp.conf				Configuration File
			digitemp -r1000									Set Read timeout to 1000mS
			digitemp -l/var/log/temperature	Send output to logfile
			digitemp -v											Verbose mode
			digitemp -t0										Read Temperature
			digitemp -q											Quiet, no copyright banner
			digitemp -a											Read all Temperatures
			digitemp -d5										Delay between samples (in sec.)
			digitemp -n50										Number of times to repeat. 0=forever
			digitemp -A											Treat DS2438 as A/D converter
			digitemp -o1										Output format for logfile
			digitemp -p10000								window in us between Lan calls
			digitemp -o"output format string"	See description below
			digitemp -O"counter format"				See description below
			digitemp -H"Humidity format"			See description below

     Logfile formats:
     1 = (default) - 1 line per sensor, time, C, F
         1 line for each sample, elapsed time, sensor #1, #2, ... tab seperated
     2 = Reading in C
     3 = Reading in F

     The format string uses strftime tokens plus 6 special
     ones for digitemp - %s for sensor #, %C for centigrade,
     %F for fahrenheit, %R for hex serial number, %N for seconds since Epoch

     Humidity uses %h for the relative humidity in percent

     The counter format uses %n for the counter # and %C for the count
     in decimal

     Remember the case of the token is important!

   =======================================================================
   See ChangeLog file for history of changes
   -----------------------------------------------------------------------*/
/* ---------------------------------------------------------------------
	Notes while modifying v3.6.0 into v3.6.0_DRJ
	
   	2014-08-16: DRJ
   		1) While reading temperatures, occationally getting the following errors:
   			ERROR: ownet.c owAccess() SerialNum does not match.
   			ERROR: DRJ_digitemp.c FamilyEF_SetChannels() Failed to gain access to device-0
				ERROR: digitemp.c read_temperature() CRC Failed. CRC is 63 instead of 0x00
			Maybe a minimum time between LAN activity will help?
			Keep track of this time from digitemp.c so that serial/usb does not matter.
			Methods that talk on the LAN:
				owWriteBytePower()
				owAccess()
				owLevel()
				owBlock()
				owWriteByte()
				owReadByte()
			Proposed Methods:
				DrjOwAccess() 

				ownetu.c	owAccess()?
				ownet.c	owAccess()?
				owtrnu.c	owBlock()?
				owtran.c owBlock()?
				linuxlnk.c	OpenCOM(int, char*);
				linuxlnk.c	WriteCOM(int, int, uchar*);
				linuxlnk.c	CloseCOM(int);
				linuxlnk.c	FlushCOM(int);
				linuxlnk.c	ReadCOM(int, int, uchar*);
				linuxlnk.c	BreakCOM(int);
				
				usblnk.c	owTouchReset(int);
				usblnk.c	owTouchBit(int,SMALLINT);
				usblnk.c	owTouchByte(int,SMALLINT);
				usblnk.c	owWriteByte(int,SMALLINT);
				usblnk.c	owReadByte(int);
				usblnk.c	owSpeed(int,SMALLINT);
				usblnk.c	owLevel(int,SMALLINT);
				usblnk.c	owProgramPulse(int);
				

			2)	If program is stopped and restarted, occationally the following errors:
				dusty@DRJs-Nuke ~/Digitemp_DRJ_2014-07-04 $ sudo ./digitemp_DS2490 -a -n 0
				DigiTemp v3.6.0-DRJ Copyright 1996-2007 by Brian C. Lane
				GNU Public License v2.0 - http://www.digitemp.com
				Found DS2490 device #1 at 2/7
				Error: owTouchReset() buffer[0x00] = 01
				ERROR: ownet.c owAccess() No Devices on Net.
				ERROR: DRJ_digitemp.c FamilyEF_SetChannels() Failed to gain access to device-0
				Error: owTouchReset() buffer[0x00] = 01
				ERROR: ownet.c owAccess() No Devices on Net.
				Error: owTouchReset() buffer[0x00] = 01
				Error: owTouchReset() buffer[0x00] = 01
				ERROR: ownet.c owAccess() No Devices on Net.
			To clear this, the USB device needs to be unplugged and replugged.
			Could this be handled during Init1WireLan()?


    2014-07-05: DRJ ToDo
    	1)	Turn off all hubs does not search for netsted hubs to turn off.
    	2)	CRC errors happen.  Uncertain of frequencey, but mabye 1/20?
    			Maybe enforcing a time interval between libusb calles will help?
    	
   	2014-07-04: DRJ
   		owTouchReset located in usblnk.c.  This meathod will often
   		not work if the code is run without outputting to the terminal.  The fault
   		is with not "zeroing" the buffer.  It is now zeroed useing memset.

		2014-06-29: DRJ
		 libusb 1.0 meathods used:
				libusb_init()
				libusb_set_debug()
				libusb_get_device_list()
				libusb_get_device_descriptor()
				libusb_open()
				libusb_set_configuration()
				libusb_close()
				libusb_control_transfer()
				libusb_bulk_transfer()
				libusb_clear_halt()


		2013-07-08: DRJ
			Added family code "0xEF Hobby Boards Microprocessor-based slave"
			Added support for "4CH2-R1-A Hobby Boards 4ch Hub"
			A strange thing... printf is causing problems!?!?!?

		2013-07-21: DRJ
			The calls to functions like owTouchReset() are not working dependably, especially
			when running digitemp without the -q flag.  Is this a USB driver issue?
			Considering using libusb_bulk_transfer() instead of usb_bulk_read()
			Generally, consider moving from libusb v0.1.7 to libusbx-1.0.16

			http://libusb.sourceforge.net/doc/index.html  <-- v0.1.7

			suspected libusb v0.1.7 meathods used:
				usb_control_msg
				usb_bulk_read
				usb_clear_halt
			suspected libusb v0.1.7 variables used:
				usb_dev_handle_list

			Who is using usb.h?
				usblnk.c
				usbses.c

		2013-07-21: DRJ
			The HobbyBoard 4ch hub is different enough from the DS2409 that it may be easier
			to treat them separately.
			struct _coupler		renamed to		_coupler_ds2409
			free_coupler()		renamed to		free_coupler_ds2409()
			num_sensors				renamed to		numDS_sensors

			new method											free_coupler_HB()
			new struct											_coupler_hobbyboard
			new unsighned char							lastHBcoupoler
			new int													numHB_sensors
   -----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#if !defined(AIX) && !defined(SOLARIS) && !defined(FREEBSD) && !defined(DARWIN)
#include <getopt.h>
#endif /* !AIX and !SOLARIS and !FREEBSD and !DARWIN */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <strings.h>
#include <stdint.h>
#include "ad26.h"

// Include endian.h
#if DARWIN
#include <machine/endian.h>
#endif
#if FREEBSD
#include <sys/endian.h>
#endif
#if !defined(DARWIN) && !defined(FREEBSD)
#include <endian.h>
#endif

#ifdef LINUX
#ifndef OWUSB
#ifdef LOCKDEV
#include <lockdev.h>
#endif
#endif
#endif

#include "digitemp.h"
#include "DRJ_digitemp.h"
#include "device_name.h"
#include "ownet.h"
#include "owproto.h"


/* Setup the correct getopt starting point */
#ifdef LINUX
#define GETOPTEOF -1
#define OPTINDSTART 0
#endif

#ifdef CYGWIN
#define GETOPTEOF -1
#define OPTINDSTART 0
#endif

#ifdef AIX
#define OPTINDSTART 0
#define GETOPTEOF 255
#endif

#ifdef SOLARIS
#define GETOPTEOF EOF
#define OPTINDSTART 1
#endif

#ifdef FREEBSD
#define GETOPTEOF EOF
#define OPTINDSTART 1
#endif

#ifdef OPENBSD
#define GETOPTEOF EOF
#define OPTINDSTART 1
#endif

#ifdef NETBSD
#define GETOPTEOF EOF
#define OPTINDSTART 1
#endif

#ifdef DARWIN
#define GETOPTEOF EOF
#define OPTINDSTART 1
#endif

#ifdef OTHER
#define GETOPTEOF EOF
#define OPTINDSTART 1
#endif


/* For tracking down strange errors */
#undef BCL_DEBUG

extern char 	*optarg;
extern int	optind, opterr, optopt;

#if defined(FREEBSD) || defined(DARWIN)
extern int optreset;
#endif /* FREEBSD or DARWIN */

// Global Variables
extern const char dtlib[];		// Library Used

char serial_port[40],					// Path to the serial port
     tmp_serial_port[40],
     serial_dev[40],					// Device name without /dev/
     log_file[1024],					// Path to the log file
     tmp_log_file[1024],
     temp_format[80],					// Format for temperature readings
     tmp_temp_format[80],
     counter_format[80],			// Format for counter readings
     tmp_counter_format[80],
     humidity_format[80],			// Format for Humidity readings
     tmp_humidity_format[80],
     conf_file[1024],					// Configuration File
     option_list[40];
int	read_time,								// Pause during read
		tmp_read_time,
		log_type,									// output format type
		tmp_log_type,
		temp_LAN_activity_window_us,
		opts = 0;									// Bitmask of flags
struct timeval	LAN_activity_last_time;		// last time of LAN activity
int				LAN_activity_window_us = 20000;		// LAN activity time window in micro-seconds.

Rom*			global_roms_list;				// Attached Roms
short			global_roms_count;			// number of roms in roms_list

Rom*			global_sensor_list;			// Attached Sensors
short			global_sensor_count;		// number of sensors in sensor_list

Rom*			global_coupler_list;		// Attached Roms that are couplers needing further investigation
short			global_coupler_count;		// number of couplers in coupler_list


int			lastCouplerNum = -1;		// last selected coupler num
int			sensor_count = 0;				// number of sensors
int			coupler_count = 0;				// number of couplers

//unsigned char lastHBcoupoler[9];							// Last selected coupler

int	global_msec = 10;			/* For ReadCOM delay       */
int	global_msec_max = 15;

/* ----------------------------------------------------------------------- *
   Print out the program usage
 * ----------------------------------------------------------------------- */
void usage()
{
  printf(BANNER_1);
  printf(BANNER_2);
  printf(BANNER_3, dtlib );		         /* Report Library version */
  printf("\nUsage: digitemp [-s -i -I -U -l -r -v -t -a -d -n -o -c]\n");
  printf("                -i                            Initalize .digitemprc file\n");
  printf("                -I                            Initalize .digitemprc file w/sorted serial #s\n");
  printf("                -w                            Walk the full device tree\n");
  printf("                -s /dev/ttyS0                 Set serial port\n");
  printf("                -l /var/log/temperature       Send output to logfile\n");
  printf("                -c digitemp.conf              Configuration File\n");
  printf("                -r 1000                       Read delay in mS\n");
  printf("                -v                            Verbose output\n");
  printf("                -t 0                          Read Sensor #\n");
  printf("                -q                            No Copyright notice\n");
  printf("                -p 10000                       window in us between Lan calls\n");
  printf("                -a                            Read all Sensors\n");
  printf("                -d 5                          Delay between samples (in sec.)\n");
  printf("                -n 50                         Number of times to repeat\n");
  printf("                                              0=loop forever\n");
  printf("                -A                            Treat DS2438 as A/D converter\n");
  printf("                -O\"counter format string\"      See description below\n");
  printf("                -o 2                          Output format for logfile\n");
  printf("                -o\"output format string\"      See description below\n");
  printf("                -H\"Humidity format string\"    See description below\n");
  printf("\nLogfile formats:  1 = One line per sensor, time, C, F (default)\n");
  printf("                  2 = One line per sample, elapsed time, temperature in C\n");
  printf("                  3 = Same as #2, except temperature is in F\n");
  printf("        #2 and #3 have the data seperated by tabs, suitable for import\n");
  printf("        into a spreadsheet or other graphing software.\n");
  printf("\n        The format string uses strftime tokens plus 5 special ones for\n");
  printf("        digitemp - %%s for sensor #, %%C for centigrade, %%F for fahrenheit,\n");
  printf("        %%R to output the hex serial number, and %%N for seconds since Epoch.\n");
  printf("        The case of the token is important! The default format string is:\n");
  printf("        \"%%b %%d %%H:%%M:%%S Sensor %%s C: %%.2C F: %%.2F\" which gives you an\n");
  printf("        output of: May 24 21:25:43 Sensor 0 C: 23.66 F: 74.59\n\n");
  printf("        The counter format string has 2 special specifiers:\n");
  printf("        %%n is the counter # and %%C is the count in decimal.\n");
  printf("        The humidity format uses %%h for the humidity in percent\n\n");
}


/* ----------------------------------------------------------------------- *
   Free up all memory used by the coupler list
 * ----------------------------------------------------------------------- */
void free_coupler_mem( int free_only )	// DRJ: No longer used
{
	// we shoud free any memory that was set aside for the couplers upon exiting
	// if free_only is set, do not bother to turn off channels

/*
  unsigned char   a[3];
  struct _coupler_ds2409 *c;

  c = couplerDS_top;
  while(c){

		// is this a DS2409???
     // Turn off the Coupler
     if (!free_only)
       SetSwitch1F(0, c->SN, ALL_LINES_OFF, 0, a, TRUE);

    // Free up the serial number lists
    if( c->num_main > 0 )
      free( c->main );

    if( c->num_aux > 0 )
      free( c->aux );

    // Point to the next in the list
    couplerDS_top = c->next;

    // Free up the current entry
    free( c );

    c = couplerDS_top;
  } // Coupler free loop

  // Make sure its null
  couplerDS_top = NULL;

  */
}




/* -----------------------------------------------------------------------
   Convert degrees C to degrees F
   ----------------------------------------------------------------------- */
float c2f(float temp)
{
  return 32 + ((temp*9)/5);
}


/* -----------------------------------------------------------------------
   Take the log_format string and parse out the
   digitemp tags (%*s %*C and %*F) including any format
   specifiers to pass to sprintf. Build a new string
   with the strftime tokens and the temperatures mixed
   together

   If humidity is <0 then it is invalid
   ----------------------------------------------------------------------- */
int build_tf( char *time_format, char *format, int sensor,
              float temp_c, int humidity, unsigned char *sn )
{
  char	*tf_ptr,
  	*lf_ptr,
// DRJ!  	*lf_ptr2,
  	*tk_ptr,
  	token[80],
  	temp[80];

  if( !time_format || !format )
    return 0;

  tf_ptr = time_format;
  lf_ptr = format;

  while( *lf_ptr )
  {
    if( *lf_ptr != '%' )
    {
      *tf_ptr++ = *lf_ptr++;
    } else {
      /* Found a token, decide if its one of ours... */
      /* save initial pointer, grab everything up to... */
// DRJ!      lf_ptr2 = lf_ptr;
      tk_ptr = token;

      /*
         At this point it has a potential format specifier, copy it over
         to the token variable, up to the alpha-numeric specifier.

	 It needs to stop copying after it gets the alpha character
      */
      while( isalnum( *lf_ptr ) || (*lf_ptr == '.') || (*lf_ptr == '*')
             || (*lf_ptr == '%') )
      {
        *tk_ptr++ = *lf_ptr++;
        *tk_ptr = 0;

	/* Break out when the alpha character is copied over */
	if( isalpha( *(lf_ptr-1) ) )
	  break;
      }

      /* see if the format specifier is digitemp or strftime */
      switch( *(tk_ptr-1) )
      {
        case 's' :
        	/* Sensor number */
	        /* Change the specifier to a d */
	        *(tk_ptr-1) = 'd';

	        /* Pass it through sprintf */
	        sprintf( temp, token, sensor );

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;
        	break;

        case 'h' :
        	/* Relative humidity % */
	        /* Change the specifier to a d */
	        *(tk_ptr-1) = 'd';

	        /* Pass it through sprintf */
	        sprintf( temp, token, humidity );

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;
        	break;

        case 'F' :
        	/* Degrees Fahrenheit */
	        /* Change the specifier to a f */
	        *(tk_ptr-1) = 'f';

	        /* Pass it through sprintf */
	        sprintf( temp, token, c2f(temp_c) );

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;

        	break;

        case 'C' :
        	/* Degrees Centigrade */
                /* Change the specifier to a f */
	        *(tk_ptr-1) = 'f';

	        /* Pass it through sprintf */
	        sprintf( temp, token, temp_c );

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;
        	break;

        case 'R' :
        	/* ROM Serial Number */
                /* Change the specifier to a hex (x) */
	        *(tk_ptr-1) = 'X';

                /* Insert the serial number in HEX, yes its ugly,
                   but it works and saves using another temporary
                   location and variable.
                */
                sprintf( temp, "%02X%02X%02X%02X%02X%02X%02X%02X",
                         sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7]);

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;
        	break;

        case 'N' :
        	/* Seconds since Epoch */
	        /* Change the specifier to a s and pass to time */
	        *(tk_ptr-1) = 's';

        	/* Intentional fall through */
        default:
		/* Not something for us, copy it into the time format */
        	tk_ptr = token;
        	while( *tk_ptr )
        	  *tf_ptr++ = *tk_ptr++;
        	break;
      }
    }

  }

  /* Terminate the string */
  *tf_ptr = 0;


  return 1;
}


/* -----------------------------------------------------------------------
   Take the log_format string and parse out the
   digitemp tags (%*s %*C and %*F) including any format
   specifiers to pass to sprintf. Build a new string
   with the strftime tokens and the temperatures mixed
   together
   ----------------------------------------------------------------------- */
int build_cf( char *time_format, char *format, int sensor, int page,
              unsigned long count, unsigned char *sn )
{
  char	*tf_ptr,
  	*lf_ptr,
// DRJ!  	*lf_ptr2,
  	*tk_ptr,
  	token[80],
  	temp[80];

  if( !time_format || !format )
    return 0;

  tf_ptr = time_format;
  lf_ptr = format;

  while( *lf_ptr )
  {
    if( *lf_ptr != '%' )
    {
      *tf_ptr++ = *lf_ptr++;
    } else {
      /* Found a token, decide if its one of ours... */
      /* save initial pointer, grab everything up to... */
// DRJ!      lf_ptr2 = lf_ptr;
      tk_ptr = token;

      /* Take numbers, astrix, period and letters */
      while( isalnum( *lf_ptr ) || (*lf_ptr == '.') ||
             (*lf_ptr == '*') || (*lf_ptr == '%') )
      {
        *tk_ptr++ = *lf_ptr++;
        *tk_ptr = 0;
      }

      /* see if the format specifier is digitemp or strftime */
      switch( *(tk_ptr-1) )
      {
        case 's' :
        	/* Sensor number */
	        /* Change the specifier to a d */
	        *(tk_ptr-1) = 'd';

	        /* Pass it through sprintf */
	        sprintf( temp, token, sensor );

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;
        	break;

        case 'F' :
        	break;

        case 'n' :
                /* Show the page/counter # (0 or 1) */
	        /* Change the specifier to a d */
	        *(tk_ptr-1) = 'd';

	        /* Pass it through sprintf */
	        sprintf( temp, token, page );

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;
                break;

        case 'C' :
        	/* Counter reading, 32 bit value */
                /* Change the specifier to a ld */
	        *(tk_ptr-1) = 'l';
	        *(tk_ptr) = 'd';
	        *(tk_ptr+1) = 0;

	        /* Pass it through sprintf */
	        sprintf( temp, token, count );

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;
        	break;

        case 'R' :
        	/* ROM Serial Number */
                /* Change the specifier to a hex (x) */
	        *(tk_ptr-1) = 'X';

                /* Insert the serial number in HEX, yes its ugly,
                   but it works and saves using another temporary
                   location and variable.
                */
                sprintf( temp, "%02X%02X%02X%02X%02X%02X%02X%02X",
                         sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7]);

		/* Insert this into the time format string */
		tk_ptr = temp;
		while( *tk_ptr )
		  *tf_ptr++ = *tk_ptr++;
        	break;

        case 'N' :
        	/* Seconds since Epoch */
	        /* Change the specifier to a s and pass to time */
	        *(tk_ptr-1) = 's';

        	/* Intentional fall through */
        default:
		/* Not something for us, copy it into the time format */
        	tk_ptr = token;
        	while( *tk_ptr )
        	  *tf_ptr++ = *tk_ptr++;
        	break;
      }
    }
  }

  /* Terminate the string */
  *tf_ptr = 0;

  return 1;
}


/* -----------------------------------------------------------------------
   Print a string to the console or the logfile
   ----------------------------------------------------------------------- */
int log_string( char *line )
{
  int fd=0;

  if( log_file[0] != 0 )
  {
    if( (fd = open( log_file, O_CREAT | O_WRONLY | O_APPEND,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ) ) == -1 )
    {
      printf("Error opening logfile: %s\n", log_file );
      return -1;
    }
    if( write( fd, line, strlen( line ) ) == -1)
      perror("Error loging to logfile");
    close( fd );
  }	// if( log_file[0] != 0 )
  else
  {
    printf("%s",line );		// should this not be "%s",line??
  }
  return 0;
}


/* -----------------------------------------------------------------------
   Log one line of text to the logfile with the current date and time

   Used with temperatures
   ----------------------------------------------------------------------- */
int log_temp( int sensor, float temp_c, unsigned char *sn )
{
  char	temp[1024],
  	time_format[160];
  time_t	mytime;


  mytime = time(NULL);
  if( mytime )
  {
    /* Build the time format string from log_format */
    build_tf( time_format, temp_format, sensor, temp_c, -1, sn );

    /* Handle the time format tokens */
    strftime( temp, 1024, time_format, localtime( &mytime ) );

    strcat( temp, "\n" );
  } else {
    sprintf( temp, "Time Error\n" );
  }
  /* Log it to stdout, logfile or both */
  log_string( temp );

  return 0;
}


/* -----------------------------------------------------------------------
   Log one line of text to the logfile with the current date and time

   Used with counters
   ----------------------------------------------------------------------- */
int log_counter( int sensor, int page, unsigned long counter, unsigned char *sn )
{
  char	temp[1024],
  	time_format[160];
  time_t	mytime;


  mytime = time(NULL);
  if( mytime )
  {
    /* Build the time format string from counter_format */
    build_cf( time_format, counter_format, sensor, page, counter, sn );

    /* Handle the time format tokens */
    strftime( temp, 1024, time_format, localtime( &mytime ) );

    strcat( temp, "\n" );
  } else {
    sprintf( temp, "Time Error\n" );
  }
  /* Log it to stdout, logfile or both */
  log_string( temp );

  return 0;
}


/* -----------------------------------------------------------------------
   Log one line of text to the logfile with the current date and time

   Used with temperatures
   ----------------------------------------------------------------------- */
int log_humidity( int sensor, double temp_c, int humidity, unsigned char *sn )
{
  char	temp[1024],
  	time_format[160];
  time_t	mytime;


  mytime = time(NULL);
  if( mytime )
  {
    /* Log the temperature */
    switch( log_type )
    {
      /* Multiple Centigrade temps per line */
      case 2:     sprintf( temp, "\t%3.2f", temp_c );
                  break;

      /* Multiple Fahrenheit temps per line */
      case 3:     sprintf( temp, "\t%3.2f", c2f(temp_c) );
                  break;

      default:
                  /* Build the time format string from log_format */
                  build_tf( time_format, humidity_format, sensor, temp_c, humidity, sn );

                  /* Handle the time format tokens */
                  strftime( temp, 1024, time_format, localtime( &mytime ) );

                  strcat( temp, "\n" );
                  break;
    }
  } else {
    sprintf( temp, "Time Error\n" );
  }
  /* Log it to stdout, logfile or both */
  log_string( temp );

  return 0;
}


/* -----------------------------------------------------------------------
   Compare two serial numbers and return 1 of they match

   The second one has an additional byte indicating the main (0) or aux (1)
   branch.
   ----------------------------------------------------------------------- */
int cmpSN( unsigned char *sn1, unsigned char *sn2, int branch )
{
  int i;

  for(i = 0; i < 8; i++ )
  {
    if( sn1[i] != sn2[i] )
    {
      return 0;
    }
  }
  if( branch != sn2[8] )
  {
    return 0;
  }

  /* Everything Matches */
  return 1;
}
/*---AddToGlobalRomList()
	Reallocate enough memory to store another ROM
	Store TempSN into the new space
	Record current coupler number / ch
	---*/

int AddNewRomsToGlobalLists(int _couplerNo, int _couplerCh)
{
	unsigned char TempSN[8] = {0,0,0,0,0,0,0,0},
								InfoByte[3];
	short 		result;

	result = owFirst(0, TRUE, FALSE);
	while(result){
		owSerialNum(0, TempSN, TRUE);
		if(!DRJ_IsRomOnList(global_roms_list, &global_roms_count, TempSN)) {
			// Print the serial number
			printSN(TempSN, 0);
			printf(" : %s\n", device_name( TempSN[0]) );
			// Add rom to global_roms_list so we can test if we've seen it before
			AddToGlobalRomList(TempSN, _couplerNo, _couplerCh);

			if(TempSN[0] == SWITCH_FAMILY) {		//is this a DS2409???
				// Save the Coupler's serial number so we can explore it later
				AddToGlobalCouplerList(TempSN, _couplerNo, _couplerCh);
				// Set the coupler type to HobbyBoard4Ch
				global_coupler_list[global_coupler_count - 1].romType = COUPLER_DS2409;

				// Select the coupler so we can make sure it's off
				owSerialNum( 0, &global_coupler_list[global_coupler_count - 1].romSN[0], TRUE );
				// Turn off the Coupler
				if(!SetSwitch1F(0, TempSN, ALL_LINES_OFF, 0, InfoByte, TRUE)){
					fprintf(stderr, "Setting Switch to OFF state failed\n");
					//if( global_coupler_list.roms != NULL ) free( global_coupler_list.roms );
					return -1;
				}
			} else if( TempSN[0] == HOBBYBOARD_EF_FAMILY ) {
				// this may be a Hobby Boards Hub.  Lets see!
				if(IsHobbyBoard4chHub(0,TempSN)) {
					// Save the Coupler's serial number so we can explore it later
					AddToGlobalCouplerList(TempSN, _couplerNo, _couplerCh);
					// Set the coupler type to HobbyBoard4Ch
					global_coupler_list[global_coupler_count - 1].romType = COUPLER_HB4CH;

					// Turn off the Coupler
					owSerialNum( 0, &global_coupler_list[global_coupler_count - 1].romSN[0], TRUE );
					FamilyEF_SetChannels(0,0);
					// DRJ ToDo Check to see if all channels are indeed off
					// Return -1 if they are not
					// free up memory???
				}
			} else if (IsTempSensorOrPIO(TempSN)){
				// add the sensor to the global sensor list
				AddToGlobalSensorList(TempSN, _couplerNo, _couplerCh);
			}
		}
		result = owNext(0, TRUE, FALSE);
	}
	return 0;
}

int AddToGlobalRomList(unsigned char *TempSN, int _couplerNo, int _couplerCh)
{
	global_roms_count++;

	// Allocate enough space for the new serial number
	if(global_roms_count == 1){
		if((global_roms_list = malloc(global_roms_count * sizeof(Rom))) == NULL) {
			fprintf( stderr,"Failed to allocate %lu bytes for global_roms_list\n", global_roms_count * sizeof(Rom) );
			if(global_roms_list != NULL) free(global_roms_list);
			return -1;
		} else {
			// do nothing
		}

	} else {
		if((global_roms_list = realloc(global_roms_list, global_roms_count * sizeof(Rom))) == NULL) {
			fprintf( stderr,"Failed to allocate %lu bytes for global_roms_list\n", global_roms_count * sizeof(Rom) );
			if(global_roms_list != NULL) free(global_roms_list);
			return -1;
		} else {
			// do nothing
		}
	}

	// copy the new serial number to the new rom location
	sercpy(global_roms_list[global_roms_count - 1].romSN,TempSN);
	global_roms_list[global_roms_count - 1].couplerNo = _couplerNo;
	global_roms_list[global_roms_count - 1].couplerCh = _couplerCh;
	global_roms_list[global_roms_count - 1].romType = 0;

	return 0;
}

int AddToGlobalSensorList(unsigned char *TempSN, int _couplerNo, int _couplerCh)
{
	global_sensor_count++;

	// Allocate enough space for the new serial number
	if(global_sensor_count == 1){		// This is the first rom
		if((global_sensor_list = malloc(global_sensor_count * sizeof(Rom))) == NULL) {
			fprintf( stderr,"Failed to allocate %lu bytes for global_sensor_list\n", global_roms_count * sizeof(Rom) );
			if(global_sensor_list != NULL) free(global_sensor_list);
			return -1;
		} else {
		}

	} else {
		if((global_sensor_list = realloc(global_sensor_list, global_sensor_count * sizeof(Rom))) == NULL) {
			fprintf( stderr,"Failed to allocate %lu bytes for global_sensor_list\n", global_sensor_count * sizeof(Rom) );
			if(global_sensor_list != NULL) free(global_sensor_list);
			return -1;
		} else {
		}
	}

	// copy the new serial number to the new rom location
	sercpy(global_sensor_list[global_sensor_count - 1].romSN,TempSN);
	global_sensor_list[global_sensor_count - 1].couplerNo = _couplerNo;
	global_sensor_list[global_sensor_count - 1].couplerCh = _couplerCh;
	global_sensor_list[global_sensor_count - 1].romType = 0;

	return 0;
}






int AddToGlobalCouplerList(unsigned char *TempSN, int _couplerNo, int _couplerCh)
{
	global_coupler_count++;

	// Allocate enough space for the new serial number
	if(global_coupler_count == 1){
		if((global_coupler_list = malloc(global_coupler_count * sizeof(Rom))) == NULL) {
			fprintf( stderr,"Failed to allocate %lu bytes for global_coupler_list\n", global_coupler_count * sizeof(Rom) );
			if(global_coupler_list != NULL) free(global_coupler_list);
			return -1;
		} else {
			// do nothing
		}

	} else {
		if((global_coupler_list = realloc(global_coupler_list, global_coupler_count * sizeof(Rom))) == NULL) {
			fprintf( stderr,"Failed to allocate %lu bytes for global_coupler_list\n", global_coupler_count * sizeof(Rom) );
			if(global_coupler_list != NULL) free(global_coupler_list);
			return -1;
		} else {
			// do nothing
		}
	}

	// copy the new serial number to the new rom location
	sercpy(global_coupler_list[global_coupler_count - 1].romSN,TempSN);
	global_coupler_list[global_coupler_count - 1].couplerNo = _couplerNo;
	global_coupler_list[global_coupler_count - 1].couplerCh = _couplerCh;

	return 0;
}






/*---TurnOffHubAndChannel()
	Turns off the Hub/Channel that is specified.
	Checks if Hub is on the main LAN and recursive calls in order to account for nested hubs
	---*/

int TurnOffHubAndChannel(int hubToTurnOff, int chToTurnOff)
{
	if(hubToTurnOff == 0) { // you are asking me to turn off the main lan???
		return 0;
	}


	if(global_coupler_list[hubToTurnOff].romType == COUPLER_HB4CH) {	// This is a HobbyBoard 4ch Hub
		// turn off chToTurnOff of HobbyBoard4Ch
		owSerialNum(0,&global_coupler_list[hubToTurnOff].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
		FamilyEF_SetChannels(0, 0); // turn off all channels
	}

	if(global_coupler_list[hubToTurnOff].romType == COUPLER_DS2409) {	// This is a DS2409 Hub
		// turn on chToTurnOff of DS2409
		DS2409_SetChannels(0, hubToTurnOff, chToTurnOff); // turn on channel 1 of Hub hubToTurnOff
	}

	if(global_coupler_list[hubToTurnOff].couplerNo != 0) { // this hub is not on the main Lan, so we have more hubs to turn on to get to this hub
		TurnOffHubAndChannel(global_coupler_list[hubToTurnOff].couplerNo, global_coupler_list[hubToTurnOff].couplerCh);
	}



	return 0;
}




/*---TurnOnHubAndChannel()
	Turns on the Hub/Channel that is specified.
	Checks if Hub is on the main LAN and recursive calls in order to account for nested hubs

	DRJ ToDo: Add support for DS2409
	---*/

int TurnOnHubAndChannel(int hubToTurnOn, int chToTurnOn)
{
	if(hubToTurnOn == 0) { // you are asking me to turn on the main lan??? we are already there!
		return 0;
	}

	if(global_coupler_list[hubToTurnOn].couplerNo != 0) { // this hub is not on the main Lan, so we have more hubs to turn on to get to this hub
		TurnOnHubAndChannel(global_coupler_list[hubToTurnOn].couplerNo, global_coupler_list[hubToTurnOn].couplerCh);
	}

	if(global_coupler_list[hubToTurnOn].romType == COUPLER_HB4CH) {	// This is a HobbyBoard 4ch Hub
		// turn on chToTurnOn of HobbyBoard4Ch
		owSerialNum(0,&global_coupler_list[hubToTurnOn].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
		FamilyEF_SetChannels(0, chToTurnOn); // turn on channel chToTurnOn
	}

	if(global_coupler_list[hubToTurnOn].romType == COUPLER_DS2409) {	// This is a DS2409 Hub
		// turn on chToTurnOn of DS2409
		DS2409_SetChannels(0, hubToTurnOn, chToTurnOn); // turn on channel 1 of Hub hubToTurnOn
	}


	return 0;
}


/* -----------------------------------------------------------------------
   Show the verbose contents of the scratchpad
   ----------------------------------------------------------------------- */
void show_scratchpad( unsigned char *scratchpad, int sensor_family )
{
  char temp[80];
  int i;

  if( log_file[0] != 0 )
  {
    switch( sensor_family )
    {
      case DS1820_FAMILY:
        sprintf( temp, "  Temperature   : 0x%02X\n", scratchpad[1] );
        sprintf( temp, "  Sign          : 0x%02X\n", scratchpad[2] );
        sprintf( temp, "  TH            : 0x%02X\n", scratchpad[3] );
        sprintf( temp, "  TL            : 0x%02X\n", scratchpad[4] );
        sprintf( temp, "  Remain        : 0x%02X\n", scratchpad[7] );
        sprintf( temp, "  Count Per C   : 0x%02X\n", scratchpad[8] );
        sprintf( temp, "  CRC           : 0x%02X\n", scratchpad[9] );
        break;

      case DS18B20_FAMILY:
      case DS1822_FAMILY:
      case DS28EA00_FAMILY:
        sprintf( temp, "  Temp. LSB     : 0x%02X\n", scratchpad[1] );
        sprintf( temp, "  Temp. MSB     : 0x%02X\n", scratchpad[2] );
        sprintf( temp, "  TH            : 0x%02X\n", scratchpad[3] );
        sprintf( temp, "  TL            : 0x%02X\n", scratchpad[4] );
        sprintf( temp, "  Config Reg.   : 0x%02X\n", scratchpad[5] );
        sprintf( temp, "  CRC           : 0x%02X\n", scratchpad[9] );
        break;

      case DS2422_FAMILY:
      case DS2423_FAMILY:

        break;
    } /* sensor_family switch */
  } else {
    switch( sensor_family )
    {
      case DS1820_FAMILY:
        printf("  Temperature   : 0x%02X\n", scratchpad[1] );
        printf("  Sign          : 0x%02X\n", scratchpad[2] );
        printf("  TH            : 0x%02X\n", scratchpad[3] );
        printf("  TL            : 0x%02X\n", scratchpad[4] );
        printf("  Remain        : 0x%02X\n", scratchpad[7] );
        printf("  Count Per C   : 0x%02X\n", scratchpad[8] );
        printf("  CRC           : 0x%02X\n", scratchpad[9] );
        break;

      case DS18B20_FAMILY:
      case DS1822_FAMILY:
      case DS28EA00_FAMILY:
        printf( "  Temp. LSB     : 0x%02X\n", scratchpad[1] );
        printf( "  Temp. MSB     : 0x%02X\n", scratchpad[2] );
        printf( "  TH            : 0x%02X\n", scratchpad[3] );
        printf( "  TL            : 0x%02X\n", scratchpad[4] );
        printf( "  Config Reg.   : 0x%02X\n", scratchpad[5] );
        printf( "  CRC           : 0x%02X\n", scratchpad[9] );
        break;

      case DS2422_FAMILY:
      case DS2423_FAMILY:

        break;
    } /* sensor_family switch */
  } /* if log_file */

  /* Dump the complete contents of the scratchpad */
  for( i = 0; i < 10; i++ )
  {
    printf( "scratchpad[%d] = 0x%02X\n", i, scratchpad[i] );
  }
}



/* -----------------------------------------------------------------------
   Read the temperature from one sensor

   Return the high-precision temperature value

   Calculated using formula from DS1820 datasheet

   Temperature   = scratchpad[1]
   Sign          = scratchpad[2]
   TH            = scratchpad[3]
   TL            = scratchpad[4]
   Count Remain  = scratchpad[7]
   Count Per C   = scratchpad[8]
   CRC           = scratchpad[9]

                   count_per_C - count_remain
   (temp - 0.25) * --------------------------
                       count_per_C

   If Sign is not 0x00 then it is a negative (Centigrade) number, and
   the temperature must be subtracted from 0x100 and multiplied by -1
   ----------------------------------------------------------------------- */
int read_temperature( int sensor_family, int sensor )
{
  char    temp[1024];              /* For output string                    */
  unsigned char lastcrc8,
                scratchpad[30],    /* Scratchpad block from the sensor     */
                TempSN[8];
  int     j,
          try,                     /* Number of tries at reading device    */
//          strong_err,              /* Error with strong pullup?            */
          ds1820_try,              /* Allow ds1820 glitch 1 time           */
          ds18s20_try;             /* Allow DS18S20 error 1 time           */
  float   temp_c,                  /* Calculated temperature in Centigrade */
          hi_precision;

  ds1820_try = 0;
  ds18s20_try = 0;
  temp_c = 0;

  for( try = 0; try < MAX_READ_TRIES; try++ )
  {
    if( DrjOwAccess(0) )
    {
      /* Convert Temperature */
      if( !owWriteBytePower( 0, 0x44 ) )
      {
        return FALSE;
      }

      /* Sleep for conversion second */
      msDelay( read_time );

      /* Turn off the strong pullup */
      if( owLevel( 0, MODE_NORMAL ) != MODE_NORMAL )
      {
//        strong_err = 2;
				fprintf(stderr, "WARNING: sfsdfsd(): Turn off the strong pullup not yet implemented\n");
      }


      /* Now read the scratchpad from the device */
      if( DrjOwAccess(0) )
      {
/* Use Read_Scratchpad instead? */
        /* Build a block for the Scratchpad read */
        scratchpad[0] = 0xBE;
        for( j = 1; j < 10; j++ )
          scratchpad[j] = 0xFF;

        /* Send the block */
        if( owBlock( 0, FALSE, scratchpad, 10 ) )
        {
          /* Calculate the CRC 8 checksum on the received data */
          setcrc8(0, 0);
          for( j = 1; j < 10; j++ )
            lastcrc8 = docrc8( 0, scratchpad[j] );

          /* If the CRC8 is valid then calculate the temperature */
          if( lastcrc8 == 0x00 )
          {
            /* DS1822 and DS18B20 use a different calculation */
            if( (sensor_family == DS18B20_FAMILY) ||
                (sensor_family == DS1822_FAMILY) ||
                (sensor_family == DS28EA00_FAMILY) ||
                (sensor_family == DS1923_FAMILY) )
            {
              short int temp2 = (scratchpad[2] << 8) | scratchpad[1];
              temp_c = temp2 / 16.0;
            }

            /* Handle the DS1820 and DS18S20 */
            if( sensor_family == DS1820_FAMILY )
            {
              /* Check for DS1820 glitch condition */
              /* COUNT_PER_C - COUNT_REMAIN == 1 */
              if( ds1820_try == 0 )
              {
                if( (scratchpad[7] - scratchpad[6]) == 1 )
                {
                  ds1820_try = 1;
                  continue;
                } /* DS1820 error */
              } /* ds1820_try */

              /* Check for DS18S20 Error condition */
              /*  LSB = 0xAA
                  MSB = 0x00
                  COUNT_REMAIN = 0x0C
                  COUNT_PER_C = 0x10
              */
              if( ds18s20_try == 0 )
              {
                if( (scratchpad[4]==0xAA) &&
                    (scratchpad[3]==0x00) &&
                    (scratchpad[7]==0x0C) &&
                    (scratchpad[8]==0x10)
                  )
                {
                  ds18s20_try = 1;
                  continue;
                } /* DS18S20 error condition */
              } /* ds18s20_try */

              /* Convert data to temperature */
              if( scratchpad[2] == 0 )
              {
                temp_c = (int) scratchpad[1] >> 1;
              } else {
                temp_c = -1 * (int) (0x100-scratchpad[1]) >> 1;
              } /* Negative temp calculation */
              temp_c -= 0.25;
              hi_precision = (int) scratchpad[8] - (int) scratchpad[7];
              hi_precision = hi_precision / (int) scratchpad[8];
              temp_c = temp_c + hi_precision;
            } /* DS1820_FAMILY */

            /* Log the temperature */
            switch( log_type )
            {
              /* Multiple Centigrade temps per line */
              case 2:     sprintf( temp, "\t%3.2f", temp_c );
                          log_string( temp );
                          break;

              /* Multiple Fahrenheit temps per line */
              case 3:     sprintf( temp, "\t%3.2f", c2f(temp_c) );
                          log_string( temp );
                          break;

              default:    owSerialNum( 0, &TempSN[0], TRUE );
                          log_temp( sensor, temp_c, TempSN );
                          break;
            } /* switch( log_type ) */

            /* Show the scratchpad if verbose is seelcted */
            if( opts & OPT_VERBOSE )
            {
              show_scratchpad( scratchpad, sensor_family );
            } /* if OPT_VERBOSE */

            /* Good conversion finished */
            return TRUE;
          } else {
            fprintf( stderr, "ERROR: digitemp.c read_temperature() CRC Failed. CRC is %02X instead of 0x00\n", lastcrc8 );

            if (try == MAX_READ_TRIES - 1)
            {
              /* need to output something (0,-,NaN?) to keep columns consistent */
              switch( log_type )
              {
            	/* Multiple Centigrade temps per line */
                case 2:
                 /* Multiple Fahrenheit temps per line */
                 case 3:     sprintf( temp, "\t%3.2f", (double) 0 );
                             log_string( temp );
                             break;

                 default:
                             break;
               } /* switch( log_type ) */
            } /* if tries == max_read_tries */

            if( opts & OPT_VERBOSE )
            {
              show_scratchpad( scratchpad, sensor_family );
            } /* if OPT_VERBOSE */
          } /* CRC 8 is OK */
        } /* Scratchpad Read */
      } /* DrjOwAccess failed */
    } /* DrjOwAccess failed */

    /* Failed to read, rest the network, delay and try again */
    owTouchReset(0);
    msDelay( read_time );
  } /* for try < 3 */

  /* Failed, no good reads after MAX_READ_TRIES */
  return FALSE;
}


/* -----------------------------------------------------------------------
   Read the current counter values
   ----------------------------------------------------------------------- */
int read_counter( int sensor_family, int sensor )
{
  char          temp[1024];        /* For output string                    */
  unsigned char TempSN[8];
  int           page;
  unsigned long counter_value;

  if( sensor_family == DS2422_FAMILY )
  {
    /* Read Pages 2, 3 */
    for( page=2; page<=3; page++ )
    {
      if( ReadCounter( 0, page, &counter_value ) )
      {
        /* Log the counter */
        switch( log_type )
        {
          /* Multiple Centigrade temps per line */
          case 2:
          case 3:     sprintf( temp, "\t%ld", counter_value );
                      log_string( temp );
                      break;

          default:    owSerialNum( 0, &TempSN[0], TRUE );
                      log_counter( sensor, page-2, counter_value, TempSN );
                      break;
        } /* switch( log_type ) */
      }
    }
  } else if( sensor_family == DS2423_FAMILY ) {
    /* Read Pages 14, 15 */
    for( page=14; page<=15; page++ )
    {
      if( ReadCounter( 0, page, &counter_value ) )
      {
        /* Log the counter */
        switch( log_type )
        {
          /* Multiple Centigrade temps per line */
          case 2:
          case 3:     sprintf( temp, "\t%ld", counter_value );
                      log_string( temp );
                      break;

          default:    owSerialNum( 0, &TempSN[0], TRUE );
                      log_counter( sensor, page-14, counter_value, TempSN );
                      break;
        } /* switch( log_type ) */
      }
    }
  }

  return FALSE;
}


/* -----------------------------------------------------------------------
   Read the DS2406
   General Purpose PIO
	by Tomasz R. Surmacz (tsurmacz@ict.pwr.wroc.pl)
   !!!! Not finished !!!!
   Needs an output format string system. Hard-coded for the moment.
   ----------------------------------------------------------------------- */
int read_ds2406( int sensor_family, int sensor )
{
  int		pio;
  char		temp[1024],
  		    time_format[160];
  time_t	mytime;


  if( sensor_family == DS2406_FAMILY )
  {
    /* Read Vdd */
    pio = PIO_Reading(0, 0);

    if (pio==-1) {
	printf(" PIO DS2406 sensor %d CRC failed\n", sensor);
	return FALSE;
    }
    mytime = time(NULL);
    if( mytime )
    {
      /* Log the temperature */
      switch( log_type )
      {
        /* Multiple Centigrade temps per line */
        case 2:     sprintf( temp, "\t%02x,%02x", pio>>8, pio&0xff );
                    break;

        /* Multiple Fahrenheit temps per line */
        case 3:     sprintf( temp, "\t%02x,%02x", pio>>8, pio&0xff);
                    break;

        default:
                    sprintf( time_format, "%%b %%d %%H:%%M:%%S Sensor %d PIO: %02x,%02x, PIO-A: %s%s", sensor, pio>>8, pio&0xff,
			((pio&0x1000)!=0)? // Port A latch: there was a change
				(((pio&0x0400)!=0)?
					"ON"	// and the current state is ON
					:"on")
				:"off",	// the current state is off, no change
			((pio&0x4000)!=0)? // we have 2 ports if bit is 1
				( ((pio&0x2000)!=0)?
					(((pio&0x0800)!=0)? // the latch says 1
						" PIO-B: ON" // and state too
						:" PIO-B: on")
					:" PIO-B: off") // the latch said no
				:
				"")
			;
                    /* Handle the time format tokens */
                    strftime( temp, 1024, time_format, localtime( &mytime ) );
                    strcat( temp, "\n" );
                    break;
      } /* switch( log_type ) */
    } else {
      sprintf( temp, "Time Error\n" );
    }

    /* Log it to stdout, logfile or both */
    log_string( temp );
  }

  return TRUE;
}


/* -----------------------------------------------------------------------
   Read the DS2438
   General Purpose A/D
   VDD
   Temperature
   ...

   !!!! Not finished !!!!
   Needs an output format string system. Hard-coded for the moment.
   ----------------------------------------------------------------------- */
int read_ds2438( int sensor_family, int sensor )
{
  double	temperature;
  float		vdd,
  		ad;
  char		temp[1024],
  		time_format[160];
  time_t	mytime;
  int           cad = 0;


  if( sensor_family == DS2438_FAMILY )
  {
    temperature = Get_Temperature(0);

    /* Read Vdd */
    vdd = Volt_Reading(0, 1, &cad);

    /* Read A/D */
    ad = Volt_Reading(0, 0, NULL);

    mytime = time(NULL);
    if( mytime )
    {
      /* Log the temperature */
      switch( log_type )
      {
        /* Multiple Centigrade temps per line */
        case 2:     sprintf( temp, "\t%3.2f", temperature );
                    break;

        /* Multiple Fahrenheit temps per line */
        case 3:     sprintf( temp, "\t%3.2f", c2f(temperature) );
                    break;

        default:
                    sprintf( time_format, "%%b %%d %%H:%%M:%%S Sensor %d VDD: %0.2f AD: %0.2f CAD: %d C: %0.2f", sensor, vdd, ad, cad, temperature );
                    /* Handle the time format tokens */
                    strftime( temp, 1024, time_format, localtime( &mytime ) );
                    strcat( temp, "\n" );
                    break;
      } /* switch( log_type ) */
    } else {
      sprintf( temp, "Time Error\n" );
    }

    /* Log it to stdout, logfile or both */
    log_string( temp );
  }

  return FALSE;
}


/* -----------------------------------------------------------------------
   (This routine is modified from code by Eric Wilde)

   Read the humidity from one sensor (e.g. the AAG TAI8540x).

   Log the temperature value and relative humidity.

   Calculated using formula cribbed from the Dallas source code (gethumd.c),
   DS2438 data sheet and HIH-3610 data sheet.

   Sensors like the TAI8540x use a DS2438 battery monitor to sense temperature
   and convert humidity readings from a Honeywell HIH-3610.  The DS2438
   scratchpad is:

   Status/config = scratchpad[2]
   Temp LSB      = scratchpad[3]
   Temp MSB      = scratchpad[4]
   Voltage LSB   = scratchpad[5]
   Voltage MSB   = scratchpad[6]
   CRC           = scratchpad[10]

                            Temp LSB
   temp = (Temp MSB * 32) + -------- * 0.03125
                                8

   The temperature is a two's complement signed number.

   voltage = ((Voltage MSB * 256) + Voltage LSB) / 100

   There are two voltages that must be read to get an accurate humidity
   reading.  The supply voltage (VDD) is read to determine what voltage the
   humidity sensor is running at (this affects the zero offset and slope of
   the humidity curve).  The sensor voltage (VAD) is read to get the humidity
   value.  Here is the formula for the humidity (temperature and voltage
   compensated):

              ((VAD/VDD) - 0.16) * 161.29
   humidity = ---------------------------
               1.0546 - (0.00216 * temp)

   The humidity sensor is linear from approx 10% to 100% R.H.  Accuracy is
   approx 2%.

   !!!! Not Finished !!!!
   ----------------------------------------------------------------------- */
int read_humidity( int sensor_family, int sensor )
{
  double	temp_c;			/* Converted temperature in degrees C */
  float		sup_voltage,		/* Supply voltage in volts            */
		hum_voltage,		/* Humidity sensor voltage in volts   */
		humidity;		/* Calculated humidity in %RH         */
  unsigned char	TempSN[8];
  int		try;

  for( try = 0; try < MAX_READ_TRIES; try++ )
  {
    /* Read Vdd, the supply voltage */
    if( (sup_voltage = Volt_Reading(0, 1, NULL)) != -1.0 )
    {
      /* Read A/D reading from the humidity sensor */
      if( (hum_voltage = Volt_Reading(0, 0, NULL)) != -1.0 )
      {
        /* Read the temperature */
        temp_c = Get_Temperature(0);

        /* Convert the measured voltage to humidity */
        humidity = (((hum_voltage/sup_voltage) - 0.16) * 161.29)
                      / (1.0546 - (0.00216 * temp_c));
	if( humidity > 100.0 )
	  humidity = 100.0;
	else if( humidity < 0.0 )
	  humidity = 0.0;

        /* Log the temperature and humidity */
        owSerialNum( 0, &TempSN[0], TRUE );
        log_humidity( sensor, temp_c, humidity, TempSN );

        /* Good conversion finished */
        return TRUE;
      }
    }

    owTouchReset(0);
    msDelay(read_time);
  }

  return FALSE;
}


/* -----------------------------------------------------------------------
   Read the DS1923 Hygrochton Temperature/Humidity Logger
   ----------------------------------------------------------------------- */
int read_temperature_DS1923( int sensor_family, int sensor )
{
  unsigned char TempSN[8],
  		block2[2];
  int try;                     /* Number of tries at reading device    */
  int b;
  int pre_t;
  float temp_c;
  int ival;
  float adval;
  float humidity;

  for( try = 0; try < MAX_READ_TRIES; try++ )
  {
    if( DrjOwAccess(0) )
    {
      /* Force Conversion */
      if( !DrjOwWriteByte( 0, 0x55 ) || !DrjOwWriteByte( 0, 0x55 ))
      {
        return FALSE;
      }
      /* TODO CRC checking and read the addresses 020Ch to 020Fh (results)i
       * and the Device Sample Counter at address 0223h to 0225h.
       * If the count has incremented, the command was executed successfully.
       */

      /* Sleep for conversion (spec says it takes max 666ms */
      /* Q. Is it possible to poll? */
      msDelay( 666 );

      /* Now read the memory 0x20C:0x020F */
      if( DrjOwAccess(0) )
      {
        if( !DrjOwWriteByte( 0, 0x69 ) )
        {
          return FALSE;
        }

        /* "Latest Temp" in the memory */
        block2[0] = 0x0c;
        block2[1] = 0x02;

        /* Send the block */
        if( owBlock( 0, FALSE, block2, 2 ) )
        {
          if (block2[0] != 0x0c && block2[1] != 0x02)
            return FALSE;

          /* Send dummy password */
          for(b = 0; b < 8; ++b) {
            DrjOwWriteByte(0, 0x04);
          }

          /* Read the temperature */
          block2[0] = DrjOwReadByte(0);
          block2[1] = DrjOwReadByte(0);
          pre_t  = (block2[1]/2)-41;
          temp_c = 1.0f * pre_t + block2[0]/512.0f;

          /* Read the humidity */
          block2[0] = DrjOwReadByte(0);
          block2[1] = DrjOwReadByte(0);
          ival = (block2[1]*256 + block2[0])/16;
          adval = 1.0f * ival * 5.02f/4096;
          humidity = (adval-0.958f) / 0.0307f;

          /* Log the temperature and humidity */
	  /* TUTAJ masz wartosci we floatach dla Thermochrona
 	     sensor to nr sensora z pliku konfiguracyjnego,
 	     a tempsn to pewnie id urzadzenia 1wire
          */
          owSerialNum( 0, &TempSN[0], TRUE );
          log_humidity( sensor, temp_c, humidity, TempSN );

          /* Good conversion finished */
          return TRUE;
        } /* Scratchpad Read */
      } /* DrjOwAccess failed */
    } /* DrjOwAccess failed */

    /* Failed to read, rest the network, delay and try again */
    owTouchReset(0);
    msDelay( read_time );
  } /* for try < 3 */

  /* Failed, no good reads after MAX_READ_TRIES */
  return FALSE;
}


/* -----------------------------------------------------------------------
	read_device(Rom *sensor_array, int sensor_num)

	returns 0 if all is well (variable status)
   Select the indicated device, turning on any required couplers
   ----------------------------------------------------------------------- */
int read_device(int sensor_num){
	int							status = 0,
									sensor_family;

	/* Tell the sensor to do a temperature conversion */
	TurnOnHubAndChannel(global_sensor_list[sensor_num].couplerNo, global_sensor_list[sensor_num].couplerCh);

	// address the sensor
	owSerialNum(0, &global_sensor_list[sensor_num].romSN[0], FALSE );

	// Get the Serial # selected
	//	owSerialNum( 0, &TempSN[0], TRUE );
	sensor_family = global_sensor_list[sensor_num].romSN[0];

	switch( sensor_family )
	{
		case DS28EA00_FAMILY:
		case DS2413_FAMILY:
			if( (opts & OPT_DS2438) || (sensor_family==DS2413_FAMILY) ) { // read PIO
		status = read_pio_ds28ea00( sensor_family, sensor_num );
			break;
		}
			// else - drop through to DS1822
		case DS1820_FAMILY:
		case DS1822_FAMILY:
		case DS18B20_FAMILY:
			status = read_temperature( sensor_family, sensor_num ); // also for DS28EA00
			break;

		case DS1923_FAMILY:
			status = read_temperature_DS1923( sensor_family, sensor_num );
			break;

		case DS2422_FAMILY:
		case DS2423_FAMILY:
			status = read_counter( sensor_family, sensor_num );
			break;

	case DS2438_FAMILY:
		// What type is it?
		{
		int page;
		for( page=3; page<8; page++)
		{
			get_ibl_type( 0, page, 0);
				}}
		if( opts & OPT_DS2438 )
		{
			status = read_ds2438( sensor_family, sensor_num );
		} else {
			status = read_humidity( sensor_family, sensor_num );
		}
		break;
	}
	// turn the hub off
//	TurnOnHubAndChannel(global_sensor_list[sensor_num].couplerNo, 0);
	TurnOffHubAndChannel(global_sensor_list[sensor_num].couplerNo, 0);

	return status;
}




/* -----------------------------------------------------------------------
   Read the temperaturess for all the connected sensors

   Step through all the sensors in the list of serial numbers
   ----------------------------------------------------------------------- */
int read_all(){
	int x;
	for(x = 0; x < global_sensor_count; x++){
		read_device(x);
	}
	return 0;
}


/* -----------------------------------------------------------------------
   Read a .digitemprc file from the current directory

   The rc file contains:

   v 4.0 reduex:
   TTY <serial>
   LOG <logfilepath>
   READ_TIME <time in mS>
   LOG_TYPE <from -o>
   LOG_FORMAT <format string for temperature logging and printing>
   CNT_FORMAT <format string for counter logging and printing>
   SENSORS <number of SROM lines>
   COUPLERS <number of CROM lines>
   Multiple Sensor ROM lines
		SROM <SERIAL NUMBER IN BYTES> <TYPE> <COUPLER #> <COUPLER CH>
   Multiple Coupler ROM lines
		CROM <SERIAL NUMBER IN BYTES> <TYPE> <COUPLER #> <COUPLER CH>
   ----------------------------------------------------------------------- */
int read_rcfile(char *fname){
	FILE	*fp;
	char	temp[1024];
	char	*ptr;
	int		sensorNum, x;

	global_roms_count = 0;
	if(global_roms_list != NULL) free(global_roms_list);
	global_sensor_count = 0;
	if(global_sensor_list != NULL) free(global_sensor_list);
	global_coupler_count = 0;
	if(global_coupler_list != NULL) free(global_coupler_list);

	if( ( fp = fopen( fname, "r" ) ) == NULL ){
		/* No rcfile to read, could be part of an -i so don't die */
		return 1;
	}
	while( fgets( temp, 80, fp ) != 0 ){
		if((temp[0] == '\n') || (temp[0] == '#'))
			continue;

		ptr = strtok(temp, " \t\n");
		if( strncasecmp( "TTY", ptr, 3 ) == 0 ){
			ptr = strtok( NULL, " \t\n" );
			strncpy( serial_port, ptr, sizeof(serial_port)-1 );
		} else if( strncasecmp( "LOG_TYPE", ptr, 8 ) == 0 ) {
			ptr = strtok( NULL, " \t\n");
			log_type = atoi( ptr );
		} else if( strncasecmp( "LOG_FORMAT", ptr, 10 ) == 0 ) {
			ptr = strtok( NULL, "\"\n");
			strncpy( temp_format, ptr, sizeof(temp_format)-1 );
		} else if( strncasecmp( "CNT_FORMAT", ptr, 10 ) == 0 ) {
			ptr = strtok( NULL, "\"\n");
			strncpy( counter_format, ptr, sizeof(counter_format)-1 );
		} else if( strncasecmp( "HUM_FORMAT", ptr, 10 ) == 0 ) {
			ptr = strtok( NULL, "\"\n");
			strncpy( humidity_format, ptr, sizeof(humidity_format)-1 );
		} else if( strncasecmp( "LOG", ptr, 3 ) == 0 ) {
			ptr = strtok( NULL, " \t\n" );
			strncpy( log_file, ptr, sizeof(log_file)-1 );
		} else if( strncasecmp( "FAIL_TIME", ptr, 9 ) == 0 ) {

		} else if( strncasecmp( "READ_TIME", ptr, 9 ) == 0 ) {
			ptr = strtok( NULL, " \t\n");
			read_time = atoi( ptr );



		} else if( strncasecmp( "SENSORS", ptr, 7 ) == 0 ) {
			ptr = strtok( NULL, " \t\n" );
			global_sensor_count = atoi( ptr );
			if(global_sensor_count > 0){
				// Reserve memory for the sensor list
				if((global_sensor_list = malloc(sizeof(Rom) * global_sensor_count)) == NULL ){
					fprintf( stderr, "Error reserving memory for %d sensors\n", global_sensor_count);
					return -1;
				}
			}

		} else if( strncasecmp( "COUPLERS", ptr, 8 ) == 0 ) {
			ptr = strtok( NULL, " \t\n" );
			global_coupler_count = atoi(ptr)+1;
			if(global_coupler_count > 0){
				// Reserve memory for the coupler list
				if((global_coupler_list = malloc(sizeof(Rom) * global_coupler_count)) == NULL ){
					fprintf( stderr, "Error reserving memory for %d couplers\n", global_coupler_count);
					return -1;
				}
				// Write info into coupler number 0, the special case of the main LAN
				global_coupler_list[0].romType = 0;
				global_coupler_list[0].couplerNo = 0;
				global_coupler_list[0].couplerCh = 0;
			}

		} else if( strncasecmp( "SROM", ptr, 4 ) == 0 ) { // sensor roms
			// get sensor num
			ptr = strtok( NULL, " \t\n" );
			sensorNum = atoi( ptr );

			// Read the 8 byte ROM address
			for( x = 0; x < 8; x++ ){
				ptr = strtok( NULL, " \t\n" );
				global_sensor_list[sensorNum].romSN[x] = strtol( ptr, (char **)NULL, 0 );
			}
			// set sensor type ... not yet implemented!!!
			ptr = strtok( NULL, " \t\n" );
			global_sensor_list[sensorNum].romType = atoi( ptr );

			// set sensor Hub Number
			ptr = strtok( NULL, " \t\n" );
			global_sensor_list[sensorNum].couplerNo = atoi( ptr );

			// set sensor Hub Channel
			ptr = strtok( NULL, " \t\n" );
			global_sensor_list[sensorNum].couplerCh = atoi( ptr );

		} else if( strncasecmp( "CROM", ptr, 4 ) == 0 ){	// coupler roms
			// get sensor num
			ptr = strtok( NULL, " \t\n" );
			sensorNum = atoi( ptr );

			// Read the 8 byte ROM address
			for( x = 0; x < 8; x++ ){
				ptr = strtok( NULL, " \t\n" );
				global_coupler_list[sensorNum].romSN[x] = strtol( ptr, (char **)NULL, 0 );
			}
			// set coupler type (as of 2013-09-20 it will be 1 or 2)
			ptr = strtok( NULL, " \t\n" );
			global_coupler_list[sensorNum].romType = atoi( ptr );

			// set coupler Hub Number
			ptr = strtok( NULL, " \t\n" );
			global_coupler_list[sensorNum].couplerNo = atoi( ptr );

			// set coupler Hub Channel
			ptr = strtok( NULL, " \t\n" );
			global_coupler_list[sensorNum].couplerCh = atoi( ptr );
		} else {
			fprintf( stderr, "Error reading rcfile: %s\n", fname );
			if(global_sensor_list != NULL) free(global_sensor_list);
			if(global_coupler_list != NULL) free(global_coupler_list);
			fclose( fp );
			return -1;
		}
	}
	fclose( fp );
	return 0;
}


/* -----------------------------------------------------------------------
   Write a .digitemprc file, it contains:

   TTY <serial>
   LOG <logfilepath>
   READ_TIME <time in mS>
   LOG_TYPE <from -o>
   LOG_FORMAT <format string for temperature logging and printing>
   CNT_FORMAT <format string for counter logging and printing>
   SENSORS <number of ROM lines>
   Multiple ROM x <serial number in bytes> lines

   v 2.3 additions:
   Multiple COUPLER x <serial number in decimal> lines
   CROM x <COUPLER #> <M or A> <Serial number in decimal>

   v 2.4 additions:
   All serial numbers are now in Hex.  Still can read older decimal
     format.
   Added 'ALIAS # <string>'

   v 3.6 additions:
   Multiple HUB x <serial number in bytes> lines
   HBROM x <COUPLER #> <1,2,3,or4> <Serial number in bytes>

   v 4.0 reduex:
   TTY <serial>
   LOG <logfilepath>
   READ_TIME <time in mS>
   LOG_TYPE <from -o>
   LOG_FORMAT <format string for temperature logging and printing>
   CNT_FORMAT <format string for counter logging and printing>
   SENSORS <number of SROM lines>
   COUPLERS <number of CROM lines>
   Multiple Sensor ROM lines
		SROM <SERIAL NUMBER IN BYTES> <TYPE> <COUPLER #> <COUPLER CH>
   Multiple Coupler ROM lines
		CROM <SERIAL NUMBER IN BYTES> <TYPE> <COUPLER #> <COUPLER CH>

   ----------------------------------------------------------------------- */
int write_rcfile(){
	FILE	*fp;
	int	x, y;

	if( ( fp = fopen( conf_file, "wb" ) ) == NULL ) {
		return -1;
	}

	fprintf( fp, "TTY %s\n", serial_port );
	if( log_file[0] != 0 )
		fprintf( fp, "LOG %s\n", log_file );

	fprintf( fp, "READ_TIME %d\n", read_time );				// mSeconds
	fprintf( fp, "LOG_TYPE %d\n", log_type );
	fprintf( fp, "LOG_FORMAT \"%s\"\n", temp_format );
	fprintf( fp, "CNT_FORMAT \"%s\"\n", counter_format );
	fprintf( fp, "HUM_FORMAT \"%s\"\n", humidity_format );

	fprintf( fp, "SENSORS %d\n", global_sensor_count);
	fprintf( fp, "COUPLERS %d\n", global_coupler_count-1);

	// write out all sensor rom lines
	// format is something like
	//	SROM 0 0x10 0x7B 0x80 0xB5 0x01 0x08 0x00 0x9D 1 2 0
	for(x = 0; x < global_sensor_count; x++){
		fprintf( fp, "SROM %d", x);
		for(y = 0; y < 8; y++){
			fprintf(fp, " 0x%02X", global_sensor_list[x].romSN[y]);
		}
		fprintf( fp, " %d %d %d", global_sensor_list[x].romType, global_sensor_list[x].couplerNo, global_sensor_list[x].couplerCh);
		fprintf(fp, "\n" );
	}

	// write out all coupler rom lines
	for(x = 1; x < global_coupler_count; x++){
		fprintf( fp, "CROM %d", x);
		for(y = 0; y < 8; y++){
			fprintf(fp, " 0x%02X", global_coupler_list[x].romSN[y]);
		}
		fprintf( fp, " %d %d %d", global_coupler_list[x].romType, global_coupler_list[x].couplerNo, global_coupler_list[x].couplerCh);
		fprintf(fp, "\n" );
	}



	fclose( fp );
	if( !(opts & OPT_QUIET) )
	{
		fprintf( stderr, "Wrote %s\n",conf_file);
	}

	return 0;
}


/* -----------------------------------------------------------------------
   Print out a serial number
   ----------------------------------------------------------------------- */
void printSN( unsigned char *TempSN, int crlf ){
  int y;

  /* Print the serial number */
  for(y = 0; y < 8; y++){
    printf("%02X", TempSN[y] );
  }
  if(crlf)
    printf("\n");
}


/* -----------------------------------------------------------------------
   Walk the entire connected 1-wire LAN and display the serial number
   and type of device.
   ----------------------------------------------------------------------- */

  // should we concern ourselves with "Nested" couplers?
	//	The roms should be kept track of (including couplers)
	//	Then as the channels of the couplers are turned on, additional roms found will assumed
	//	to be on that branch of the tree... a little sloppy...
	// The following should be global variables:
	//	roms_list
	//	roms_count
	//	coupler_list
	//	coupler_count
	// That way functions such as AddRomToList could be easily called.
	// Coupler zero is the special case of the main LAN

int Walk1Wire() {
	unsigned char TempSN[8] = {0,0,0,0,0,0,0,0};
	int	currentHubNo;
	int	currentHubCh;
	int   x;

	FindCouplersAndTurnThemOff(0,0,!(opts & OPT_QUIET));

	// initialize global variables
	global_roms_count = 0;
	if(global_roms_list != NULL) free(global_roms_list);
	global_sensor_count = 0;
	if(global_sensor_list != NULL) free(global_sensor_list);
	global_coupler_count = 0;
	if(global_coupler_list != NULL) free(global_coupler_list);

	// initialze local variables
	currentHubNo	= 0;
	currentHubCh	=	0;


	AddToGlobalCouplerList(TempSN, currentHubNo, currentHubCh);
	// Set the coupler type to zero for the main LAN
	global_coupler_list[global_coupler_count - 1].romType = 0;

	// we know all the couplers on the main LAN are off, we
	// can now start mapping the 1-Wire LAN
	if(!(opts & OPT_QUIET)){
		printf("Devices on the Main LAN\n");
	}

	AddNewRomsToGlobalLists(currentHubNo, currentHubCh);


	// If there were any couplers/hubs we need to walk their trees as well.
	// should we concern ourselves with "Nested" couplers?
	// if we included a TurnOnHubAndChannel() befor each hub we walk
	// we would be well on our way to handling "Nested" couplers!
	if(global_coupler_count > 1){
		for(x = 1; x < global_coupler_count; x++){
			currentHubNo = x;
			TurnOnHubAndChannel(global_coupler_list[x].couplerNo, global_coupler_list[x].couplerCh);
			if(global_coupler_list[x].romType == COUPLER_DS2409) {	// This is a DS2409 Hub
				if(!(opts & OPT_QUIET)){
					printf("\nHub type DS2409 : ");
					printf("\nDevices on Main Branch of Coupler : ");
					printSN( &global_coupler_list[x].romSN[0], 1 );
				}
				// turn on ch1 of DS2409
				DS2409_SetChannels(0, x, 1); // turn on channel 1 of Hub x
				currentHubCh = 1;
				AddNewRomsToGlobalLists(currentHubNo, currentHubCh);
				DS2409_SetChannels(0, x, 0); // turn off Hub x
				if(!(opts & OPT_QUIET)){
					printf("\nHub type DS2409 : ");
					printf("\nDevices on Aux Branch of Coupler : ");
					printSN( &global_coupler_list[x].romSN[0], 1 );
				}
				// turn on ch2 of DS2409
				DS2409_SetChannels(0, x, 2); // turn on channel 2 of Hub x
				currentHubCh = 2;
				AddNewRomsToGlobalLists(currentHubNo, currentHubCh);
				DS2409_SetChannels(0, x, 0); // turn off Hub x
			} else { // DS2409 Hub
				// do nothing
			}

			if(global_coupler_list[x].romType == COUPLER_HB4CH) {	// This is a HobbyBoard 4ch Hub
				// Channel 1
				if(!(opts & OPT_QUIET)){
					printf("\nHub type HobbyBoard4Ch : ");
					printf("\nDevices on Ch1 of Hub : ");
					printSN(&global_coupler_list[currentHubNo].romSN[0], 1);
				}
				// turn on ch1 of HobbyBoard4Ch
				owSerialNum(0,&global_coupler_list[currentHubNo].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				currentHubCh = 1;
				FamilyEF_SetChannels(0, currentHubCh); // turn on channel 1
				AddNewRomsToGlobalLists(currentHubNo, currentHubCh);
				owSerialNum(0,&global_coupler_list[currentHubNo].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				FamilyEF_SetChannels(0, 0);

				// Channel 2
				if(!(opts & OPT_QUIET)){
					printf("\nDevices on Ch2 of Hub : ");
					printSN(&global_coupler_list[x].romSN[0], 1);
				}
				// turn on ch2 of HobbyBoard4Ch
				owSerialNum(0,&global_coupler_list[x].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				currentHubCh = 2;
				FamilyEF_SetChannels(0, currentHubCh); // turn on channel 2
				AddNewRomsToGlobalLists(currentHubNo, currentHubCh);
				owSerialNum(0,&global_coupler_list[currentHubNo].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				FamilyEF_SetChannels(0, 0);


				// Channel 3
				if(!(opts & OPT_QUIET)){
					printf("\nDevices on Ch3 of Hub : ");
					printSN(&global_coupler_list[x].romSN[0], 1);
				}
				// turn on ch3 of HobbyBoard4Ch
				owSerialNum(0,&global_coupler_list[x].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				currentHubCh = 3;
				FamilyEF_SetChannels(0, currentHubCh); // turn on channel 3
				AddNewRomsToGlobalLists(currentHubNo, currentHubCh);
				owSerialNum(0,&global_coupler_list[currentHubNo].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				FamilyEF_SetChannels(0, 0);

				// Channel 4
				if(!(opts & OPT_QUIET)){
					printf("\nDevices on Ch4 of Hub : ");
					printSN(&global_coupler_list[x].romSN[0], 1);
				}
				// turn on ch4 of HobbyBoard4Ch
				owSerialNum(0,&global_coupler_list[x].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				currentHubCh = 4;
				FamilyEF_SetChannels(0, currentHubCh); // turn on channel 4
				AddNewRomsToGlobalLists(currentHubNo, currentHubCh);
				owSerialNum(0,&global_coupler_list[currentHubNo].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				FamilyEF_SetChannels(0, 0);

				// turn off ch1,2,3,4 of HobbyBoard4Ch
				owSerialNum(0,&global_coupler_list[x].romSN[0],FALSE);	// Set the current device to our HobbyBoard coupler
				FamilyEF_SetChannels(0, 0);
			} // Hobby Board 4ch Hub
		}  /* Coupler loop */
	} /* num_couplers check */

	return 0;
} // Walk1Wire


/* -----------------------------------------------------------------------
   Find all the supported temperature sensors on the bus, searching down
   DS2409 hubs on the main bus (but not on other hubs).
   2013-07-27: Added HobbyBoards 4ch Hub
   numDS_sensors				is the number of DS2409 coupler censors
   couplerDS_top	is a linked list of couplers (hubs)
   sensor_list	is a linked list of sensors

	2013-09-20 Walk1Wire has been re-written to populate global variables
		with sensor and hub information.
		Rom*			global_sensor_list;			// Attached Sensors
		short			global_sensor_count;		// number of sensors in sensor_list

		Rom*			global_coupler_list;		// Attached Roms that are couplers needing further investigation
		short			global_coupler_count;		// number of couplers in coupler_list

	  struct Rom includes
				unsigned char   romSN[8];					// SN of rom
			  unsigned int		romType;					// Either COUPLER_DS2409, COUPLER_HB4CH, SENSOR_DSXXXX -Maybe this should be family code?
				int							couplerNo;				// if 0 then device is on the main lan, 1 = device is on Coupler[1]
				int							couplerCh;				// Hub ch
		couplerNo and couplerCh indicate what channels need to be open to "see" the Rom on the 1wire network.
		Walk1Wire can be re-written so that nested couplers are accounted for.  As it is, they will be identified and added to the list
		of couplers, but they will not be "walked"

   ----------------------------------------------------------------------- */
int Init1WireLan() {
	int x;
	if(!(opts & OPT_QUIET)){
		printf("Calling Walk1Wire to find Roms and add them to the sensor and coupler lists\n");
	}
	Walk1Wire();

	// Did the search find any sensors? Even if there was an error it may
	// have found some valid sensors
	if(global_sensor_count > 0){
		if(!(opts & OPT_QUIET)){
			printf("Found %i sensors.\n",global_sensor_count);
		}
		// Print all sensors
		for(x = 0; x < global_sensor_count; x++){
			printf("ROM #%d : Hub=%d Ch=%d Type=%d SN=", x, global_sensor_list[x].couplerNo, global_sensor_list[x].couplerCh, global_sensor_list[x].romType);
			printSN(global_sensor_list[x].romSN, 1);
		}
	} else {
		if(!(opts & OPT_QUIET)){
			printf("Did not find any sensors.\n");
		}
	}

	if(global_coupler_count > 0){
		if(!(opts & OPT_QUIET)){
			printf("Found %i couplers.\n",global_coupler_count-1); // coupler number 0 is the main LAN
		}
		// Print all sensors
		for(x = 1; x < global_coupler_count; x++){
			printf("ROM #%d : Hub=%d Ch=%d Type=%d SN=", x, global_coupler_list[x].couplerNo, global_coupler_list[x].couplerCh, global_coupler_list[x].romType);
			printSN(global_coupler_list[x].romSN, 1);
		}
	} else {
		if(!(opts & OPT_QUIET)){
			printf("Did not find any couplers.\n");
		}
	}

	// Write the new list of sensors to the current directory
	write_rcfile();
	return 0;
}


/* ----------------------------------------------------------------------- *
   Check to see if the file actually exists
 * ----------------------------------------------------------------------- */
int file_exists (char * fileName)
{
   struct stat buf;

   if (stat( fileName, &buf ) == 0)
   {
       /* file found */
       return 1;
   }
   return 0;
}


/* ----------------------------------------------------------------------- *
   DigiTemp main routine

   Parse command line options, run functions
 * ----------------------------------------------------------------------- */
int main(int argc, char *argv[]){
	int	sensor;			// Single sensor index to read
	char	temp[1024];		// Temporary strings
	int	c;
	int	sample_delay = 0;	// Delay between samples (SEC)
	unsigned int	x,
						num_samples = 1;		// Number of samples
	time_t	last_time,	// Last time we started samples
				start_time; // Starting time
	long int	elapsed_time;	// Elapsed from start

	Rom*	roms;			// array of roms
	Rom*	couplers;	// array of couplers


	if(argc <= 1){
		fprintf(stderr,"Error! Not enough arguements.\n\n");
		usage();
		return -1;
	}

	roms = NULL;
	couplers = NULL;


#ifndef OWUSB
	serial_port[0] = 0;				// No default port
#else
	strcpy(serial_port, "USB");
#endif
	tmp_serial_port[0] = 0;
	log_file[0] = 0;					// No default log file
	tmp_log_file[0] = 0;
	tmp_counter_format[0] = 0;
	tmp_temp_format[0] = 0;
	tmp_humidity_format[0] = 0;
	read_time = 1000;					// 1000mS read delay
	tmp_read_time = -1;
	temp_LAN_activity_window_us = -1;
	sensor = 0;							// First sensor in list
	log_type = 1;						// Normal DigiTemp logfile
	tmp_log_type = -1;
	sample_delay = 0;					// No delay
	num_samples = 1;					// Only do it once by default

	// Default log format string:
	// May 24 21:25:43 Sensor 0 C: 23.66 F: 74.59
	strcpy( temp_format, "%b %d %H:%M:%S Sensor %s C: %.2C F: %.2F" );
	strcpy( counter_format, "%b %d %H:%M:%S Sensor %s #%n %C" );
	strcpy( humidity_format, "%b %d %H:%M:%S Sensor %s C: %.2C F: %.2F H: %h%%" );
	strcpy( conf_file, ".digitemprc" );
	strcpy( option_list, "?ThqiaAvwr:f:s:l:t:d:n:o:p:c:O:H:" );


	TimeSetToNow(&LAN_activity_last_time);	// initialize the time our last LAN activity

	// Command line options override any .digitemprc options temporarily
	// Unless the -i parameter is specified, then changes are saved to
	// .digitemprc file
	optind = OPTINDSTART;
	opterr = 1;

	while( (c = getopt(argc, argv, option_list)) != GETOPTEOF ){
		// Process the command line arguments
		switch( c ) {
			case 'c':			// Configuration file
				if( optarg ) {
					strncpy( conf_file, optarg, sizeof( conf_file ) - 1 );
				}
				break;

			case 'T':
				opts |= OPT_TEST;
				break;

			case 'w':			// Walk the LAN
				opts |= OPT_WALK;
				break;

			case 'i':			// Initalize the s#'s
				opts |= OPT_INIT;
				break;

			case 'r':			// Read delay in mS
				tmp_read_time = atoi(optarg);
				break;
			case 'p':			// lan call window in uS
				temp_LAN_activity_window_us = atoi(optarg);
				break;
			case 'v':			// Verbose
				opts |= OPT_VERBOSE;
				break;

			case 's':			// Serial port
				if(optarg)
					{
					strncpy( tmp_serial_port, optarg, sizeof(tmp_serial_port) - 1 );
				}
				break;

			case 'l':			// Log Filename
				if(optarg) {
					strncpy( tmp_log_file, optarg, sizeof( tmp_log_file ) - 1);
				}
				break;

			case 't':			// Single Sensor #
				if(optarg) {
					sensor = atoi(optarg);
					opts |= OPT_SINGLE;
				}
				break;

			case 'a': 			// Read All sensors
				opts |= OPT_ALL;
				break;

			case 'd':			// Sample Delay
				if(optarg) {
					sample_delay = atoi(optarg);
				}
				break;

			case 'n':			// Number of samples
				if(optarg) {
					num_samples = atoi(optarg);
				}
				break;

			case 'A':			// Treat DS2438 as A/D converter
				opts |= OPT_DS2438;
				break;

			case 'o':			// Temperature Logfile format
				if(optarg) {
					if( isdigit( optarg[0] ) ) {
						// Its a number, get it
						tmp_log_type = atoi(optarg);
					} else {
						// Not a number, get the string
						if( strlen( optarg ) > sizeof(tmp_temp_format)-1 )
							printf("Temperature format string too long! > %d\n", (int) sizeof(tmp_temp_format)-1);
						else
							strncpy( tmp_temp_format, optarg, sizeof(tmp_temp_format)-1 );
						tmp_log_type=0;
					}
				}
			break;

			case 'O':			// Counter Logfile format
				if(optarg) {
					if( strlen( optarg ) > sizeof(tmp_counter_format)-1 )
						printf("Counter format string too long! > %d\n", (int) sizeof(tmp_counter_format)-1);
					else
						strncpy( tmp_counter_format, optarg, sizeof(tmp_counter_format)-1 );
				}
				break;

			case 'H':			// Humidity Logfile format
				if(optarg) {
					if( strlen( optarg ) > sizeof(tmp_humidity_format)-1 )
						printf("Humidity format string too long! > %d\n", (int) sizeof(tmp_humidity_format)-1);
					else
						strncpy( tmp_humidity_format, optarg, sizeof(tmp_humidity_format)-1 );
				}
				break;

			case 'q':
				opts |= OPT_QUIET;
				break;

			case ':':
			case 'h':
			case '?':
				usage();
				exit(EXIT_HELP);
				break;
			default:	break;
		} // switch getopt
	}  // while getopt

	/* Require one 1 action command, no more, no less. */
	if ((opts & (OPT_WALK|OPT_INIT|OPT_SINGLE|OPT_ALL)) == 0 )
	{
		fprintf( stderr, "Error!  You need 1 of the following action commands, -w -a -i -t\n");
		exit(EXIT_HELP);
	}

	if ( read_rcfile(conf_file) < 0 ) {
		exit(EXIT_NORC);
	}



	if (temp_LAN_activity_window_us > 0) {
		printf("setting lan activity window to %ius",temp_LAN_activity_window_us);
		LAN_activity_window_us = temp_LAN_activity_window_us;
	}

	/* Now we go through and override with values from the command line */
	if (tmp_read_time > 0) {
	read_time = tmp_read_time;
	}

	if (tmp_serial_port[0] != 0) {
	strncpy( serial_port, tmp_serial_port, sizeof(serial_port)-1 );
	}

	if (tmp_log_file[0] != 0) {
	strncpy( log_file, tmp_log_file, sizeof(log_file)-1 );
	}

	if (tmp_log_type != -1) {
		log_type = tmp_log_type;
		if ( tmp_log_type == 0 )
		{
			strncpy( temp_format, tmp_temp_format, sizeof(temp_format)-1 );
		}
	}

	if( tmp_counter_format[0] != 0 ) {
		strncpy( counter_format, tmp_counter_format, sizeof(counter_format)-1 );
	}

	if( tmp_humidity_format[0] != 0 ) {
		strncpy( humidity_format, tmp_humidity_format, sizeof(humidity_format)-1 );
	}

	/* Show the copyright banner? */
	if( !(opts & OPT_QUIET) )
	{
		printf(BANNER_1);
		printf(BANNER_2);
	}

#ifndef OWUSB
	// Check to see if the device file actually exists
	if( !file_exists( serial_port ) )
	{
		fprintf( stderr, "Error, serial port '%s' does not exists!\n", serial_port );
		// 2014-10-19 DRJ:	sensor_list has changed to global_sensor_list
		//		couplerDS_top is no longer used
		//		allocated variables are now: global_roms_list, global_sensor_list, and global_coupler_list
		if(global_roms_list != NULL) free(global_roms_list);
		if(global_sensor_list != NULL) free(global_sensor_list);
		if(global_coupler_list != NULL) free(global_coupler_list);

		exit(EXIT_NOPORT);
	}

	/* Check to make sure we have permission to access the port */
	if( access( serial_port, R_OK|W_OK ) < 0 ) {
		fprintf( stderr, "Error, you don't have +rw permission to access serial port: %s\n", serial_port );
		if(global_roms_list != NULL) free(global_roms_list);
		if(global_sensor_list != NULL) free(global_sensor_list);
		if(global_coupler_list != NULL) free(global_coupler_list);
		exit(EXIT_NOPERM);
	}

	/* Lock our use of the serial port, exit if it is in use */
#ifdef LINUX
#ifndef OWUSB
#ifdef LOCKDEV
	/* First turn serial_port into just the final device name */
	if( !(p = strrchr( serial_port, '/' )) )
	{
		fprintf( stderr, "Error getting serial device from %s\n", serial_port );
		// 2014-10-19 DRJ:	sensor_list has changed to global_sensor_list
		//		couplerDS_top is no longer used
		//		allocated variables are now: global_roms_list, global_sensor_list, and global_coupler_list
		if(global_roms_list != NULL) free(global_roms_list);
		if(global_sensor_list != NULL) free(global_sensor_list);
		if(global_coupler_list != NULL) free(global_coupler_list);

		exit(EXIT_DEVERR);
	}
	strncpy( serial_dev, p+1, sizeof(serial_dev)-1 );

	if( (pid = dev_lock( serial_dev )) != 0 )
	{
		if( pid == -1 )
		{
			fprintf( stderr, "Error locking %s. Do you have permission to write to /var/lock?\n", serial_dev );
		} else {
			fprintf( stderr, "Error, %s is locked by process %d\n", serial_dev, pid );
		}
		if(global_roms_list != NULL) free(global_roms_list);
		if(global_sensor_list != NULL) free(global_sensor_list);
		if(global_coupler_list != NULL) free(global_coupler_list);

		exit(EXIT_LOCKED);
	}
#endif		/* LOCKDEV	*/
#endif		/* OWUSB	*/
#endif		/* LINUX 	*/
#endif		/* !OWUSB 	*/

	/* Connect to the MLan network */
#ifndef OWUSB
	if( !owAcquire( 0, serial_port) )
	{
#else
	if( !owAcquire( 0, serial_port, temp ) ){
		fprintf( stderr, "USB ERROR: %s\n", temp );
#endif

		/* Error connecting, print the error and exit */
		OWERROR_DUMP(stdout);

		if(roms != NULL)
			free(roms);

		if(couplers != NULL)
			free(couplers);

#ifdef LINUX
#ifndef OWUSB
#ifdef LOCKDEV
		dev_unlock( serial_dev, 0 );
#endif		/* LOCKDEV	*/
#endif		/* OWUSB	*/
#endif		/* LINUX	*/
		exit(EXIT_ERR);
	}
#ifdef OWUSB
#endif  // OWUSB


	/* Should we walk the whole LAN and display all devices? */
	if( opts & OPT_WALK ){
		Walk1Wire();

		if(roms != NULL)
			free(roms);

		if(couplers != NULL)
			free(couplers);

#ifndef OWUSB
			owRelease(0);
#else
			owRelease(0, temp );
#endif /* OWUSB */

#ifdef LINUX
#ifndef OWUSB
#ifdef LOCKDEV
		dev_unlock( serial_dev, 0 );
#endif		/* LOCKDEV	*/
#endif		/* OWUSB	*/
#endif		/* LINUX	*/

		exit(EXIT_OK);
	}


	// ------------------------------------------------------------------//
	// Should we initalize the sensors?                                  //
	// This should store the serial numbers to the .digitemprc file      //
	if(opts & OPT_INIT) {
		if(Init1WireLan() != 0) {
			if(global_roms_list != NULL) free(global_roms_list);
			if(global_sensor_list != NULL) free(global_sensor_list);
			if(global_coupler_list != NULL) free(global_coupler_list);

			// Close the serial port
#ifndef OWUSB
			owRelease(0);
#else
			owRelease(0, temp );
			fprintf( stderr, "USB ERROR: %s\n", temp );
#endif // OWUSB

#ifdef LINUX
#ifndef OWUSB
#ifdef LOCKDEV
			dev_unlock( serial_dev, 0 );
#endif		// LOCKDEV
#endif		// OWUSB
#endif		// LINUX

			exit(EXIT_ERR);
		}
	}


	/* Record the starting time */
	start_time = time(NULL);

	/* Sample the prescribed number of times, 0=infinity */
	for( x = 0;num_samples==0 || x < num_samples; x++ )
	{
		last_time = time(NULL);
		elapsed_time = last_time - start_time;

		switch( log_type )
		{
			/* For this type of logging we print out the elapsed time at the
				 start of the line
			 */
			case 2:
			case 3:	sprintf(temp, "%ld", elapsed_time );
								log_string( temp );
		break;
			default:
		break;
		}
		/* Should we read just one sensor? */
		if( opts & OPT_SINGLE )
		{
			read_device(sensor );
		}

		/* Should we read all connected sensors? */
		if( opts & OPT_ALL ) {
			read_all();
		}

		switch( log_type )
		{
			/* For this type of logging we print out the elapsed time at the
				 start of the line
			 */
			case 2:
			case 3:	log_string( "\n" );
		break;
			default:
		break;
		}

		/* Wait until we have passed last_time + sample_delay. We do it
			 this way because reading the sensors takes a certain amount
			 of time, and sample_delay may be less then the time needed
			 to read all the sensors. We should complain about this.
		*/
		if( (time(NULL) > last_time + sample_delay) && (sample_delay > 0) )
		{
			fprintf(stderr, "Warning: delay (-d) is less than the time needed to ");
			fprintf(stderr, "read all of the attached sensors. It took %ld seconds", (long int) time(NULL) - last_time );
			fprintf(stderr, " to read the sensors\n" );
		}

		/* Should we delay before the next sample? */
		if( sample_delay > 0 )
		{
			/* Sleep for the remaining time, if there is any */
			if( (time(NULL) - last_time) < sample_delay )
				sleep( sample_delay - (time(NULL) - last_time) );
		}
		// make certain all the hubs are off??
		// TODO	
	}



	if( roms != NULL )
		free( roms );

	if(couplers != NULL)
		free(couplers);

#ifndef OWUSB
	owRelease(0);
#else
	owRelease(0, temp );
#endif /* OWUSB */

#ifdef LINUX
#ifndef OWUSB
#ifdef LOCKDEV
	dev_unlock( serial_dev, 0 );
#endif		/* LOCKDEV	*/
#endif		/* OWUSB	*/
#endif		/* LINUX	*/

	exit(EXIT_OK);
} // main?




unsigned short int Get_2800_Pio(int portnum) {
	unsigned short int pio = -1;

	if(DrjOwAccess(portnum)) {
		// read pio command
		DrjOwWriteByte(portnum, 0xf5);
		pio=DrjOwReadByte(portnum);
	}
		if(DrjOwAccess(portnum)) {
		return (pio);
	} else {
		return -1;
	}
}

/* -----------------------------------------------------------------------
	 Read the DS28ea00 temperature or PIO by Tomasz R. Surmacz
	 (tsurmacz@ict.pwr.wroc.pl)
	 ----------------------------------------------------------------------- */
int read_pio_ds28ea00( int sensor_family, int sensor )
{
	unsigned char pio;
	char		temp[1024],
					time_format[160];
	time_t	mytime;


	if ( (sensor_family == DS28EA00_FAMILY) || (sensor_family == DS2413_FAMILY) )
	{
		pio = Get_2800_Pio(0);

	if ( ((pio ^ (pio>>4)) &0xf) != 0xf) {
		// upper nibble should be complement of lower nibble
					// sprintf( temp, "Sensor %d Read Error (%02x)\n", sensor, pio );
			fprintf(stderr, "Sensor %d Read Error (%02x)\n", sensor,  pio );
		return FALSE;
	}


		mytime = time(NULL);
		if( mytime )
		{
			/* Log the temperature */
			switch( log_type )
			{
				/* Multiple Centigrade temps per line */
		case 3:
				case 2:     sprintf( temp, "\t%02x", pio );
										break;

				default:
										sprintf( time_format, "%%b %%d %%H:%%M:%%S Sensor %d PIO: %02x, PIO-A: %s PIO-B: %s", sensor, pio, (pio&0x01)?"ON ":"OFF", (pio&0x04)?"ON ":"OFF" );
										/* Handle the time format tokens */
										strftime( temp, 1024, time_format, localtime( &mytime ) );
										strcat( temp, "\n" );
										break;
			} /* switch( log_type ) */
		} else {
			sprintf( temp, "Time Error\n" );
		}

		/* Log it to stdout, logfile or both */
		log_string( temp );
	}

	return FALSE;
}


int FindCouplersAndTurnThemOff(int currentHubNo, int	currentHubCh, int verbose) {
	// This Method will call itself recursively until it finds no new hubs
	// This method recoginizes DS2409 and HobbyBoard 4ch hubs.
	//
	// Nested couplers may be in such a state such that if we "see" a coupler,
	// it may be behind another coupler.
	// Therefore, we will
	//		Scan the LAN for new couplers
	//		Turn the newfound couplers off
	// 	Look for couplers
	//		If a coupler is found
	//			it is not behind another coupler.
	//			it shalll be added to the global list of known couplers
	// Find any DS2409 Couplers and turn them all off.
	// Find all HobbyBoard 4Ch Hubs (Couplers) and turn them all off.
	// 
	// set verbose to FALSE for "quiet" opperation
	// set verbose to TRUE for a step-by-step reporting
	// 
	// Nothing is recorded and the 1-Wire LAN may be left pointing to a different device.
	// global variable will be left empty.
	//		global_roms_count		global_roms_list
	//		global_sensor_count	global_sensor_list
	//		global_coupler_count	global_coupler_list
	// stops searching and return negative upon first discovery of a problem.
	// return 0	:	all is well
	// return -1 : setting DS2409 to off state failed
	// return -2 : setting HobbyBoard 4ch to off state failed
	// 
	unsigned char	TempSN[8] = {0,0,0,0,0,0,0,0},
						InfoByte[3];	// passed to SetSwitch1F()
	short result;		// holds return of call to owFirst() or owNext()
	int efConfig;		// holds return of call to FamilyEF_Initialize4Ch()
	int numHubsFound = 0;	// report how many hubs we found is this necessary???
	int numChannels = 0;
	int retCode = 0;	// return value
	int   x;
	int	y;
	int	z;

	if(currentHubNo == 0) {
   	global_roms_count = 0;
   	if(global_roms_list != NULL) free(global_roms_list);
   	global_sensor_count = 0;
   	if(global_sensor_list != NULL) free(global_sensor_list);
   	global_coupler_count = 0;
   	if(global_coupler_list != NULL) free(global_coupler_list);
   	// by default, there is the main LAN
   	AddToGlobalCouplerList(TempSN, currentHubNo, currentHubCh);
   	// Set the coupler type to zero for the main LAN
   	global_coupler_list[global_coupler_count - 1].romType = 0;
   	if(verbose){
   		printf("Turning off all DS2409 Couplers (and Hobby Board 4ch Hubs)\n");
   	}
   } // if(nestingLevel == 0)

	// search for hubs
	// only turn off hubs that are not yet known
	
	result = owFirst(0, TRUE, FALSE);
	while(result){
		owSerialNum(0, TempSN, TRUE);
		if(verbose){
			printf(".");
		}
		if (DRJ_IsRomOnList(global_coupler_list, &global_coupler_count, TempSN)) {
			// if the rom is already on the coupler list
			// do nothing but move on
		} else {
   		if(TempSN[0] == SWITCH_FAMILY) {
   			// turn the coupler off
   			// turn off the hub
   			if(!SetSwitch1F(0, TempSN, ALL_LINES_OFF, 0, InfoByte, TRUE)){
   				fprintf( stderr, "Setting DS2409 Coupler to OFF state failed.\n");
   				retCode = -1;
   			}
   		}
   		if(IsHobbyBoard4chHub(0,TempSN)){
   			efConfig = FamilyEF_Initialize4Ch(0);		// DRJ: ToDo error check the return
   			if(efConfig == 0){
   				// all is well
   				if(verbose){
   					printf("(HB4Ch turned off)");
   				}
   			} else {
   				fprintf(stderr, "ERROR: FindCouplersAndTurnThemOff() Setting HobbyBoard 4ch Coupler to OFF state failed.\n");
   				retCode = -2;
   			}
   		}
   	} // if (DRJ_IsRomOnList(Rom *romList, short *roms_count, unsigned char *TempSN))
		
		result = owNext( 0, TRUE, FALSE );
	} // initial hub search
	// now that all hubs have been turned off, any hubs that are found are genuinely on this LAN
	result = owFirst(0, TRUE, FALSE);
	// second hub search
	// add any new hubs to the global_coupler_list
	numHubsFound = -global_coupler_count;
	AddNewRomsToGlobalLists(currentHubNo, currentHubCh);
	numHubsFound += global_coupler_count;

	if(verbose){
		if(numHubsFound > 0) {
			printf("\nFound %i hubs at Hub=%i, Channel=%i\n",numHubsFound,currentHubNo,currentHubCh);
		}
		printf(".");
	}
	// now turn on each new hub/channel and recursively call this method
	for (x = 0 ; x < numHubsFound ; x++){
		z = global_coupler_count - numHubsFound + x;
		numChannels = 0;
		// COUPLER_HB4CH has four channels
		if(global_coupler_list[z].romType == COUPLER_HB4CH) {
			numChannels = 4;
		}
		//	COUPLER_DS2409 has two channels
		if(global_coupler_list[z].romType == COUPLER_DS2409) {
			numChannels = 2;
		}
		// turn on each channel and make recursive call
		for (y = 1; y <= numChannels ; y++) {
			TurnOnHubAndChannel(z, y);
			FindCouplersAndTurnThemOff(z,	y,	verbose);
		}
		// channel 0 is the same as turning off the hub
		TurnOnHubAndChannel(z, 0);
	}

	// lets not worry about freeing memory
	/*
	if(currentHubNo == 0) {
   	if(verbose){
   		printf("\nFound %i hubs and turned them off.\n\n",global_coupler_count);
   	}
   	global_roms_count = 0;
   	if(global_roms_list != NULL){
			free(global_roms_list);
			global_roms_list = NULL;
		}
   	global_sensor_count = 0;
   	if(global_sensor_list != NULL) {
   		free(global_sensor_list);
			global_sensor_list = NULL;
   	}
   	global_coupler_count = 0;
   	if(global_coupler_list != NULL) {
   		free(global_coupler_list);
			global_coupler_list = NULL;
   	}
	}
	*/
	
	return retCode;		// DRJ: haven't done any error proofing/reporting yet!
}