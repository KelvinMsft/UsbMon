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
	_In_ PURB urb
);

/* -----------------------------------------------------------------------------------------------------------------
*/

VOID DumpHidMiniDriverExtension(
	_In_ HID_DEVICE_EXTENSION* hid_common_extension
);

/* -----------------------------------------------------------------------------------------------------------------
*/
NTSTATUS GetUsbHub(
	_In_  USB_HUB_VERSION usb_hub_version,
	_Out_ PDRIVER_OBJECT* pDriverObj
);

/* -----------------------------------------------------------------------------------------------------------------
*/
WCHAR* GetUsbHubDriverNameByVersion(
	_In_ USB_HUB_VERSION usb_hub_version
);

#endif