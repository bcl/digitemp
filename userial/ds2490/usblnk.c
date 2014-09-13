//---------------------------------------------------------------------------
//	usblnk.c
// Copyright (C) 2001 Matthew Dharm, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Matthew is not activily supporting this code but can be reached at:
// mailto:mdharm-usb@one-eyed-alien.net
//---------------------------------------------------------------------------
//
//  usb_drv.c - General 1-Wire link layer for the DS2490 on Linux 
//
//
//  Version: 3.00B
//
//
//           3.00 -> 3.10:  bcl	  Implemented strong pullup and program pulse
//                                support that was keeping it from working
//                                correctly using owWriteBytePower()
//
/*
	DRJ 2013-07-28 Changes
71	usb_dev_handle	->	libusb_device_handle
107	usb_control_msg ->	libusb_control_transfer
121 usb_bulk_read		->	libusb_bulk_transfer
166	usb_control_msg ->	libusb_control_transfer
173	usb_bulk_read		->	libusb_bulk_transfer
183	usb_bulk_read		->	libusb_bulk_transfer
187	usb_clear_halt  ->	libusb_clear_halt
214	usb_control_msg ->	libusb_control_transfer
221	usb_bulk_read		->	libusb_bulk_transfer
231	usb_bulk_read		->	libusb_bulk_transfer
235	usb_clear_halt  ->	libusb_clear_halt
290	usb_control_msg	->	libusb_control_transfer
297	usb_bulk_read		->	libusb_bulk_transfer
307	usb_control_msg	->	libusb_control_transfer
314	usb_bulk_read		->	libusb_bulk_transfer
367	usb_control_msg	->	libusb_control_transfer
375	usb_bulk_read		->	libusb_bulk_transfer
*/
#include "ownet.h"
//#include "usb.h"
#include <libusb-1.0/libusb.h>
#include <errno.h>
#include <time.h>
#include "usblnk.h"
#include <string.h>
//#include "DRJ_digitemp.h"

/* wrap in Linux? */
#include <sys/time.h>
/* Wrap in Linux? */

// was set to 5000
#define TIMEOUT_VALUE	5000

/* the structure we'll use to access other devices */
extern struct libusb_device_handle *usb_dev_handle_list[MAX_PORTNUM];

// exportable link-level functions
SMALLINT owTouchReset(int);
SMALLINT owTouchBit(int,SMALLINT);
SMALLINT owTouchByte(int,SMALLINT);
SMALLINT owWriteByte(int,SMALLINT);
SMALLINT owReadByte(int);
SMALLINT owSpeed(int,SMALLINT);
SMALLINT owLevel(int,SMALLINT);
SMALLINT owProgramPulse(int);
void msDelay(int);
long msGettick(void);
SMALLINT hasPowerDelivery(int);
SMALLINT hasOverDrive(int);
SMALLINT hasProgramPulse(int);
SMALLINT owWriteBytePower(int,SMALLINT);
SMALLINT owReadBitPower(int, SMALLINT);

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
// Returns: TRUE(1):  presense pulse(s) detected, device(s) reset
//          FALSE(0): no presense pulses detected
//
// DRJ: Considering using libusb_bulk_transfer() instead of usb_bulk_read()
int owTouchReset(int portnum){
	int result;
	unsigned char buffer[0x20];
	int transferred;		// number of bytes transferred using libusb_bulk_transfer

	memset(buffer,0,0x20);

	// issue the 1-wire reset (DS2490 Datasheet 19-4993; 10/09 page 21)
	// DS2490 Datasheet 19-4993; 10/09 page 41 describes command 
	//	PST = 1 continuously generate 1-Wire Reset sequences until a presence pulse is discovered. 
	//	IM = 1 enables immediate execution of the command. Assumes that all 1-Wire device data required by the command has been received at EP2. 
	result = libusb_control_transfer(	usb_dev_handle_list[portnum],	//dev_handle
														0x40, 							//bmRequestTyp
														COMM_CMD, 						//bRequest
														0x4043, 							//wValue PST IM
														0x0000, 							//wIndex
														NULL,								//data
														0x0, 								//wLength
														TIMEOUT_VALUE);				//timeout
	//printf("DEBUG: owTouchReset() libusb_control_transfer returned [%i]\n",result);
	// get the status,
	// look at byte 0x10
	//		A value of 0x01 indicates an error with one of the following:
	//		1-WIRE RESET did not reveal a Presence Pulse. SET
	//		PATH command did not get a Presence Pulse from the
	//		branch that was to be connected. No response from one
	//		or more ROM ID bits during a SEARCH ACCESS
	//		command.
	//		if it is 0x00 all is well
	//	state registers...
	// buffer = 00:01	SPUE	If set to 1, the strong pullup to 5V is enabled, if set to 0, it is disabled. 
	//				01:00 current 1-Wire bus speed code
	//				02:20 current pullup duration
	//				03:40
	//				04:05	current pulldown slew rate code
	//				05:04	current Write-1 low time code
	//				06:04	current data sample offset/ Write-0 recovery time code
	//				07:00	test register
	//				08:20	IDLE
	//				09:43	communication command byte 1
	//				0A:40	communication command byte 2
	//				0B:00	communcationc command buffer status
	//				0C:00	data out buffer status
	//				0D:00	data in buffer status
	//				0E:00	reserved
	//				0F:00 reserved
	//	
	//	Sometimes only 16 bytes are returned... is this because there is nothing more interesting to report?
	//	Can we then assume that there is not problem (byte 17 is used to indicate an error).
	
	result = libusb_bulk_transfer(usb_dev_handle_list[portnum], 
														0x81,								//endpoint
														buffer,							//data
														0x20, 							//length
														&transferred,					//transferred bytes
														TIMEOUT_VALUE);				//timeout
	// libusb_bulk_transfer returns 0 upon success.
	//
	/*	DEBUG BEGIN
	printf("DEBUG: number of transferred bytes = %i\n",transferred);
	printf("buffer = ");
	// print the first 16 bytes
	for (result = 0; result < 0x10; result++) {
		printf("%02X:%02X ", result, buffer[result]);
	}
	printf("\n");
	// now, if needed, print the next 16 bytes
	if(transferred > 16) {
		printf("         ");
		for (; result < 0x20; result++) {
			printf("%02X:%02X ", result, buffer[result]);
		}
		printf("\n");
	}
	//
	DEBUG END */ 
	
	//	status bytes are described in DS2490.pdf, page 29-32
	if (result < 0) {
		return FALSE;
	}
	if (transferred > 16) {
		// there are more than just the STATE REGISTERS being reported.
		// there may be an error... look at byte 0x10
		if (buffer[0x10] & 0x01) {
			fprintf(stderr, "Error: owTouchReset() buffer[0x10] = %02x\n", buffer[0x10]);
			return FALSE;
		} 
	}
	return TRUE;
} // owTouchReset

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and return the
// result 1 bit read from the 1-Wire Net.  The parameter 'sendbit'
// least significant bit is used and the least significant bit
// of the result is the return bit.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbit'    - the least significant bit is the bit to send
//
// Returns: 0:   0 bit read from sendbit
//          1:   1 bit read from sendbit
//
int owTouchBit(int portnum, SMALLINT sendbit) {
	int result; 
	unsigned char retval;
	unsigned char buffer[0x20];
	int transferred;		// number of bytes transferred using libusb_bulk_transfer

	/* issue the bit i/o command */
	result = libusb_control_transfer(usb_dev_handle_list[portnum], 0x40,
			COMM_CMD, 0x0021 | (sendbit << 3), 0x0000, NULL, 0x0, TIMEOUT_VALUE);
	//printf("result is %d\n", result);

	/* repeat until the unit is not idle */
	do {
		/* get the status */
		result = libusb_bulk_transfer(usb_dev_handle_list[portnum], 0x81,
				buffer, 0x20, &transferred, TIMEOUT_VALUE);
		//printf("result is %d\n", result);

		//for (result = 0; result < 0x20; result++) {
		//	printf("%02X: %02X\n", result, buffer[result]);
		//}
	} while (!(buffer[0x08] & 0x20) && !(result < 0));

	/* get the data */
	result = libusb_bulk_transfer(usb_dev_handle_list[portnum], 0x83,
			&retval, 0x1, &transferred, 1000);
	if (result == -1) {
		printf ("owTouchBit: clearing halt\n");
		libusb_clear_halt(usb_dev_handle_list[portnum], 0x83);
	}


	/* return the data */
	return retval;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and return the
// result 8 bits read from the 1-Wire Net.  The parameter 'sendbyte'
// least significant 8 bits are used and the least significant 8 bits
// of the result is the return byte.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbyte'   - 8 bits to send (least significant byte)
//
// Returns:  8 bytes read from sendbyte
//
int owTouchByte(int portnum, SMALLINT sendbyte){
	int result; 
	unsigned char retval;
	unsigned char buffer[0x20];
	int transferred;		// number of bytes transferred using libusb_bulk_transfer

	/* issue the byte i/o command */
	result = libusb_control_transfer(usb_dev_handle_list[portnum],
												0x40,
												COMM_CMD,
												0x0053,
												0x0000 | sendbyte,
												NULL,
												0x0,
												TIMEOUT_VALUE);

	/* repeat until the unit is not idle */
	do {
		/* get the status */
		result = libusb_bulk_transfer(usb_dev_handle_list[portnum],
												0x81,
												buffer,
												0x20,
												&transferred, 
												TIMEOUT_VALUE);
// debug code //
//		printf("result is %d\n", result);
//		for (result = 0; result < 0x20; result++) {
//			printf("%02X:%02X ", result, buffer[result]);
//		}
//		printf("\n");
// end of debug code //
	} while (!(buffer[0x08] & 0x20) && !(result < 0));
//	printf("Debug Get The Data\n");
	/* get the data */
	result = libusb_bulk_transfer(usb_dev_handle_list[portnum],
											0x83,
											&retval,
											0x1,
											&transferred,
											1000);
	if (result == -1) {
		printf ("owTouchByte: clearing halt\n");
		libusb_clear_halt(usb_dev_handle_list[portnum], 0x83);
	}

	/* return the data */
	return retval;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'sendbyte'   - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same 
//
int owWriteByte(int portnum, SMALLINT sendbyte)
{
   return (owTouchByte(portnum,sendbyte) == sendbyte) ? TRUE : FALSE;
}

//--------------------------------------------------------------------------
// Send 8 bits of read communication to the 1-Wire Net and and return the
// result 8 bits read from the 1-Wire Net.   
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
// Returns:  8 bytes read from 1-Wire Net
//
int owReadByte(int portnum)
{
   return owTouchByte(portnum,0xFF);
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net communucation speed.  
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'new_speed'  - new speed defined as
//                MODE_NORMAL     0x00
//                MODE_OVERDRIVE  0x01
//
// Returns:  current 1-Wire Net speed 
//
int owSpeed(int portnum, SMALLINT new_speed){
	int result; 
	unsigned char buffer[0x20];
	int transferred;		// number of bytes transferred using libusb_bulk_transfer

	/* issue the command to enable speed changes */
	result = libusb_control_transfer(usb_dev_handle_list[portnum], 0x40,
			MODE_CMD, MOD_SPEED_CHANGE_EN, 0x0001, NULL, 0x0, TIMEOUT_VALUE);
	//printf("result is %d\n", result);

	/* repeat until the unit is not idle */
	do {
		/* get the status */
		result = libusb_bulk_transfer(usb_dev_handle_list[portnum], 0x81,
				buffer, 0x20, &transferred, TIMEOUT_VALUE);
		//printf("result is %d\n", result);

		//for (result = 0; result < 0x20; result++) {
		//	printf("%02X: %02X\n", result, buffer[result]);
		//}
	} while (!(buffer[0x08] & 0x20) && !(result < 0));

	/* issue the command to change the speed */
	result = libusb_control_transfer(usb_dev_handle_list[portnum], 0x40,
			MODE_CMD, MOD_1WIRE_SPEED, new_speed ? 0x0002 : 0x0000, NULL, 0x0, TIMEOUT_VALUE);
	//printf("result is %d\n", result);

	/* repeat until the unit is not idle */
	do {
		/* get the status */
		result = libusb_bulk_transfer(usb_dev_handle_list[portnum], 0x81,
				buffer, 0x20, &transferred, TIMEOUT_VALUE);
		//printf("result is %d\n", result);

		//for (result = 0; result < 0x20; result++) {
		//	printf("%02X: %02X\n", result, buffer[result]);
		//}
	} while (!(buffer[0x08] & 0x20) && !(result < 0));

	/* return the data */
	return new_speed;
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net line level.  The values for NewLevel are
// as follows:
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'new_level'  - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 
//
// Returns:  current 1-Wire Net level  
//
int owLevel(int portnum, SMALLINT new_level){
	int result; 
	unsigned char buffer[0x20];
	unsigned int  pulse;
	int transferred;		// number of bytes transferred using libusb_bulk_transfer
	
	switch( new_level )
	{
	  case MODE_NORMAL:
	  	pulse = 0x0000;
	  	break;
	  	
	  case MODE_STRONG5:
	  	pulse = 0x0002;
	  	break;
	  	
	  case MODE_PROGRAM:
	  	pulse = 0x0001;
	  	break;
	  	
	  default:
	  	pulse = 0x0000;
	  	break;
	}

	/* issue the command to enable speed changes */
	result = libusb_control_transfer(usb_dev_handle_list[portnum], 0x40,
			MODE_CMD, MOD_PULSE_EN, pulse, NULL, 0x0, TIMEOUT_VALUE);
	//printf("result is %d\n", result);

	/* repeat until the unit is not idle */
	do {
		/* get the status */
		result = libusb_bulk_transfer(usb_dev_handle_list[portnum], 0x81,
				buffer, 0x20, &transferred, TIMEOUT_VALUE);
		//printf("result is %d\n", result);

		//for (result = 0; result < 0x20; result++) {
		//	printf("%02X: %02X\n", result, buffer[result]);
		//}
	} while (!(buffer[0x08] & 0x20) && !(result < 0));

	/* return the data */
	return (result < 0) ? result : new_level;
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse 
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
//
// Returns:  TRUE  successful
//           FALSE program voltage not available  
//
int owProgramPulse(int portnum)
{
   // Adapter supports it but not implemented yet
   return 0;
}

//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
// 
void msDelay(int len)
{
   struct timespec s;              // Set aside memory space on the stack

   s.tv_sec = len / 1000;
   s.tv_nsec = (len - (s.tv_sec * 1000)) * 1000000;
   nanosleep(&s, NULL);
}

//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
   struct timezone tmzone;
   struct timeval  tmval;
   long ms;

   gettimeofday(&tmval,&tmzone);
   ms = (tmval.tv_sec & 0xFFFF) * 1000 + tmval.tv_usec / 1000;
   return ms;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).  
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same 
//
SMALLINT owWriteBytePower(int portnum, SMALLINT sendbyte)
{
   // replace if platform has better implementation (faster response)
   if (!hasPowerDelivery(portnum))
      return FALSE;
   
   if(owTouchByte(portnum,sendbyte) != sendbyte)
      return FALSE;

   if(owLevel(portnum,MODE_STRONG5) != MODE_STRONG5)
      return FALSE;

   return TRUE;
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// response matches the 'applyPowerResponse' bit and apply power delivery
// to the 1-Wire net.  Note that some implementations may apply the power
// first and then turn it off if the response is incorrect.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'applyPowerResponse' - 1 bit response to check, if correct then start
//                        power delivery 
//
// Returns:  TRUE: bit written and response correct, strong pullup now on
//           FALSE: response incorrect
//
SMALLINT owReadBitPower(int portnum, SMALLINT applyPowerResponse)
{
   // replace if platform has better implementation (faster response)
   if (!hasPowerDelivery(portnum))
      return FALSE;

   if(owTouchBit(portnum,0x01) != applyPowerResponse)
      return FALSE;

   if(owLevel(portnum,MODE_STRONG5) != MODE_STRONG5)
      return FALSE;

   return TRUE;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  if adapter is capable of delivering power. 
//
SMALLINT hasPowerDelivery(int portnum)
{
   // Implemented by bcl@brianlane.com
   return TRUE;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  if adapter is capable of over drive. 
//
SMALLINT hasOverDrive(int portnum)
{
   return TRUE;
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse 
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  program volatage available
//           FALSE program voltage not available  
SMALLINT hasProgramPulse(int portnum)
{
   // Implemented by bcl@brianlane.com
   return TRUE;
}


