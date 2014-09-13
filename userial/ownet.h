//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
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
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//---------------------------------------------------------------------------
//
// ownet.h - Include file for 1-Wire Net library
//
// Version: 2.10
//
// History: 1.02 -> 1.03 Make sure uchar is not defined twice.
//          1.03 -> 2.00 Changed 'MLan' to 'ow'.
//          2.00 -> 2.01 Added error handling. Added circular-include check.
//          2.01 -> 2.10 Added raw memory error handling and SMALLINT
//          2.10 -> 3.00 Added memory bank functionality
//                       Added file I/O operations
//

#ifndef OWNET_H
#define OWNET_H

//--------------------------------------------------------------//
// Target Specific Information
//--------------------------------------------------------------//
//--------------------------------------------------------------//
// Handhelds (PalmOS, WinCE)
//--------------------------------------------------------------//
#ifdef __MC68K__
   //MC68K is the type of processor in the PILOT
   //Metrowerk's CodeWarrior defines this symbol
   #include <string.h>
   #ifndef strcmp
      #include <StringMgr.h>
      #define strcmp StrCompare
   #endif
#endif

#ifdef _WIN32_WCE
   //All of our projects had this flag defined by default (_WIN32_WCE),
   //but I'm not 100% positive that this is _the_ definitive
   //flag to use to identify a WinCE system.
   #include "WinCElnk.h"
#endif

#ifdef _WIN32_WCE
#ifndef FILE
   #define FILE int
   extern int sprintf(char *buffer, char *format,...);
   extern void fprintf(FILE *fp, char *format,...);
   extern void printf(char *format,...);
#endif
#endif
#ifdef __MC68K__
   #include <file_struc.h>
#endif
#if !defined(_WIN32_WCE) && !defined(__MC68K__)
   #include <stdio.h>
#endif

#ifdef __C51__
   #define FILE int
   extern void fprintf (FILE*fileNo,char*s,...);
   #define exit(c) return
   #define key_abort() (FALSE)
   typedef unsigned int ushort;
   typedef unsigned long ulong;
   #define SMALLINT uchar
#endif

#ifdef SOLARIS
#include <sys/types.h>
#include <time.h>
#endif



//--------------------------------------------------------------//
// Common Includes to ownet applications
//--------------------------------------------------------------//
#include <stdlib.h>


//--------------------------------------------------------------//
// Typedefs
//--------------------------------------------------------------//
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

#ifndef OW_UCHAR
   #define OW_UCHAR
   typedef unsigned char  uchar;
   #ifdef __CYGWIN__ //&& GCC
      typedef unsigned long  ulong;
      //ushort already defined in CygWin, in sys/types.h
      #include <sys/types.h>
   #else
      #if defined(FREEBSD) || defined(_WIN32) || defined(WIN32) || defined(__MC68K__) || defined(_WIN32_WCE)
         typedef unsigned short ushort;
         typedef unsigned long  ulong;
      #endif
      #if defined(DARWIN)
         typedef unsigned short ushort;
         typedef unsigned long  ulong;
      #endif
   #endif
   #ifdef SDCC
      //intent of ushort is 2 bytes unsigned.
      //for ds390 in sdcc, an int, not a short,
      //is 2 bytes.
      typedef unsigned int ushort;
   #endif
#endif

// general defines
#define WRITE_FUNCTION 1
#define READ_FUNCTION  0

// error codes
// todo: investigate these and replace with new Error Handling library
#define READ_ERROR    -1
#define INVALID_DIR   -2
#define NO_FILE       -3
#define WRITE_ERROR   -4
#define WRONG_TYPE    -5
#define FILE_TOO_BIG  -6

// Misc
#define FALSE          0
#define TRUE           1

#ifndef MAX_PORTNUM
   #define MAX_PORTNUM    16
#endif

// mode bit flags
#define MODE_NORMAL                    0x00
#define MODE_OVERDRIVE                 0x01
#define MODE_STRONG5                   0x02
#define MODE_PROGRAM                   0x04
#define MODE_BREAK                     0x08

// Output flags
#define LV_ALWAYS          2
#define LV_OPTIONAL        1
#define LV_VERBOSE         0

//--------------------------------------------------------------//
// Error handling
//--------------------------------------------------------------//
extern int owGetErrorNum(void);
extern int owHasErrors(void);

//Clears the stack.
#define OWERROR_CLEAR() while(owHasErrors()) owGetErrorNum();

#ifdef DEBUG
   //Raises an exception with extra debug info
   #define OWERROR(err) owRaiseError(err,__LINE__,__FILE__)
   extern void owRaiseError(int,int,char*);
   #define OWASSERT(s,err,ret) if(!(s)){owRaiseError((err),__LINE__,__FILE__);return (ret);}
#else
   //Raises an exception with just the error code
   #define OWERROR(err) owRaiseError(err)
   extern void owRaiseError(int);
   #define OWASSERT(s,err,ret) if(!(s)){owRaiseError((err));return (ret);}
#endif

#ifdef SMALL_MEMORY_TARGET
   #define OWERROR_DUMP(fileno) /*no-op*/;
#else
   //Prints the stack out to the given file.
   #define OWERROR_DUMP(fileno) while(owHasErrors()) owPrintErrorMsg(fileno);
   extern void owPrintErrorMsg(FILE *);
   extern void owPrintErrorMsgStd();
   extern char *owGetErrorMsg(int);
#endif

#define OWERROR_NO_ERROR_SET                    0
#define OWERROR_NO_DEVICES_ON_NET               1
#define OWERROR_RESET_FAILED                    2
#define OWERROR_SEARCH_ERROR                    3
#define OWERROR_ACCESS_FAILED                   4
#define OWERROR_DS2480_NOT_DETECTED             5
#define OWERROR_DS2480_WRONG_BAUD               6
#define OWERROR_DS2480_BAD_RESPONSE             7
#define OWERROR_OPENCOM_FAILED                  8
#define OWERROR_WRITECOM_FAILED                 9
#define OWERROR_READCOM_FAILED                  10
#define OWERROR_BLOCK_TOO_BIG                   11
#define OWERROR_BLOCK_FAILED                    12
#define OWERROR_PROGRAM_PULSE_FAILED            13
#define OWERROR_PROGRAM_BYTE_FAILED             14
#define OWERROR_WRITE_BYTE_FAILED               15
#define OWERROR_READ_BYTE_FAILED                16
#define OWERROR_WRITE_VERIFY_FAILED             17
#define OWERROR_READ_VERIFY_FAILED              18
#define OWERROR_WRITE_SCRATCHPAD_FAILED         19
#define OWERROR_COPY_SCRATCHPAD_FAILED          20
#define OWERROR_INCORRECT_CRC_LENGTH            21
#define OWERROR_CRC_FAILED                      22
#define OWERROR_GET_SYSTEM_RESOURCE_FAILED      23
#define OWERROR_SYSTEM_RESOURCE_INIT_FAILED     24
#define OWERROR_DATA_TOO_LONG                   25
#define OWERROR_READ_OUT_OF_RANGE               26
#define OWERROR_WRITE_OUT_OF_RANGE              27
#define OWERROR_DEVICE_SELECT_FAIL              28
#define OWERROR_READ_SCRATCHPAD_VERIFY          29
#define OWERROR_COPY_SCRATCHPAD_NOT_FOUND       30
#define OWERROR_ERASE_SCRATCHPAD_NOT_FOUND      31
#define OWERROR_ADDRESS_READ_BACK_FAILED        32
#define OWERROR_EXTRA_INFO_NOT_SUPPORTED        33
#define OWERROR_PG_PACKET_WITHOUT_EXTRA         34
#define OWERROR_PACKET_LENGTH_EXCEEDS_PAGE      35
#define OWERROR_INVALID_PACKET_LENGTH           36
#define OWERROR_NO_PROGRAM_PULSE                37
#define OWERROR_READ_ONLY                       38
#define OWERROR_NOT_GENERAL_PURPOSE             39
#define OWERROR_READ_BACK_INCORRECT             40
#define OWERROR_INVALID_PAGE_NUMBER             41
#define OWERROR_CRC_NOT_SUPPORTED               42
#define OWERROR_CRC_EXTRA_INFO_NOT_SUPPORTED    43
#define OWERROR_READ_BACK_NOT_VALID             44
#define OWERROR_COULD_NOT_LOCK_REDIRECT         45
#define OWERROR_READ_STATUS_NOT_COMPLETE        46
#define OWERROR_PAGE_REDIRECTION_NOT_SUPPORTED  47
#define OWERROR_LOCK_REDIRECTION_NOT_SUPPORTED  48
#define OWERROR_READBACK_EPROM_FAILED           49
#define OWERROR_PAGE_LOCKED                     50
#define OWERROR_LOCKING_REDIRECTED_PAGE_AGAIN   51
#define OWERROR_REDIRECTED_PAGE                 52
#define OWERROR_PAGE_ALREADY_LOCKED             53
#define OWERROR_WRITE_PROTECTED                 54
#define OWERROR_NONMATCHING_MAC                 55
#define OWERROR_WRITE_PROTECT                   56
#define OWERROR_WRITE_PROTECT_SECRET            57
#define OWERROR_COMPUTE_NEXT_SECRET             58
#define OWERROR_LOAD_FIRST_SECRET               59
#define OWERROR_POWER_NOT_AVAILABLE             60
#define OWERROR_XBAD_FILENAME                   61
#define OWERROR_XUNABLE_TO_CREATE_DIR           62
#define OWERROR_REPEAT_FILE                     63
#define OWERROR_DIRECTORY_NOT_EMPTY             64
#define OWERROR_WRONG_TYPE                      65
#define OWERROR_BUFFER_TOO_SMALL                66
#define OWERROR_NOT_WRITE_ONCE                  67
#define OWERROR_FILE_NOT_FOUND                  68
#define OWERROR_OUT_OF_SPACE                    69
#define OWERROR_TOO_LARGE_BITNUM                70
#define OWERROR_NO_PROGRAM_JOB                  71
#define OWERROR_FUNC_NOT_SUP                    72
#define OWERROR_HANDLE_NOT_USED                 73
#define OWERROR_FILE_WRITE_ONLY                 74
#define OWERROR_HANDLE_NOT_AVAIL                75
#define OWERROR_INVALID_DIRECTORY               76
#define OWERROR_HANDLE_NOT_EXIST                77
#define OWERROR_NONMATCHING_SNUM                78
#define OWERROR_NON_PROGRAM_PARTS               79
#define OWERROR_PROGRAM_WRITE_PROTECT           80
#define OWERROR_FILE_READ_ERR                   81
#define OWERROR_ADDFILE_TERMINATED              82
#define OWERROR_READ_MEMORY_PAGE_FAILED         83
#define OWERROR_MATCH_SCRATCHPAD_FAILED         84
#define OWERROR_ERASE_SCRATCHPAD_FAILED         85
#define OWERROR_READ_SCRATCHPAD_FAILED          86
#define OWERROR_SHA_FUNCTION_FAILED             87
#define OWERROR_NO_COMPLETION_BYTE              88
#define OWERROR_WRITE_DATA_PAGE_FAILED          89
#define OWERROR_COPY_SECRET_FAILED              90
#define OWERROR_BIND_SECRET_FAILED              91
#define OWERROR_INSTALL_SECRET_FAILED           92
#define OWERROR_VERIFY_SIG_FAILED               93
#define OWERROR_SIGN_SERVICE_DATA_FAILED        94
#define OWERROR_VERIFY_AUTH_RESPONSE_FAILED     95
#define OWERROR_ANSWER_CHALLENGE_FAILED         96
#define OWERROR_CREATE_CHALLENGE_FAILED         97
#define OWERROR_BAD_SERVICE_DATA                98
#define OWERROR_SERVICE_DATA_NOT_UPDATED        99
#define OWERROR_CATASTROPHIC_SERVICE_FAILURE    100
#define OWERROR_LOAD_FIRST_SECRET_FAILED        101
#define OWERROR_MATCH_SERVICE_SIGNATURE_FAILED  102

#endif //OWNET_H

