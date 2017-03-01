#ifndef __USB_UTIL_HEADER__
#define __USB_UTIL_HEADER__ 
#include <fltKernel.h> 
#include "UsbType.h"
#include "CommonUtil.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//// Type
//// 
typedef enum
{
	USB = 0,
	USB2,
	USB3,
	USB3_NEW,
	USB_COMPOSITE,
}USB_HUB_VERSION;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 
/* -----------------------------------------------------------------------------------------------------------------


*/
VOID DumpUrb(
	PURB urb
);

/* -----------------------------------------------------------------------------------------------------------------
	Parameter 1: hid_common_extension 
*/

VOID DumpHidMiniDriverExtension(
	HID_DEVICE_EXTENSION* hid_common_extension
);

/* -----------------------------------------------------------------------------------------------------------------
 

*/
NTSTATUS GetUsbHub(
	USB_HUB_VERSION usb_hub_version,
	PDRIVER_OBJECT* pDriverObj
);

/* -----------------------------------------------------------------------------------------------------------------
 

*/
WCHAR* GetUsbHubDriverNameByVersion(
	USB_HUB_VERSION usb_hub_version
);

#endif