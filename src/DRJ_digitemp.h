/* -----------------------------------------------------------------------
   DigiTemp- DRJ Mod
   
   2013-07-08: DRJ- Modified _coupler to add 2 more "channels" for 4 channel hub
   ----------------------------------------------------------------------- */
#if !defined(DRJ_DIGITEMP_H)
#define	DRJ_DIGITEMP_H			"2013-07-21"


#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>	//for struct timeval




//--------------------------------------------------------------//
// Typedefs from ownet.h
//--------------------------------------------------------------//
#ifdef __C51__
//   #define FILE int
//   extern void fprintf (FILE*fileNo,char*s,...);
//   #define exit(c) return
//   #define key_abort() (FALSE)
//   typedef unsigned int ushort;
//   typedef unsigned long ulong;
   #define SMALLINT uchar
#endif

#ifndef SMALLINT
   //
   // purpose of smallint is for compile-time changing of formal
   // parameters and return values of functions.  For each target
   // machine, an integer is alleged to represent the most "simple"
   // number representable by that architecture.  This should, in
   // most cases, produce optimal code for that particular arch.
   // BUT...  The majority of compilers designed for embedded
   // processors actually keep an int at 16 bits, although the
   // architecture might only be comfortable with 8 bits.
   // The default size of smallint will be the same as that of
   // an integer, but this allows for easy overriding of that size.
   //
   // NOTE:
   // In all cases where a smallint is used, it is assumed that
   // decreasing the size of this integer to something as low as
   // a single byte _will_not_ change the functionality of the
   // application.  e.g. a loop counter that will iterate through
   // several kilobytes of data should not be SMALLINT.  The most
   // common place you'll see smallint is for boolean return types.
   //
   #define SMALLINT int
#endif












/* Prototypes */

int sercmp( unsigned char *sn1, unsigned char *sn2 );
int sercpy( unsigned char *sn1, unsigned char *sn2 );

int serprint(unsigned char *sn1);




int FamilyEF_GetConfig(int portnum);
int FamilyEF_GetType(int portnum);
int FamilyEF_Initialize4Ch(int portnum);
int FamilyEF_SetChannels(int portnum, int byteToSend);
int FamilyEF_SetConfig(int portnum, int byteToSend);

int DS2409_AllLinesOff(int portnum);
int DS2409_DirectOnMain(int portnum);
int DS2409_StatusReadWrite(int portnum, int byteToSend);
int DS2409_SetChannels(int portnum, int hubToTurnOn, int chToTurnOn);
int DS2409_SmartOnMain(int portnum);
int DS2409_SmartOnAux(int portnum);


int DRJ_IsRomOnList(Rom *romList, short *roms_count, unsigned char *TempSN);

int IsHobbyBoard4chHub(int portnum, unsigned char *TempSN);		// assume the 1-wire network is accessing the rom of interest
int IsTempSensorOrPIO(unsigned char *TempSN);

// time meathods
int TimeSetToNow(struct timeval *timeToSet);
double	TimevalElapsed(struct timeval *startTime);
double	TimevalDiff (struct timeval *minuend, struct timeval *subtrahend);


int TimevalElapsed_us(struct timeval *startTime);
long	TimevalDiff_us (struct timeval *minuend, struct timeval *subtrahend);
int	DRJ_WaitForWindow(struct timeval *last_time, int window_us);
int	DrjOwWriteByte(int portnum, SMALLINT sendbyte);
int	DrjOwReadByte(int portnum);

SMALLINT	DrjOwAccess(int portnum);



// int InitializeCouplerStruct(Couper *c);


/* Local Variables: */
/* mode: C */
/* compile-command: "cd ..; make -k" */
/* End: */
#endif // !DRJ_DIGITEMP_H