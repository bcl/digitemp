//---------------------------------------------------------------------------
// usbses.c
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
//  usb_sessuin.c - General 1-Wire session layer for the DS2490 on Linux 
//
//  Version: 3.00B
// 
/*	DRJ Modifications:
 |	2013-07-28
 |		moved from libusb0.1 to libusb1.0 in the hopes of more reliability
 |	70	usb_dev_handle		->	libusb_device_handle
 |	87	usb_init					->	libusb_init
 |	88	usb_set_debug			->	libusb_set_debug
 |	89	usb_find_busses		->	
 |	90	usb_find_devices	->	
 |		Oh well!  same problems as before.
 |		The problems ended up being with owTouchReset()
 */

#define DEBUG_USB 1
#define DEBUG_TIME 0


#include "ownet.h"
#include <libusb-1.0/libusb.h>
#include <string.h>
#include <sys/time.h>	//for struct timeval
#include "digitemp.h"
#include "DRJ_digitemp.h"


SMALLINT owAcquire(int,char *, char *);
void owRelease(int,char *);
static int usb_ds2490_init(void);

struct libusb_device_handle *usb_dev_handle_list[MAX_PORTNUM];
struct libusb_device *usb_dev_list[MAX_PORTNUM];


int usb_num_devices = -1;
int initted_flag = 0;

extern int opts;            // Command line options


//
// Initialize the DS2490
//

int usb_ds2490_init(void) {
	libusb_device **devs;
	libusb_device *dev;
	
	int r;
	ssize_t cnt;
	int i = 0;

	// initialize USB subsystem
	r = libusb_init(NULL);		// using default context
	if (r < 0){
		fprintf(stderr,"ERROR:usb_ds2490_init() Failed to initialize the USB system\n");
		return r;
	}
	libusb_set_debug(NULL, 3);		// LIBUSB_LOG_LEVEL_INFO = 3, but not defined in libusb-1.0-dev (1.0.8-r4)?
	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0) {
		fprintf(stderr,"ERROR: usb_ds2490_init() Failed to get device list.\n");
		return (int) cnt;
	}
	// step trough each of the devices found by libusb_get_device_list, looking for a device that we might care about
	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "ERROR: usb_ds2490_init() Failed to get device descriptor");
			return r;
		}
		if (desc.idVendor == 0x04FA && desc.idProduct == 0x2490) {
			++usb_num_devices;
			usb_dev_list[usb_num_devices] = dev;
			if( !(opts & OPT_QUIET) ) {
				int bus_no, device_address;
				bus_no = libusb_get_bus_number(dev);
				device_address = libusb_get_device_address(dev);
				printf("Found DS2490 device #%d at %d/%d\n", usb_num_devices + 1,bus_no,device_address);
			}
		}
	}

	initted_flag = 1;
	return 0;
}

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'port_zstr'  - zero terminated port name.  
// 'return_msg' - zero terminated return message. 
//
// Returns: TRUE - success, port opened
//
SMALLINT owAcquire(int portnum, char *port_zstr, char *return_msg){
	int retValue;
	if (!initted_flag)
		usb_ds2490_init();
	
	// check the port string
	if (strcmp (port_zstr, "USB")) {
		strcpy(return_msg, "owAcquire called with invalid port string\n");
	  return 0;
	}
	
	// check to see if the portnumber is valid
	if (usb_num_devices < portnum) {
		strcpy(return_msg, "Attempted to select invalid port number\n");
		return FALSE;
	}

	// check to see if opening the device is valid
	if (usb_dev_handle_list[portnum] != NULL) {
		strcpy(return_msg, "Device allready open\n");
		return FALSE;
	}

	// open the device
	retValue = libusb_open(usb_dev_list[portnum], usb_dev_handle_list);		// is usb_dev_handle_list populated with all device handles?

	// if the kernel driver is attached, detach it
  if(libusb_kernel_driver_active(usb_dev_handle_list[portnum], 0) == 1) { //find out if kernel driver is attached
		printf("Kernel Driver Active\n");
    if(libusb_detach_kernel_driver(usb_dev_handle_list[portnum], 0) == 0) //detach it
    {
			printf("Kernel Driver Has been succesfully detatched\n");
	  }
	  else
	  {
	  	fprintf(stderr,"ERR: owAcquire() Could not detatch kernel driver.\n");
	  	return FALSE;
	  }
	}

	// set the configuration
	retValue = libusb_set_configuration(usb_dev_handle_list[portnum], 1);
	if (retValue != 0) {
		strcpy(return_msg, "Failed to set configuration\n");
		fprintf(stderr,"ERROR: usbses.c owAcquire() Failed to set configuration, retValue=%i\n",retValue);
		libusb_close(usb_dev_handle_list[portnum]);
		return FALSE;
	}

	// claim the interface
	retValue = libusb_claim_interface(usb_dev_handle_list[portnum], 0);
	if (retValue) {							//why is the interface number = 0?
		strcpy(return_msg, "Failed to claim interface\n");
		fprintf(stderr,"ERROR: usbses.c owAcquire() Failed to claim interface.\n");

		libusb_close(usb_dev_handle_list[portnum]);
		return FALSE;
	}

	// set the alt interface
	retValue = libusb_set_interface_alt_setting(usb_dev_handle_list[portnum], 0, 3);	//why is the interface number = 0?
	if(retValue) {
		strcpy(return_msg, "Failed to set altinterface\n");
		fprintf(stderr,"ERROR: usbses.c owAcquire() Failed to set altinterface.\n");
		libusb_release_interface(usb_dev_handle_list[portnum], 0);								//why is the interface number = 0?
		libusb_close(usb_dev_handle_list[portnum]);
		return FALSE;
	}

	// we're all set here!
	strcpy(return_msg, "DS2490 successfully acquired by USB driver\n");
	return TRUE;
}	// owAcquire()

//---------------------------------------------------------------------------
// Release the previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number is provided to
//                indicate the symbolic port number.
// 'return_msg' - zero terminated return message. 
//
void owRelease(int portnum, char *return_msg){
	libusb_release_interface(usb_dev_handle_list[portnum], 0);
	libusb_close(usb_dev_handle_list[portnum]);
	usb_dev_handle_list[portnum] = NULL;
	strcpy(return_msg, "DS2490 successfully released by USB driver.\n");
}


