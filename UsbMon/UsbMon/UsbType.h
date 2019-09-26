#ifndef __USB_TYPE_HEADER__
#define __USB_TYPE_HEADER__ 

#include <ntddk.h> 
#include "Hidport.h"
#include "usb.h" 
#include "TList.h"
#include "hidpddi.h"


/////////////////////////////////////////////////////////////////////////////////////////
//	Marco 
//
#define MAX_LEN 256
#define SUPPORTED_BUTTON_COUNT 	2

#define MOUSE_FLAGS						 0x1
#define KEYBOARD_FLAGS					 0x2


#define CHANNEL_DUMP_ATTRIBUTE_RELATED					1
#define CHANNEL_DUMP_REPORT_REALTED						2
#define CHANNEL_DUMP_REPORT_BYTE_OFFSET_REALTED			4 
#define CHANNEL_DUMP_RANGE_RELATED						8
#define CHANNEL_DUMP_LINK_COL_RELATED					16
#define CHANNEL_DUMP_ALL  CHANNEL_DUMP_ATTRIBUTE_RELATED | CHANNEL_DUMP_REPORT_REALTED | CHANNEL_DUMP_RANGE_RELATED | CHANNEL_DUMP_LINK_COL_RELATED | CHANNEL_DUMP_REPORT_BYTE_OFFSET_REALTED

////////////////////////////////////////////////////////////////////////////////////////
//  Types
// 
typedef BOOLEAN(*PFOUNDHIDDEVICECALLBACK)(PVOID);

#pragma pack(push, 4)
typedef struct _HID_CONTEXT
{
	PFOUNDHIDDEVICECALLBACK MouseCallback;
	PFOUNDHIDDEVICECALLBACK KeyboardCallback;
	ULONG RequiredDevice;
}HIDCONTEXT, *PHIDCONTEXT;
#pragma pack(push, 4)


#pragma pack(push, 4)
typedef struct _USER_KBDDATA
{
	ULONG  ScanCode;
	ULONG  Flags;
	ULONG Reserved1;
	ULONG Reserved2;
}USERKBDDATA, *PUSERKBDDATA;
#pragma pack(pop)


#pragma pack(push, 4)
typedef struct _USER_MOUDATA
{
	LONG  x;
	LONG  y;
	LONG  z;
	LONG  Click;
	ULONG IsAbsolute;
	ULONG Reserved1;
	ULONG Reserved2;
}USERMOUDATA, *PUSERMOUDATA;;
#pragma pack(pop)


#pragma pack(push, 4)
typedef struct _USB_HUB_LIST
{
	TChainListHeader*					  	 head;
	ULONG							  currentSize;
	ULONG								 RefCount;
} USB_HUB_LIST, *PUSB_HUB_LIST;
#pragma pack(pop)

#pragma pack(push, 4)
typedef struct _HID_DEVICE_LIST
{
	TChainListHeader*					  	 head;
	ULONG							  currentSize;
	ULONG								 RefCount;
} HID_DEVICE_LIST, *PHID_DEVICE_LIST;
#pragma pack(pop)

typedef struct _HID_USB_DEVICE_EXTENSION {
	ULONG64							   status;			//+0x0	
	PUSB_DEVICE_DESCRIPTOR		    DeviceDesc;			//+0x8
	PUSBD_INTERFACE_INFORMATION		InterfaceDesc;		//+0x10
	USBD_CONFIGURATION_HANDLE 		ConfigurationHandle;//+0x18
	ULONG                           NumPendingRequests;
	KEVENT                          AllRequestsCompleteEvent;
	ULONG                           DeviceFlags;
	PIO_WORKITEM                    ResetWorkItem;
	HID_DESCRIPTOR				    HidDescriptor;
	PDEVICE_OBJECT                  functionalDeviceObject;
}HID_USB_DEVICE_EXTENSION, *PHID_USB_DEVICE_EXTENSION;

typedef struct _COORDINATE_DESCRIPTOR
{
	USHORT ByteOffset;
	USHORT BitOffset;
	USHORT OffsetSize;
}COORDESC, *PCOORDESC;

typedef struct _MOUSE_BTN_DESCRIPTOR
{
	USHORT ByteOffsetBtn[SUPPORTED_BUTTON_COUNT];
	USHORT BitOffsetBtn[SUPPORTED_BUTTON_COUNT];
	USHORT BtnOffsetSize[SUPPORTED_BUTTON_COUNT];
	USHORT BtnUsageMin[SUPPORTED_BUTTON_COUNT];
}BTNDESC, *PBTHESC;

typedef struct _REPORT_ID_DESCRIPTOR
{
	USHORT ReportId;
	USHORT Usage;
	USHORT UsageIndex;
}REPORTIDDESC, *PREPORTIDDESC;


typedef struct _EXTRACT_DATA
{
	union
	{
		struct
		{
			COORDESC X_Descriptor;
			COORDESC Y_Descriptor;
			COORDESC Z_Descriptor;
			BTNDESC	 BtnDescriptor;
			ULONG  	 IsAbsolute;
			REPORTIDDESC 	ReportIdDesc[4];
		}MOUDATA;

		struct
		{
			USHORT  SpecialKeyByteOffset;
			USHORT	SpecialKeyBitOffset;
			USHORT  SpecialKeySize;
			USHORT  NormalKeyByteOffset;
			USHORT	NormalKeyBitOffset;
			USHORT  NormalKeySize;
			USHORT  LedKeyByteOffset;
			USHORT	LedKeyBitOffset;
			USHORT  LedKeySize;
			USHORT  ReportId;
		}KBDDATA;

	};

}EXTRACTDATA, *PEXTRACTDATA;


#pragma pack(push, 4)
typedef struct USBHUBNODE1
{
	PDRIVER_OBJECT HubDriverObject;
	WCHAR 		   HubName[MAX_LEN];
	ULONG	 	   HubNameLen;
	BOOLEAN		   IsHooked;
}USBHUBNODE, *PUSBHUBNODE;
#pragma pack(pop)


#pragma pack(push, 4)
typedef struct _HID_DEVICE_NODE
{
	PDEVICE_OBJECT					device_object;
	HID_USB_DEVICE_EXTENSION*		mini_extension;
	HIDP_DEVICE_DESC				parsedReport;
	EXTRACTDATA* 					ExtractedData[3];
	PHIDP_COLLECTION_DESC 			Collection;
	LONG							LastClickStatus;
	//keyboard
	UCHAR* 							PreviousUsageList;
	UCHAR* 							CurrentUsageList;
	UCHAR* 							MakeUsageList;
	UCHAR* 							BreakUsageList;
	HIDP_KEYBOARD_MODIFIER_STATE	ModifierState;
}HID_DEVICE_NODE, *PHID_DEVICE_NODE;
#pragma pack(pop)


#pragma pack(push, 4)
typedef struct _USER_CONFIG_EX
{
	ULONG 	  Version;
	ULONG	  ProtocolVersion;
	ULONGLONG UserData;
	ULONG	  UserDataLen;
	ULONG 	  Flags;
}USERCONFIGEX, *PUSERCONFIGEX;
#pragma pack(pop)

//--------------------------------------//
#endif