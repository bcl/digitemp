/* -----------------------------------------------------------------------
   DRJ Mod of DigiTemp
   
   
   2013-07-08: DRJ
   
   	Added family code "0xEF Hobby Boards Microprocessor-based slave"
   	Added support for "4CH2-R1-A Hobby Boards 4ch Hub"

   	
   -----------------------------------------------------------------------*/

#include "digitemp.h"
#include "DRJ_digitemp.h"
#include "device_name.h"
#include "ownet.h"
#include "owproto.h"

extern	Rom*			global_coupler_list;		// Attached Roms that are couplers needing further investigation
extern	short			global_coupler_count;		// number of couplers in coupler_list
extern	struct timeval	LAN_activity_last_time;		// last time of LAN activity
extern	int			LAN_activity_window_us;		// LAN activity time window in micro-seconds.

#define	DEBUG_WAIT_TIME 0

int	DRJ_WaitForWindow(struct timeval *last_time, int window_us) {
	int	timeToWait;
	timeToWait = LAN_activity_window_us - TimevalElapsed_us(&LAN_activity_last_time);
	if(timeToWait > 0) {
		usleep(timeToWait);
	} else {
		if(DEBUG_WAIT_TIME) printf("DEBUG DRJ_digitemp.c DRJ_WaitForWindow() Window time exceeded already, No Need to wait.\n");
	}
	if(DEBUG_WAIT_TIME) printf("DEBUG DRJ_digitemp.c DRJ_WaitForWindow() Time since last call=%ius\n",TimevalElapsed_us(&LAN_activity_last_time));
	TimeSetToNow(&LAN_activity_last_time);
	return 0;
}

int DrjOwReadByte(int portnum) {
	int retNum;
	// wait for call time
	if(DEBUG_WAIT_TIME) printf("DrjOwReadByte()  ");
	DRJ_WaitForWindow(&LAN_activity_last_time, LAN_activity_window_us);
	// call owReadByte()
	retNum = owReadByte(portnum);
	return retNum;
}



int DrjOwWriteByte(int portnum, SMALLINT sendbyte) {
	int retNum;
	// wait for call time
	if(DEBUG_WAIT_TIME) printf("DrjOwWriteByte() ");
	DRJ_WaitForWindow(&LAN_activity_last_time, LAN_activity_window_us);
	// call owWriteByte()
	retNum = owWriteByte(portnum, sendbyte);
	return retNum;
}

SMALLINT DrjOwAccess(int portnum) {
	SMALLINT retNum;
	// wait for call time
	if(DEBUG_WAIT_TIME) printf("DrjOwAccess()    ");
	DRJ_WaitForWindow(&LAN_activity_last_time, LAN_activity_window_us);
	// call owAccess()
	retNum = owAccess(portnum);
	return retNum;
}

// time meathods
int TimeSetToNow(struct timeval *timeToSet) {
	gettimeofday(timeToSet,NULL);
	return 0;
}

int TimevalElapsed_us(struct timeval *startTime) {
	// returns time in microseconds elapsed since startTime
	//	if time elapsed exceeds int, then return max int
	struct timeval now;
	long	elapsedTime;
	gettimeofday(&now,NULL);									// what time is it right now
	
	elapsedTime = TimevalDiff_us(&now, startTime);
	
	if(elapsedTime > 10000000) {	// if elapsedTime is greater than 10s, then make it 10s
		elapsedTime = 10000000;
	}
	return (int)elapsedTime;
}

long	TimevalDiff_us (struct timeval *minuend, struct timeval *subtrahend){
  /* Subtract the `struct timeval' values minuend and subtrahend,
   * storing the difference in difference.
	 *	difference is returned as an int.
   *	difference = minuend - subtrahend
   *	tv_sec and tv_usec are type "long"
   *
   *	Returns time difference in microseconds.
   */

	//start with difference = 0;
	long difference = 0;

	//calculate the difference using native (long) values
	//first, perform any carries on subtrahend
	if (minuend->tv_usec < subtrahend->tv_usec) {			// we need to "borrow" from the subtrahend->tv_sec, a sort of negagive borrow!
  	int nsec = (subtrahend->tv_usec - minuend->tv_usec) / 1000000 + 1;
  	subtrahend->tv_usec -= 1000000 * nsec;
  	subtrahend->tv_sec += nsec;
	}
	// if somehow the minuend - subrahend is too big fo the usec, carry on to the subtrahend
	if (minuend->tv_usec - subtrahend->tv_usec > 1000000) {
  	int nsec = (minuend->tv_usec - subtrahend->tv_usec) / 1000000;
  	subtrahend->tv_usec += 1000000 * nsec;
  	subtrahend->tv_sec -= nsec;
	}

	difference = (long)(minuend->tv_sec - subtrahend->tv_sec)*1000000 + (long)(minuend->tv_usec - subtrahend->tv_usec);
	return difference;
}




double TimevalElapsed(struct timeval *startTime) {
	// returns time in seconds elapsed since startTime
	//
	struct timeval now;
	double	elapsedTime;
	gettimeofday(&now,NULL);									// what time is it right now
	
	elapsedTime = TimevalDiff(&now, startTime);
	
	return elapsedTime;
}
double	TimevalDiff (struct timeval *minuend, struct timeval *subtrahend){
  /* Subtract the `struct timeval' values minuend and subtrahend,
   * storing the difference in difference.
	 *	difference is returned as a double.
   *	difference = minuend - subtrahend
   *	tv_sec and tv_usec are type "long"
   *
   *	Returns time difference in seconds.
   */

	//start with difference = 0;
	double difference = 0.0;

	//calculate the difference using native (long) values
	//first, perform any carries on subtrahend
	if (minuend->tv_usec < subtrahend->tv_usec) {			// we need to "borrow" from the subtrahend->tv_sec, a sort of negagive borrow!
  	int nsec = (subtrahend->tv_usec - minuend->tv_usec) / 1000000 + 1;
  	subtrahend->tv_usec -= 1000000 * nsec;
  	subtrahend->tv_sec += nsec;
	}
	// if somehow the minuend - subrahend is too big fo the usec, carry on to the subtrahend
	if (minuend->tv_usec - subtrahend->tv_usec > 1000000) {
  	int nsec = (minuend->tv_usec - subtrahend->tv_usec) / 1000000;
  	subtrahend->tv_usec += 1000000 * nsec;
  	subtrahend->tv_sec -= nsec;
	}

	difference = (double)(minuend->tv_sec - subtrahend->tv_sec) + (double)(minuend->tv_usec - subtrahend->tv_usec)/1000000;
	return difference;
}






/*
	sercpy(sn1, sn2)
	copy serial number 2 to serial number 1
 */
int sercpy(unsigned char *sn1, unsigned char *sn2){
	int i;
	
	for(i=0; i<8; i++){
		sn1[i] = sn2[i];
	}
//	printf("\n");
	return 0;
}


int serprint(unsigned char *sn1) {
	int i;
	
	for (i=0; i<8; i++){
		printf("%i",sn1[i]);
	}
	
	return 0;
}
	

/* -----------------------------------------------------------------------
   Compare 2 serial numbers (8 bytes)
   
   Return:
     -1 if the 2nd is < 1st
     0 if equal
     1 if the 2nd is > 1st
   ----------------------------------------------------------------------- */
int sercmp( unsigned char *sn1, unsigned char *sn2 ) {
	int i;
	
	for (i=0; i<8; i++){
		if (sn2[i] < sn1[i])
			return -1;
		if (sn2[i] > sn1[i])
			return 1;
	}
	
	return 0;
}


/*
	We assume this is a HobbyBoard 4ch hub with ConfigSet to single channel mode
	chToTurnOn is the channel to turn on.
	if chToTurnOn is set to 0, all channels will be turned off
				FamilyEF_SetChannels(0, 18); // turn on channel 2 (xxx1 0010)

	return -1	:	could not access 1-wire device
	return -2	:	channel out of range	
*/

int FamilyEF_SetChannels(int portnum, int chToTurnOn) {
	if((chToTurnOn < 0)|| (chToTurnOn > 4)){
		// channel out of range
		fprintf( stderr, "ERROR: DRJ_digitemp.c FamilyEF_SetChannels() channel out of range. chToTurnOn = %i.\n",chToTurnOn);
		return -2;
	}
	
	if(DrjOwAccess(portnum))
	{
		DrjOwWriteByte(portnum, 0x21);							// set channels command
		switch(chToTurnOn) {
			case 0	:	DrjOwWriteByte(portnum, 15);	// turn off all channels (xxx0 1111);
								break;
			case 1	:	DrjOwWriteByte(portnum, 17);	// turn on ch1 (xxx1 0001);
								break;
			case 2	:	DrjOwWriteByte(portnum, 18);	// turn on ch2 (xxx1 0010);
								break;
			case 3	:	DrjOwWriteByte(portnum, 20);	// turn on ch3 (xxx1 0100);
								break;
			case 4	:	DrjOwWriteByte(portnum, 24);	// turn on ch4 (xxx1 1000);
								break;
			default	: printf("FamilyEF_SetChannels() This shouldnt be happening! We should never get here!\n");
		}
	} // if(DrjOwAccess(portnum))
	else
	{
		// could not access 1-wire device
		fprintf(stderr,"ERROR: DRJ_digitemp.c FamilyEF_SetChannels() Failed to gain access to device-%i\n",portnum);
		return -1;
	}	// else if(DrjOwAccess(portnum))
	return FamilyEF_GetConfig(portnum);
}

/*
	FamilyEF_Initialize4Ch()
	
	return 0	:	all is well
	return -1	:	failed to get config
	return -2	:	failed to turn off ports (to be implemented!)
*/

int FamilyEF_Initialize4Ch(int portnum) {
	// assumes a 4ch Hub has been found on the network.
	// SetConfig to Single Channel
	// Turn off all ports
	int returnConfigGet;		// future used for FamilyEF_GetConfig() call
	int returnConfigSet = -1;		// future used for FamilyEF_SetConfig() call

	returnConfigGet = FamilyEF_GetConfig(portnum);


	if(returnConfigGet != -1) {
		// setConfig to Single Channel (bit 2 = 1)
		returnConfigSet = FamilyEF_SetConfig(portnum, 2);	// HobbyBoards reccomends that we use GetConfig, changing only the second bit
		if(!(returnConfigSet & 0x02)) {
			fprintf(stderr, "Error: Failed to set Hobbyboard to Single CHannel, Hobbyboard is in config %x\n",returnConfigSet);
		}
		// turn all ports off
		FamilyEF_SetChannels(portnum,0); // turn off channel 1,2,3,and 4 (xxx0 1111)
	}
	
	return 0;		// really not so cool.  We should verify that we successfully turned off all ports
}

// sets config for Hobby Boards EF family.  Only second bit is used.  Returns config.
int FamilyEF_SetConfig(int portnum, int byteToSend) {

	if(DrjOwAccess(portnum)) {
		// read device type
		DrjOwWriteByte(portnum, 0x60);
		DrjOwWriteByte(portnum, byteToSend);
	}

	return FamilyEF_GetConfig(portnum);
}

// gets config for Hobby Boards EF family.  Only second bit is used
int FamilyEF_GetConfig(int portnum) {
	int returnConfig = -1;

	if(DrjOwAccess(portnum)) {
		// read device type
		DrjOwWriteByte(portnum, 0x61);
		returnConfig = DrjOwReadByte(portnum);
	}

	return returnConfig;
}

int FamilyEF_SearchRom(int portnum) {
	int returnType = -1;

	if(DrjOwAccess(portnum)) {
		// call SearchRom
		DrjOwWriteByte(portnum, 0xF0);
		returnType=DrjOwReadByte(portnum);
	}
	if(DrjOwAccess(portnum)) {
		return(returnType);
	} else {
		return -1;
	}
}

// returns type
// should return 0x05 for Hobby Boards 4ch Hub
int FamilyEF_GetType(int portnum) {
	int returnType = -1;
	if(DrjOwAccess(portnum)) {
		// read device type
		DrjOwWriteByte(portnum, 0x12);
		returnType=DrjOwReadByte(portnum);
	}
	return returnType;
}

// turns off the Main and the Aux lines of the DS2409.
// Also clears event flags and ends a discharge cycle.
int DS2409_AllLinesOff(int portnum) {
	if(DrjOwAccess(portnum)) {
		// Send "All Lines Off" command
		DrjOwWriteByte(portnum, 0x66);
	}
	return 0;	// DRJ ToDo: What should I return???
}

int DS2409_DirectOnMain(int portnum) {
	if(DrjOwAccess(portnum)) {
		// Send "Direct-On Main" command
		DrjOwWriteByte(portnum, 0xA5);
	}

	return 0;	// DRJ ToDo: What should I return???
}

int DS2409_SmartOnMain(int portnum) {
	if(DrjOwAccess(portnum)) {
		// Send "Smart-On Main" command
		DrjOwWriteByte(portnum, 0xCC);
	}
	return 0;	// DRJ ToDo: What should I return???
}

int DS2409_SmartOnAux(int portnum) {
	if(DrjOwAccess(portnum)) {
		// Send "Smart-On Aux" command
		DrjOwWriteByte(portnum, 0x33);
	}
	return 0;	// DRJ ToDo: What should I return???
}



int DS2409_StatusReadWrite(int portnum, int byteToSend) {
	if(DrjOwAccess(portnum)) {
		// Send "Status Read/Write" command
		DrjOwWriteByte(portnum, 0x5A);
		DrjOwWriteByte(portnum, byteToSend);
	}

	return 0;	// DRJ ToDo: What should I return???
}


int DS2409_SetChannels(int portnum, int hubToTurnOn, int chToTurnOn) {
	int smart_main = 4;
	int smart_aux = 2;
	int numextra = 2;
	uchar extra[3];

	if((chToTurnOn < 0) || (chToTurnOn > 4)){
		// channel out of range
		fprintf( stderr, "DS2409_SetChannels() channel out of range. chToTurnOn = %i.\n",chToTurnOn);
		return -2;
	}
	// 128  64  62  16   8   4   2   1
	
            // bytes 0-2: don't care
            // bytes 3-4: write control 0 to change status
            // byte 5: 0 = auto-control, 1 = manual mode
            // byte 6: 0 = main, 1 = auxiliary
            // byte 7: value to be written to control output, manual mode only
            // 0x00 default value
	if(DrjOwAccess(portnum)) {
		switch (chToTurnOn ) {
			case 0:
				SetSwitch1F(portnum, global_coupler_list[hubToTurnOn].romSN, 0, 0, extra, TRUE);
				break;
			case 1:
				SetSwitch1F(portnum, global_coupler_list[hubToTurnOn].romSN, smart_main, numextra, extra, TRUE);
				break;
			case 2:
				SetSwitch1F(portnum, global_coupler_list[hubToTurnOn].romSN, smart_aux, numextra, extra, TRUE);
				break;
		}
	} else {
		// could not access 1-wire device
		return -1;
	}
	return 0;		// should return the result of SetSwitch1F()
}


int DRJ_IsRomOnList(Rom *romList, short *roms_count, unsigned char *TempSN) {
	int	isOnList;
	int i;
	
	isOnList = FALSE;

	for(i = 0; i < (*roms_count); i++) {
		if(sercmp(&romList[i].romSN[0],TempSN) == 0) {
			isOnList = TRUE;
		} else {
			// do nothing
		}
  }
	return isOnList;
}






int IsHobbyBoard4chHub(int portnum, unsigned char *TempSN){
	// return 1 if TempSN is a Hobby Board 4ch Hub
	// return 0 if it is not
	if(TempSN[0] == HOBBYBOARD_EF_FAMILY) {	// first byte tells the family type
		if(FamilyEF_GetType(portnum) == 5) {	// type 5 is a Hobby Board 4ch Hub
			return 1;
		}
	}
	return 0;
}


int IsTempSensorOrPIO(unsigned char *TempSN){
	// Check to see if it is a temperature sensor or a PIO device
	if( (TempSN[0] == DS1820_FAMILY) ||
			(TempSN[0] == DS1822_FAMILY) ||
			(TempSN[0] == DS28EA00_FAMILY) ||
			(TempSN[0] == DS18B20_FAMILY)||
			(TempSN[0] == DS1923_FAMILY) ||
			(TempSN[0] == DS2406_FAMILY) ||
			(TempSN[0] == DS2413_FAMILY) ||
			(TempSN[0] == DS2422_FAMILY) ||
			(TempSN[0] == DS2423_FAMILY) ||
			(TempSN[0] == DS2438_FAMILY)	) {
		return TRUE;
	}
	return FALSE;
}




