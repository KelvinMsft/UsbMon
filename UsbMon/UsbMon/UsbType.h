#ifndef __USB_TYPE_HEADER__
#define __USB_TYPE_HEADER__ 

#include <fltKernel.h> 
#include "Hidport.h"
#include "usb.h" 
#include "hubbusif.h"
#include "usbbusif.h" 
#include "Usbioctl.h"

typedef struct _HID_DEVICE_NODE
{
	PDEVICE_OBJECT					device_object;
	PUSBD_INTERFACE_INFORMATION		InterfaceDesc;
}HID_DEVICE_NODE, *PHID_DEVICE_NODE;

typedef struct _HID_USB_DEVICE_EXTENSION {
	ULONG64							   status;			//+0x0	
	PUSB_DEVICE_DESCRIPTOR		    DeviceDesc;			//+0x8
	PUSBD_INTERFACE_INFORMATION		InterfaceDesc;		//+0x10
	USBD_CONFIGURATION_HANDLE 		ConfigurationHandle;//+0x18
	char							reserved[0x37];
	HID_DESCRIPTOR*					descriptorList;		//+0x57
}HID_USB_DEVICE_EXTENSION, *PHID_USB_DEVICE_EXTENSION;
static_assert(sizeof(HID_USB_DEVICE_EXTENSION) == 0x60, "Size Check");

#endif