#ifndef __REPORT_UTIL_HEARER__
#define __REPORT_UTIL_HEADER__

#include <fltKernel.h>
#include "CommonUtil.h"
#include "UsbType.h"

#define DUMP_INPUT_REPORT_FLAG   1
#define DUMP_OUTPUT_REPORT_FLAG  2
#define DUMP_FEATURE_REPORT_FLAG 4  

//usage page-id
#define HID_GENERIC_DESKTOP_PAGE 0x1

//There is a sub-usage id
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

VOID 
DumpReport(
	HIDP_DEVICE_DESC* report
);

VOID 
DumpChannel(
	PHIDP_COLLECTION_DESC collectionDesc, 
	HIDP_REPORT_TYPE type, 
	ULONG Flags
);

#endif