/* -----------------------------------------------------------------------
   DigiTemp

   Copyright 1996-2005 by Brian C. Lane <bcl@brianlane.com>
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


   
   2013-07-08: DRJ- Modified _coupler to add 2 more "channels" for 4 channel hub
   2013-08-19: DRJ- No longer using a linked list of hubs.  Instead, using array.
   ----------------------------------------------------------------------- */

#if !defined(DIGITEMP_H)
#define	DIGITEMP_H			"2013-07-21"

#define BANNER_1     "DigiTemp v3.6.0-DRJ Copyright 1996-2007 by Brian C. Lane\n"
#define BANNER_2     "GNU Public License v2.0 - http://www.digitemp.com\n"
#define BANNER_3     "Compiled for %s\n\n"

#define OPT_INIT     0x0001
#define OPT_VERBOSE  0x0002
#define OPT_SINGLE   0x0004
#define OPT_ALL      0x0008
#define OPT_QUIET    0x0010
#define OPT_WALK     0x0020
#define OPT_DS2438   0x0040
#define OPT_SORT     0x0080
#define OPT_TEST     0x0100


/* Family codes for supported devices */
#define DS1820_FAMILY		0x10
#define DS1822_FAMILY		0x22
#define DS18B20_FAMILY	0x28
#define DS28EA00_FAMILY	0x42
#define DS1923_FAMILY		0x41
#define DS2406_FAMILY		0x12
#define DS2422_FAMILY		0x1C
#define DS2423_FAMILY		0x1D
#define DS2438_FAMILY		0x26
// and non-supported yet, but coming soon
#define DS2408_FAMILY		0x29
#define DS2413_FAMILY		0x3A


/* Coupler related definitions */
#define SWITCH_FAMILY						0x1F
#define HOBBYBOARD_EF_FAMILY		0xEF		// DRJ- Hobby Boards 4ch hub
#define MAXDEVICES         15
#define ALL_LINES_OFF      0
#define DIRECT_MAIN_ON     1
#define AUXILARY_ON        2
#define STATUS_RW          3

// used by struct _copuler to define type of hub
#define MAIN_LAN					-1
#define COUPLER_DS2409		1		// DS2409 MicroLAN Coupler
#define COUPLER_HB4CH			2		// Hobby Boards 4ch hub
#define SENSOR_DS2800_PIO				11	// PIO
#define SENSOR_DS1820_TEMP			12	// DS1820 DES1822 DS18B20Temperature
#define SENSOR_DS1923_TEMP			13	// DS1923 Temperature
#define SENSOR_DS2422_COUNTER		14	// DS2422 DS2423 Counter
#define SENSOR_DS2438_BATT			15	// DS2438 A/D Battery Monitor???
#define SENSOR_DS2438_HUMID			16	// DS2438 Temperature and Humidity




/* Exit codes */
#define EXIT_OK   0      /* No errors                          */
#define EXIT_ERR  1      /* Couldnt read temp                  */
#define EXIT_NORC 2      /* Couldn't read the rc file          */
#define EXIT_HELP 3      /* Exit and showing help              */
#define EXIT_NOPERM 4    /* No permission to use serial port   */
#define EXIT_LOCKED 5	 /* Serial port is locked              */
#define EXIT_DEVERR 6    /* Error getting serial device        */
#define EXIT_NOPORT 7    /* Port device file doesn't exists    */

/* Number of tries to read a sensor before giving up */
#define MAX_READ_TRIES	3

/*
	In order to aid in "calling up" roms, add one field
	int						couplerCH;	0,1,2,3,4	0 = main net,
	
	when initialized, couplerCH would be set to -1?
	if couplerCH is 0, then the device is on the main lan
	if couplerCH is 1 or greater, then the romSN can be interigated to determine it's type
	
	In order to accomodate nested hubs, Hubs will indicate what couplerNum/Ch they are on.
	*/		

struct _rom {
	unsigned char	romSN[8];	// SN of rom
	int	romType;					// Either COUPLER_DS2409, COUPLER_HB4CH, SENSOR_DSXXXX -Maybe this should be family code?
	int	couplerNo;				// if 0 then device is on the main lan, 1 = device is on Coupler[1]
	int	couplerCh;				// Hub ch
};

typedef struct _rom Rom;


/* Prototypes */
void usage();
//void free_coupler_ds2409();
void free_coupler_mem( int free_only );
float c2f( float temp );
int build_tf( char *time_format, char *format, int sensor, 
              float temp_c, int humidity, unsigned char *sn );
int build_cf( char *time_format, char *format, int sensor, int page,
              unsigned long count, unsigned char *sn );
int log_string( char *line );
int log_temp( int sensor, float temp_c, unsigned char *sn );
int log_counter( int sensor, int page, unsigned long counter, unsigned char *sn );
int log_humidity( int sensor, double temp_c, int humidity, unsigned char *sn );
int cmpSN( unsigned char *sn1, unsigned char *sn2, int branch );
void show_scratchpad( unsigned char *scratchpad, int sensor_family );
int read_temperature( int sensor_family, int sensor );
int read_counter( int sensor_family, int sensor );
int read_ds2438( int sensor_family, int sensor );
int read_humidity( int sensor_family, int sensor );
int read_device(int sensor);
int read_all();
int read_rcfile(char *fname);
int write_rcfile();
void printSN( unsigned char *TempSN, int crlf );
int Walk1Wire();
int Init1WireLan();
int read_pio_ds28ea00( int sensor_family, int sensor );

int AddToGlobalRomList(unsigned char *TempSN, int _couplerNo, int _couplerCh);
int AddToGlobalCouplerList(unsigned char *TempSN, int _couplerNo, int _couplerCh);


int AddToGlobalSensorList(unsigned char *TempSN, int _couplerNo, int _couplerCh);

int AddNewRomsToGlobalLists(int _couplerNo, int _couplerCh);
int TurnOnHubAndChannel(int hubToTurnOn, int chToTurnOn);
int FindCouplersAndTurnThemOff(int currentHubNo, int	currentHubCh, int verbose);

/* From ds2438.c */
int get_ibl_type(int portnum, unsigned char page, int offset);

/* Local Variables: */
/* mode: C */
/* compile-command: "cd ..; make -k" */
/* End: */
#endif /* !DIGITEMP_H */   
