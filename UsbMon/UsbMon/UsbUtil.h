#ifndef __USB_UTIL_HEADER__
#define __USB_UTIL_HEADER__

#include <ntddk.h> 
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
/* -----------------------------------------------------------------------------------------------------------------
*/
PHIDP_REPORT_IDS GetReportIdentifier(	
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc, 
	_In_ ULONG 				  reportId
);

/* -----------------------------------------------------------------------------------------------------------------
*/
PHIDP_COLLECTION_DESC GetCollectionDesc(	
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_In_ ULONG 				collectionId
);



 /*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

	- VerifyUsageInCollection gets mouse usage collection in collections array
	
	- One Interface (End point) can has more than one Collection
	
	- It get the collection corresponding to the reportId

Arguments:
	 Usage		   - Usage ID you are looking for. such as, keyboard / mouse.
	 
	 pSourceReport - The pointer of raw usb data report

	 pDesc		   - The pointer of HIDP_DEVICE_DESC struct which included all reportId 
					 the reportId struct has an unique collection number, it can be used
					 for finding the collection by enumerating the collections array.

	 pCollection   - The pointer of output collection
	 
Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
			 - STATUS_XXXXXXXXXXXX   , Depended on old completion
-----------------------------------------------------------------------------------*/  
NTSTATUS VerifyUsageInCollection( 
	_In_ ULONG					 Usage,
	_In_ UCHAR*					 pSourceReport, 
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_Out_ PHIDP_COLLECTION_DESC* pCollection
);

 
 /*----------------------------------------------------
Description: 

	//Little-endian : 
	//Src[0] Src[1] Src[2] Src[3]
	//  0x11   0x22   0x33   0x44
	
	//Memory Order:
	//0x00000000 - 0x44 0x33 0x22 0x11
	
	//For example:
	//  X ByteOffset = 0  , Y ByteOffset = 1
	//  X BitOffset  = 0  , Y BitOffset  = 4	; BitOffset means the data read should be start from this bit in a byte
	//	X BitSize    = 12 , Y BitSize    = 12
	//  
	//
	//			 Shared bit in a byte
	//			  ----------- 	-----------   -----------
	//usb data - | 1111 1111 | | 1111 1111 | | 1111 1111 |	
	//			  -----------   -----------   -----------
	//Shared Bit: 	 		     ^^^^ ^^^^ 
				  |-----------------| |-----------------|
				  
	//For dealing with this problem, we copied two byte first, and shift

Arguments:
	Src 			- 		UsbHid Data Source
	ByteOffset  	- 		which byte we start start to read
	BitOffset   	- 		which bit in byte we start to read
	BitLen			- 		how many bits we read 
	
Return Value:			
	LONG 			- 		Read data  
	
------------------------------------------------------*/
LONG ReadHidCoordinate(
	_In_	PCHAR Src , 
	_In_	ULONG ByteOffset, 
	_In_	ULONG BitOffset,  
	_In_	ULONG BitLen);
   
#endif