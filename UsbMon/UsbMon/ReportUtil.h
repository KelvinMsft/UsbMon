#ifndef __REPORT_UTIL_HEARER__
#define __REPORT_UTIL_HEADER__

#include <fltKernel.h>
#include "CommonUtil.h"
#include "UsbType.h"

#define DUMP_INPUT_REPORT_FLAG   1
#define DUMP_OUTPUT_REPORT_FLAG  2
#define DUMP_FEATURE_REPORT_FLAG 4  

//usage page-id
//-------------------------------------------// 
#define HID_GENERIC_DESKTOP_PAGE	0x1
#define HID_SIMULATION_CONTROL		0x2
#define HID_VR_CONTROL				0x3 
#define HID_SPORT_CONTROL			0x4
#define HID_GAME_CONTROL			0x5
#define HID_GENERIC_DEVICE_CONTROL	0x6
#define HID_KEYBOARD_OR_KEYPAD		0x7
#define HID_LEDS					0x8
#define HID_BUTTON					0x9
#define HID_ORIDINAL				0xA
#define HID_TELEPHONY				0xB
#define HID_CONSUMER				0xC
#define HID_DIGITILZER				0xD
#define HID_RESERVED				0xE
#define HID_PID_PAGE				0xF
#define HID_UNICODE					0x10
#define HID_RESERVED1				0x11
#define HID_RESERVED2				0x12
#define HID_RESERVED3				0x13
#define HID_ALPHA_DISPLAY			0x14 
#define HID_MEDICAL_INSTR			0x40
#define HID_MONITOR_PAGES			0x80
#define HID_MONITOR_PAGES1			0x81
#define HID_MONITOR_PAGES2			0x82 
#define HID_MONITOR_PAGES3			0x83 
#define HID_POWER_PAGES1			0x84
#define HID_POWER_PAGES2			0x85
#define HID_POWER_PAGES3			0x86
#define HID_POWER_PAGES4			0x87 
#define HID_BARCODE_SCANNER			0x8C
#define HID_SCALE_PAGE				0x8D 
#define HID_RESEVRED_POS_PAGE		0x8F
#define HID_CAMERA_PAGE				0x90
//-------------------------------------------//


//There is a sub-usage id For HID_GENERIC_DESKTOP_PAGE
//-------------------------------------------//
#define HID_POINTER_USAGE	       0x1
#define HID_MOU_USAGE		       0x2
#define HID_RESERVED_USAGE	       0x3
#define HID_JOYSTICK_USAGE	       0x4
#define HID_GAMEPAD_USAGE	       0x5  
#define HID_KBD_USAGE		       0x6
#define HID_KBYPAD_USAGE	       0x7
#define HID_LED_USAGE		       0x8
#define HID_MULTI_AXIS_USAGE	   0x9 

//0xA-0x2f reserved 
#define HID_NOT_RANGE_USAGE_X	  0x30
#define HID_NOT_RANGE_USAGE_Y	  0x31
#define HID_NOT_RANGE_USAGE_Z	  0x32 

#define HID_NOT_RANGE_USAGE_RX	  0x33
#define HID_NOT_RANGE_USAGE_RY	  0x34
#define HID_NOT_RANGE_USAGE_RZ	  0x35 

#define HID_USAGE_SLIDER		  0x36 
#define HID_USAGE_DIAL			  0x37 
#define HID_NOT_RANGE_USAGE_WHELL 0x38  
//-------------------------------------------//

NTSTATUS
ExtractKeyboardData(
	_In_	 PHIDP_COLLECTION_DESC collectionDesc,
	_In_	 HIDP_REPORT_TYPE type,
	_Inout_  EXTRACTDATA* ExtractedData
);

NTSTATUS
ExtractMouseData(
	PHIDP_COLLECTION_DESC collectionDesc,
	HIDP_REPORT_TYPE type,
	EXTRACTDATA* ExtractedData
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

			DumpReport
			 
Arguments:
			report	- structure of HIDP_DEVICE_DESC, it saved all report, 
					  we dump it all

Return Value: 
			No 
-----------------------------------------------------------------------------------*/
VOID 
DumpReport(
	HIDP_DEVICE_DESC* report
);





/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

			DumpChannel


Arguments:
			collectionDesc - Collection Description , it saved all channel. a channel
							 respect to the Usage in report descriptor 

			type		   - what report type of channel to be dumped

			Flags		   - which part of channel need to be dumped

Return Value: 
			No

-----------------------------------------------------------------------------------*/
VOID 
DumpChannel(
	PHIDP_COLLECTION_DESC collectionDesc, 
	HIDP_REPORT_TYPE type, 
	ULONG Flags
);

#endif