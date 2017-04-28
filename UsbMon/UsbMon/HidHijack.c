 
#include <fltKernel.h>
#include "UsbUtil.h" 
#include "UsbHid.h" 
#include "IrpHook.h"
#include "Usbioctl.h"
#include "ReportUtil.h" 
#include "OpenLoopBuffer.h" 

#pragma warning (disable : 4100) 


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Types
////  

typedef struct HIJACK_CONTEXT
{
	PDEVICE_OBJECT		   DeviceObject;		// Hid deivce
	PURB							urb;		// Urb packet saved in IRP hook
	HID_DEVICE_NODE*			   node;		// An old context we need 
	PENDINGIRP*			    pending_irp;		// pending irp node for cancellation 
}HIJACK_CONTEXT, *PHIJACK_CONTEXT;

/*
multi-process struct
*/
typedef struct _USERPROCESS_INFO
{
	PEPROCESS proc;
	PVOID     mapped_user_addr;
	PMDL      mdl;
}USERPROCESSINFO, *PUSERPROCESSINFO;


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
}USERMOUDATA, *PUSERDATA;
#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////////////////////
////	Marco Utilities
////  
//// 	 

#define IsSafeNode(x)					 (x->node)
//Hijack Context Marco

//urb Marco
#define UrbGetFunction(urb)				 (urb->UrbHeader.Function)
#define UrbGetTransferBuffer(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferBuffer)
#define UrbGetTransferFlags(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferFlags)
#define UrbGetTransferLength(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferBufferLength) 
#define UrbGetTransferPipeHandle(urb)	 (urb->UrbBulkOrInterruptTransfer.PipeHandle)
#define UrbIsInputTransfer(urb)			 (UrbGetTransferFlags(urb) & (USBD_TRANSFER_DIRECTION_IN ))
#define UrbIsOutputTransfer(urb)		 (UrbGetTransferFlags(urb) & (USBD_TRANSFER_DIRECTION_OUT | USBD_DEFAULT_PIPE_TRANSFER))

#define USB_PEN_INIT_VERSION			 0x00000001
#define USB_PEN_PROTOCOL_VERSION		 0x00000001

#define IS_REPORT_RAW_DESC 				 0x94170A9F	 //Hash(IsReportRawDesc):94170A9F,2484538015,-1810429281
#define USB_PENERATE_CONFIG 			 0x2443D155	 //Hash(UsbPenerate):2443D155,608424277,608424277
#define USB_MOUDATA_ABS_FLAGS 			 0x80000000
#define CB_SIZE 					 			128  
#define REPORT_MAX_COUNT 						  7
#define MAX_COORDINATE_ERROR					160

///////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////
////
DRIVER_DISPATCH*  g_HidPnpHandler = NULL;
DRIVER_DISPATCH*  g_UsbHub2InternalDeviceControl = NULL;
DRIVER_DISPATCH*  g_UsbHub3InternalDeviceControl = NULL;
BOOLEAN			  g_bLoaded = FALSE;
BOOLEAN 		  g_bUnloaded = FALSE;
TChainListHeader* g_ProcessInfo = NULL;
ULONG 			  g_ProcessCount = 0;
CIRCULARBUFFER*	  g_mou_data = NULL;
PENDINGIRPLIST*   g_Usb2PendingIrpList = NULL;
PENDINGIRPLIST*   g_Usb3PendingIrpList = NULL;
ULONG			  g_Usb3Count = 0;
BOOLEAN			  g_IsBugReported = FALSE;
BOOLEAN			  g_IsTestReported = FALSE;
BOOLEAN			  g_IsReportRawDesc = FALSE;
extern PHID_DEVICE_LIST g_HidClientPdoList;



/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 
////


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

Free Pending IRP list memory and each node

Arguments:

No

Return Value:
NTSTATUS	- STATUS_SUCCESS	  if Free Memory successfully
- STATUS_UNSUCCESSFUL if failed

-------------------------------------------------------*/
NTSTATUS		FreePendingIrpList();


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

Handling And Extract data from keyboard URB

Arguments:
pContext	- Hijack Context included all we need
see HIJACK_CONTEXT declaration.

reportType	- Hid Report Type
-	HidP_Input,
-	HidP_Output,
-	HidP_Feature

Return Value:
NTSTATUS	- STATUS_SUCCESS 		if Handle successfully
- STATUS_UNSUCCESSFUL 	if failed
-------------------------------------------------------*/
NTSTATUS HandleKeyboardData(
	_In_ HIJACK_CONTEXT* pContext,
	_In_ HIDP_REPORT_TYPE reportType
);


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

Handling And Extract data from mouse URB

Arguments:

pContext	- Hijack Context included all we need ,
see HIJACK_CONTEXT declaration.

reportType  -	Hid Report Type
-	HidP_Input,
-	HidP_Output,
-	HidP_Feature

Return Value:
NTSTATUS	- STATUS_SUCCESS	  if Handle successfully
- STATUS_UNSUCCESSFUL if failed
-------------------------------------------------------*/
NTSTATUS HandleMouseData(
	_In_ HIJACK_CONTEXT* pContext,
	_In_ HIDP_REPORT_TYPE reportType
);


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Proxy Function of USB Hub 2 / 3 of Internal Control Irp

- It intercept all internal IOCTL ,

I.e. IOCTL_INTERNAL_USB_SUBMIT_URB -> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER

1. Check is it mouse / keyboard

2. Save a IRP Context (included old IRP completion function and context)

3. Hook Completion of IRP

4. Insert into Pending IRP List

Arguments:

DeviceObject 		- USB Hub Device Object, iusb3hub , usbhub ,... etc

Irp			 		- IRP come from upper level

pPendingList 		- pre-allocated pending list , each hubs use private list for
storing pendign IRP

pCompletionRoutine  - IRP Completion Function Address

pOldFunction		- CALL Old Function

Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on IRP Handler

-----------------------------------------------------------------------------------*/
NTSTATUS UsbhubIrpHandler(
	_Inout_ struct _DEVICE_OBJECT*	pDeviceObject,
	_Inout_ struct _IRP*			pIrp,
	_In_     PENDINGIRPLIST* 	   	pPendingList,
	_In_ 	 PIO_COMPLETION_ROUTINE pCompletionRoutine,
	_In_	 DRIVER_DISPATCH*		pOldFunction
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Proxy Function of USB Hub IRP Completion Function (i.e. For URB IRP Request)
- It intercept all completion of Usbhub internal IOCTL ,

I.e. IOCTL_INTERNAL_USB_SUBMIT_URB -> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER

1. Get its Collection first (one report descrirptor can own more than one
collection description) it distinct by reportID, if reportID = 0 ,
it means only one collection, vice versa.

2. Dispatch the data to the UserMode Mapped buffer (if it does exist)

Arguments:

DeviceObject 		- USB Hub Device Object, iusb3hub , usbhub ,... etc

Irp			 		- IRP that is going to be completed

Context 			- IRP Context , we modifiy the Completion context and saved
old context in new context

pPendingList  		- The pending IRP list we need to removed ( usb2 / usb3 uses
independent list. )


Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on IRP Handler

-----------------------------------------------------------------------------------*/
NTSTATUS UsbIrpCompletionHandler(
	_In_     PDEVICE_OBJECT  	   DeviceObject,
	_In_     PIRP            	   Irp,
	_In_	 PVOID           	   Context,
	_In_     PENDINGIRPLIST* 	   pPendingList
);


//See HidHijack.h for detail
NTSTATUS InitializeHidPenetrate();

//See HidHijack.h for detail
NTSTATUS UnInitializeHidPenetrate();


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
	_In_	PCHAR Src,
	_In_	ULONG ByteOffset,
	_In_	ULONG BitOffset,
	_In_	ULONG BitLen);


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Get Client Physical Device Object Extension by HIDCLASS EXTENSION

Arguments:

HidCommonExt - Device Extension of HIDCLASS

Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on old completion
-----------------------------------------------------------------------------------*/
PDO_EXTENSION*	GetClientPdoExtension(
	_In_ HIDCLASS_DEVICE_EXTENSION* HidCommonExt
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Get Functional Device Object Extension by Client PDO Extension

Arguments:

pdoExt - Device Extension of Client PDO

Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on old completion
-----------------------------------------------------------------------------------*/
FDO_EXTENSION* GetFdoExtByClientPdoExt(
	_In_ PDO_EXTENSION* pdoExt
);



/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- VerifyMouseUsageInCollection gets mouse usage collection in collections array

- One Interface (End point) can has more than one Collection

- It get the collection corresponding to the reportId

Arguments:

pSourceReport - The pointer of raw usb data report

pDesc		   - The pointer of HIDP_DEVICE_DESC struct which included all reportId
the reportId struct has an unique collection number, it can be used
for finding the collection by enumerating the collections array.

pCollection   - The pointer of output collection

Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on old completion
-----------------------------------------------------------------------------------*/
NTSTATUS VerifyMouseUsageInCollection(
	_In_ UCHAR*					 pSourceReport,
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_Out_ PHIDP_COLLECTION_DESC* pCollection
);


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Pnp Irp hook handler of UsbHid handler

- it verify the device first

- then insert or remove the node into the pipe list

- usbhub irp hook will next compared with it new device

Arguments:

DeviceObject - Client PDO we need

Irp		  - Irp request.

Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on old completion

-----------------------------------------------------------------------------------*/
NTSTATUS HidUsbPnpIrpHandler(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
);

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
//// 
////  





//----------------------------------------------------------------------------------------//
NTSTATUS HandleKeyboardData(
	_In_ HIJACK_CONTEXT* pContext,
	_In_ HIDP_REPORT_TYPE reportType
)
{
	return STATUS_UNSUCCESSFUL;
}

//----------------------------------------------------------------------------------------//

LONG ReadHidCoordinate(
	_In_	PCHAR Src,
	_In_	ULONG ByteOffset,
	_In_	ULONG BitOffset,
	_In_	ULONG BitLen)
{
	LONG64    Ret = 0;
	UCHAR*    lpret = (UCHAR*)&Ret;
	long      higthNullbitlen = 64 - (BitLen + BitOffset);

	if (!Src)
	{
		return 0;
	}

	memcpy(lpret, Src + ByteOffset, (BitLen + BitOffset + 7) / 8);

	Ret = (Ret << higthNullbitlen);

	if (Ret & 0x8000000000000000)
	{
		ULONGLONG tmp = -1;
		Ret = (Ret >> (higthNullbitlen + BitOffset));
		Ret = Ret | (tmp << BitLen);
	}
	else
	{
		Ret = (Ret >> (higthNullbitlen + BitOffset));
	}

	return (long)Ret;
}

//-----------------------------------------------------------------------------------//
PHIDP_REPORT_IDS GetReportIdentifier(
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_In_ ULONG 				  reportId
)
{
	PHIDP_REPORT_IDS result = NULL;
	PHIDP_DEVICE_DESC deviceDesc = pDesc;
	ULONG i;
	if (!deviceDesc)
	{
		return result;
	}
	if (!deviceDesc->ReportIDs)
	{
		return result;
	}


	for (i = 0; i < deviceDesc->ReportIDsLength; i++)
	{
		if (deviceDesc->ReportIDs[i].ReportID == reportId)
		{
			result = &deviceDesc->ReportIDs[i];
			break;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------------//
PHIDP_COLLECTION_DESC GetCollectionDesc(
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_In_ ULONG 				collectionId
)
{
	PHIDP_COLLECTION_DESC result = NULL;
	PHIDP_DEVICE_DESC deviceDesc = pDesc;
	ULONG i;

	if (deviceDesc->CollectionDesc) {
		for (i = 0; i < deviceDesc->CollectionDescLength; i++) {
			if (deviceDesc->CollectionDesc[i].CollectionNumber == collectionId) {
				result = &deviceDesc->CollectionDesc[i];
				break;
			}
		}
	}
	return result;
}

//--------------------------------------------------------------------------------------------------------------------------------//
NTSTATUS VerifyMouseUsageInCollection(
	_In_ UCHAR*					 pSourceReport,
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_Out_ PHIDP_COLLECTION_DESC* pCollection
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG 	colIndex = 0;
	UCHAR ReportId = 0;
	PHIDP_REPORT_IDS 		reportIdentifier;
	PHIDP_COLLECTION_DESC	collectionDesc;

	if (!pSourceReport || !pDesc || !pCollection)
	{
		USB_DEBUG_INFO_LN_EX("pSourceReport: %I64x fdoExtension: %I64x pDesc: %I64x pCollection: %I64x",
			pSourceReport,
			pDesc,
			pCollection
		);

		if (pCollection)
		{
			*pCollection = NULL;
		}

		return status;
	}

	if (pDesc->ReportIDs[0].ReportID == 0)
	{
		ReportId = 0;
	}
	else
	{
		ReportId = (ULONG)*pSourceReport;
	}

	USB_DEBUG_INFO_LN_EX("Report ID: %x", ReportId);

	reportIdentifier = GetReportIdentifier(pDesc, ReportId);

	USB_DEBUG_INFO_LN_EX("Report Identifier: %I64x ", reportIdentifier);

	if (!reportIdentifier)
	{
		USB_DEBUG_INFO_LN_EX("reportIdentifier: %I64x", reportIdentifier);

		if (!g_IsTestReported)
		{
			
			g_IsTestReported = TRUE;
		}
		*pCollection = NULL;
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	collectionDesc = GetCollectionDesc(pDesc, reportIdentifier->CollectionNumber);

	USB_DEBUG_INFO_LN_EX("CollectionDesc: %I64x  reportIdentifier->CollectionNumber: %x ", collectionDesc, reportIdentifier->CollectionNumber);

	if (!collectionDesc)
	{
		USB_DEBUG_INFO_LN_EX("collectionDesc: %I64x", collectionDesc);

		if (!g_IsTestReported)
		{
			g_IsTestReported = TRUE;
		}
		*pCollection = NULL;
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (collectionDesc->Usage == HID_MOU_USAGE)
	{
		if (!g_IsTestReported)
		{
			USB_DEBUG_INFO_LN_EX("It is mou collection");
			g_IsTestReported = TRUE;
		}

		*pCollection = collectionDesc;
		status = STATUS_SUCCESS;
	}
	else
	{
		if (!g_IsTestReported)
		{
			g_IsTestReported = TRUE;
		}
		*pCollection = NULL;
		status = STATUS_UNSUCCESSFUL;
	}
	return status;
}

//------------------------------------------------------------------------------------------//
PDO_EXTENSION*	GetClientPdoExtension(
	_In_ HIDCLASS_DEVICE_EXTENSION* HidCommonExt
)
{
	PDO_EXTENSION* pdoExt = NULL;
	if (!HidCommonExt)
	{
		return pdoExt;
	}
	if (!HidCommonExt->isClientPdo)
	{
		return pdoExt;
	}
	pdoExt = &HidCommonExt->pdoExt;
	return pdoExt;
}

//-----------------------------------------------------------------------------------------//
FDO_EXTENSION* GetFdoExtByClientPdoExt(
	_In_ PDO_EXTENSION* pdoExt
)
{
	FDO_EXTENSION* fdoExt = NULL;
	if (!pdoExt)
	{
		return fdoExt;
	}
	if (!pdoExt->deviceFdoExt)
	{
		return fdoExt;
	}

	fdoExt = &pdoExt->deviceFdoExt->fdoExt;
	return fdoExt;
}

//----------------------------------------------------------------------------------------//
NTSTATUS HandleMouseData(
	_In_ HIJACK_CONTEXT*    pContext,
	_In_ HIDP_REPORT_TYPE reportType
)
{
	FDO_EXTENSION*				fdoExt = NULL;
	PDO_EXTENSION* 				pdoExt = NULL;
	HIDCLASS_DEVICE_EXTENSION*  HidCommonExt = NULL;
	EXTRACTDATA*				data = { 0 };
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	ULONG						colIndex = 0;
	ULONG						totalSize = 0;
	BOOLEAN						IsMouse = FALSE;
	BOOLEAN						IsKbd = FALSE;
	ULONG						index = 0;

	if (!pContext)
	{
		USB_DEBUG_INFO_LN_EX("NULL pContext");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (!pContext->DeviceObject)
	{
		USB_DEBUG_INFO_LN_EX("NULL DeviceObject");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	HidCommonExt = (HIDCLASS_DEVICE_EXTENSION*)(pContext->DeviceObject->DeviceExtension);
	if (!HidCommonExt)
	{
		USB_DEBUG_INFO_LN_EX("NULL HidCommonExt");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (!IsSafeNode(pContext))
	{
		USB_DEBUG_INFO_LN_EX("NULL pdoExt");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	pdoExt = GetClientPdoExtension(HidCommonExt);
	if (!pdoExt)
	{
		USB_DEBUG_INFO_LN_EX("Null pdoExt 1");
		return status;
	}

	fdoExt = GetFdoExtByClientPdoExt(pdoExt);
	if (!fdoExt)
	{
		USB_DEBUG_INFO_LN_EX("Null fdoExt 1");
		return status;
	}

	if (!pContext->node->Collection)
	{
		if (!NT_SUCCESS(VerifyMouseUsageInCollection(UrbGetTransferBuffer(pContext->urb), &pContext->node->parsedReport, &pContext->node->Collection)))
		{
			USB_DEBUG_INFO_LN_EX("NULL VerifyMouseUsageInCollection");
			status = STATUS_UNSUCCESSFUL;
			return status;
		}
	}

	if (!pContext->node->Collection)
	{
		USB_DEBUG_INFO_LN_EX("NULL Hid Collection");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (!pContext->node->InputExtractData)
	{
		switch (reportType)
		{
		case HidP_Input:
			pContext->node->InputExtractData = (EXTRACTDATA*)ExAllocatePoolWithTag(NonPagedPoolMustSucceed, sizeof(EXTRACTDATA), 'exdt');
			if (!pContext->node->InputExtractData)
			{
				break;
			}
			status = ExtractMouseData(pContext->node->Collection, reportType, pContext->node->InputExtractData);
			USB_DEBUG_INFO_LN_EX("Should only come once!");
			break;
		case HidP_Output:
			break;
		case HidP_Feature:
			break;
		default:
			break;
		}
	}

	if (!pContext->node->InputExtractData)
	{
		if (!g_IsBugReported)
		{
			g_IsBugReported = TRUE;
		}
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	data = pContext->node->InputExtractData;

	USB_DEBUG_INFO_LN_EX("ByteOffsetX: %X BitOffsetX: %X		 Size: %x", data->MOUDATA.ByteOffsetX, data->MOUDATA.BitOffsetX, data->MOUDATA.XOffsetSize);
	USB_DEBUG_INFO_LN_EX("ByteOffsetY: %X BitOffsetY: %X		 Size: %x", data->MOUDATA.ByteOffsetY, data->MOUDATA.BitOffsetY, data->MOUDATA.YOffsetSize);
	USB_DEBUG_INFO_LN_EX("ByteOffsetZ: %X BitOffsetZ: %X		 Size: %x", data->MOUDATA.ByteOffsetZ, data->MOUDATA.BitOffsetZ, data->MOUDATA.ZOffsetSize);
	USB_DEBUG_INFO_LN_EX("ByteOffsetBtn: %X BitOffsetBtn:  %X   Size: %x ", data->MOUDATA.ByteOffsetBtn, data->MOUDATA.BitOffsetBtn, data->MOUDATA.BtnOffsetSize);

	totalSize = data->MOUDATA.XOffsetSize + data->MOUDATA.YOffsetSize + data->MOUDATA.ZOffsetSize + data->MOUDATA.BtnOffsetSize;

	IsMouse = TRUE;

	if (!totalSize)
	{
		USB_DEBUG_INFO_LN_EX("Cannot Get Moudata \r\n");
		if (!g_IsBugReported)
		{
			g_IsBugReported = TRUE;
		}
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (IsMouse && g_mou_data)
	{
		USERMOUDATA mou_data = { 0 };

		mou_data.x = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.ByteOffsetX, data->MOUDATA.BitOffsetX, data->MOUDATA.XOffsetSize);
		mou_data.y = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.ByteOffsetY, data->MOUDATA.BitOffsetY, data->MOUDATA.YOffsetSize);
		mou_data.z = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.ByteOffsetZ, data->MOUDATA.BitOffsetZ, data->MOUDATA.ZOffsetSize);
		mou_data.Click = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.ByteOffsetBtn, data->MOUDATA.BitOffsetBtn, data->MOUDATA.BtnOffsetSize);
		mou_data.IsAbsolute = (data->MOUDATA.IsAbsolute) ? (mou_data.IsAbsolute | USB_MOUDATA_ABS_FLAGS) : (mou_data.IsAbsolute & ~USB_MOUDATA_ABS_FLAGS);

		OpenLoopBufferWrite(g_mou_data, (const void*)&mou_data);


		USB_DEBUG_INFO_LN_EX("ReportId: %d X: %d Y: %d Z: %d Click: %d Abs: %d Size: %d",
			data->MOUDATA.ReportId,
			mou_data.x,
			mou_data.y,
			mou_data.z,
			mou_data.Click,
			mou_data.IsAbsolute,
			data->MOUDATA.XOffsetSize);

		//Id MOU DATA cannot be get normally, report it
		if (!g_IsBugReported)
		{
			if (mou_data.x > MAX_COORDINATE_ERROR ||
				mou_data.y > MAX_COORDINATE_ERROR ||
				mou_data.z > MAX_COORDINATE_ERROR ||
				mou_data.Click > MAX_COORDINATE_ERROR)
			{

			/*	char str[REPORT_MAX_COUNT * 4 + 1] = { 0 };
				ULONG size = (UrbGetTransferLength(pContext->urb) > REPORT_MAX_COUNT) ? REPORT_MAX_COUNT : UrbGetTransferLength(pContext->urb);
				binToString((PCHAR)UrbGetTransferBuffer(pContext->urb), size, str, REPORT_MAX_COUNT * 4 + 1);

				USB_DEBUG_INFO_LN_EX("Report: x: %d y: %d z: %d ", mou_data.x,
					mou_data.y,
					mou_data.z);*/
				g_IsBugReported = TRUE;
			}
		}
	}
	return status;
}
//---------------------------------------------------------------------//
NTSTATUS UsbIrpCompletionHandler(
	_In_     PDEVICE_OBJECT  	   DeviceObject,
	_In_     PIRP            	   Irp,
	_In_	 PVOID           	   Context,
	_In_     PENDINGIRPLIST* 	   pPendingList
)
{
	HIJACK_CONTEXT*		   pContext = (HIJACK_CONTEXT*)Context;
	PVOID					context = NULL;
	IO_COMPLETION_ROUTINE* callback = NULL;

	if (!pContext)
	{
		USB_DEBUG_INFO_LN_EX("EmptyContext");
		return STATUS_UNSUCCESSFUL;
	}

	if (!pContext->pending_irp)
	{
		USB_DEBUG_INFO_LN_EX("Empty pending_irp");
		return STATUS_UNSUCCESSFUL;
	}

	context = pContext->pending_irp->oldContext;
	callback = pContext->pending_irp->oldRoutine;
	if (!context || !callback)
	{
		return STATUS_UNSUCCESSFUL;
	}

	// Completion func is called when driver unloading.
	if (g_bUnloaded)
	{
		PENDINGIRP* entry = GetRealPendingIrpByIrp(pPendingList, Irp);
		context = entry->oldContext;
		callback = entry->oldRoutine;
		USB_DEBUG_INFO_LN_EX("Safety Call old Routine: %I64x Context: %I64x", callback, context);
	}

	//Rarely call here when driver unloading, Safely call because driver_unload 
	//will handle all element in the list. we dun need to free and unlink it,
	//otherwise, system crash.
	if (g_bUnloaded && callback && context)
	{
		return callback(DeviceObject, Irp, context);
	}

	//If Driver is not unloading , delete it 
	if (!NT_SUCCESS(RemovePendingIrp(pPendingList, pContext->pending_irp)))
	{
		USB_DEBUG_INFO_LN_EX("FATAL: Delete element FAILED");
	}


	if (UrbIsInputTransfer(pContext->urb))
	{
		if (!NT_SUCCESS(HandleMouseData(pContext, HidP_Input)))
		{

		}
	}

	if (UrbIsOutputTransfer(pContext->urb))
	{
		USB_COMMON_DEBUG_INFO("Output Data ");
	}

	if (pContext)
	{
		ExFreePool(pContext);
		pContext = NULL;
	}

	USB_COMMON_DEBUG_INFO("Completion ");

	return callback(DeviceObject, Irp, context);
}
//-------------------------------------------------------------------------------------------------//
NTSTATUS  Usb2CompletionCallback(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
)
{
	return UsbIrpCompletionHandler(DeviceObject, Irp, Context, g_Usb2PendingIrpList);
}
//-----------------------------------------------------------------------------------------------//
NTSTATUS  Usb3CompletionCallback(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
)
{
	return UsbIrpCompletionHandler(DeviceObject, Irp, Context, g_Usb3PendingIrpList);
}

//----------------------------------------------------------//
NTSTATUS UsbhubIrpHandler(
	_Inout_ struct _DEVICE_OBJECT*	pDeviceObject,
	_Inout_ struct _IRP*			pIrp,
	_In_     PENDINGIRPLIST* 	   	pPendingList,
	_In_ 	 PIO_COMPLETION_ROUTINE pCompletionRoutine,
	_In_	 DRIVER_DISPATCH*		pOldFunction
)
{
	do
	{
		HIJACK_CONTEXT*		 hijack = NULL;
		PIO_STACK_LOCATION irpStack = NULL;
		PURB					urb = NULL;
		PENDINGIRP*		  new_entry = NULL;
		PHID_DEVICE_NODE	   node = NULL;

		if (!g_HidClientPdoList)
		{
			break;
		}

		irpStack = IoGetCurrentIrpStackLocation(pIrp);
		if (irpStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
		{
			break;
		}

		urb = (PURB)irpStack->Parameters.Others.Argument1;
		if (!urb)
		{
			break;
		}

		if (UrbGetFunction(urb) != URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
		{
			break;
		}

		//If Urb pipe handle is used by HID mouse / kbd device. 
		if (IsHidDevicePipe(g_HidClientPdoList->head, UrbGetTransferPipeHandle(urb), &node))
		{
			HIDCLASS_DEVICE_EXTENSION* class_extension = NULL;

			if (!node ||
				!node->device_object)
			{
				continue;
			}

			class_extension = (HIDCLASS_DEVICE_EXTENSION*)node->device_object->DeviceExtension;
			if (!class_extension->isClientPdo)
			{
				//if FDO (HID_000000x), then next
				continue;
			}
			hijack = (HIJACK_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIJACK_CONTEXT), 'kcaj');
			if (!hijack)
			{
				continue;
			}

			new_entry = (PENDINGIRP*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRP), 'kcaj');
			if (!new_entry)
			{
				ExFreePool(hijack);
				hijack = NULL;
				continue;
			}

			RtlZeroMemory(hijack, sizeof(HIJACK_CONTEXT));
			RtlZeroMemory(new_entry, sizeof(PENDINGIRP));

			//Save all we need to use when unload driver / delete node , 
			//Add to linked list
			if (irpStack)
			{

				new_entry->Irp = pIrp;
				new_entry->oldRoutine = irpStack->CompletionRoutine;
				new_entry->oldContext = irpStack->Context;
				new_entry->IrpStack = irpStack;


				//Fake Context for Completion Routine
				hijack->DeviceObject = node->device_object;		//USBHUB device
				hijack->urb = urb;
				hijack->node = node;
				hijack->pending_irp = new_entry;

				InsertPendingIrp(pPendingList, new_entry);
				irpStack->CompletionRoutine = pCompletionRoutine;

				//Completion Routine hook
				irpStack->Context = hijack;
			}

		}
	} while (0);

	return pOldFunction(pDeviceObject, pIrp);
}

//----------------------------------------------------------------------------------------//
NTSTATUS Usb2HubInternalDeviceControl(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	return UsbhubIrpHandler(DeviceObject, Irp, g_Usb2PendingIrpList, Usb2CompletionCallback, g_UsbHub2InternalDeviceControl);
}

//----------------------------------------------------------------------------------------//
NTSTATUS Usb3HubInternalDeviceControl(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	return UsbhubIrpHandler(DeviceObject, Irp, g_Usb3PendingIrpList, Usb3CompletionCallback, g_UsbHub3InternalDeviceControl);
}
//--------------------------------------------------------------------------------------------//
NTSTATUS HidUsbPnpIrpHandler(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	IRPHOOKOBJ* HookObject = NULL;
	PIO_STACK_LOCATION irpStack;

	do
	{
		HIDCLASS_DEVICE_EXTENSION* ClassExt = (HIDCLASS_DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
		if (ClassExt->isClientPdo)
		{
			USB_DEBUG_INFO_LN_EX("IsClientPdo: %x", ClassExt->isClientPdo);

			irpStack = IoGetCurrentIrpStackLocation(Irp);
			if (irpStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
			{
				RemoveNodeFromHidList(DeviceObject);
				USB_DEBUG_INFO_LN_EX("REMOVE Device ");
			}

			if (irpStack->MinorFunction == IRP_MN_START_DEVICE)
			{
				USB_DEBUG_INFO_LN_EX("Add Device");
				VerifyAndInsertIntoHidList(DeviceObject);
			}
		}
	} while (FALSE);

	return g_HidPnpHandler(DeviceObject, Irp);
}
//----------------------------------------------------------------------------------------------------------------------------------------------
ULONG __fastcall FindInfoByProcess(
	USERPROCESSINFO* Header,
	PEPROCESS proc
)
{
	if (Header)
	{
		if (Header->proc == proc)
		{
			return CLIST_FINDCB_RET;
		}
	}
	return CLIST_FINDCB_CTN;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
USERPROCESSINFO*  __fastcall FindMappedUsbInfo(
	PEPROCESS proc
)
{
	if (g_ProcessInfo)
	{
		return (USERPROCESSINFO*)QueryFromChainListByCallback(g_ProcessInfo, &FindInfoByProcess, proc);
	}
	return NULL;
}


//----------------------------------------------------------------------------------------//
NTSTATUS AllocateUsb2PendingList()
{
	if (!NT_SUCCESS(AllocatePendingIrpLinkedList(&g_Usb2PendingIrpList)))
	{
		USB_DEBUG_INFO_LN_EX("AllocatePendingIrpLinkedList Error");
	}
	return (g_Usb2PendingIrpList) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}
//----------------------------------------------------------------------------------------//
NTSTATUS AllocateUsb3PendingList()
{
	if (!NT_SUCCESS(AllocatePendingIrpLinkedList(&g_Usb3PendingIrpList)))
	{
		USB_DEBUG_INFO_LN_EX("AllocatePendingIrpLinkedList Error");
	}

	return (g_Usb3PendingIrpList) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}
//----------------------------------------------------------------------------------------//
NTSTATUS FreeUsb2PendingList()
{
	if (g_Usb2PendingIrpList)
	{
		FreePendingList(g_Usb2PendingIrpList);
		g_Usb2PendingIrpList = NULL;
	}
	return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------//
NTSTATUS FreeUsb3PendingList()
{
	if (g_Usb3PendingIrpList)
	{
		FreePendingList(g_Usb3PendingIrpList);
		g_Usb3PendingIrpList = NULL;
	}
	return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
NTSTATUS MappingUsbMemory(
	void* 	 source,
	ULONG 	 length,
	PVOID* mapped_user_address
)
{
	PEPROCESS proc = PsGetCurrentProcess();

	USERPROCESSINFO* processInfo = FindMappedUsbInfo(proc);

	if (!source ||
		!length)
	{
		USB_DEBUG_INFO_LN_EX("Mapped STATUS_INVALID_PARAMETER %x %x %x \r\n ", mapped_user_address, source, length);
		return STATUS_INVALID_PARAMETER;
	}

	if (!processInfo)
	{
		processInfo = (USERPROCESSINFO*)ExAllocatePoolWithTag(NonPagedPool, sizeof(USERPROCESSINFO), 'pinf');
		if (!processInfo)
		{
			return STATUS_UNSUCCESSFUL;
		}

		processInfo->proc = PsGetCurrentProcess();
		processInfo->mdl = NULL;
//		processInfo->mapped_user_addr = MapNonpagedMemToSpace(source, length, &processInfo->mdl, UserMode, 0);

		if (!processInfo->mapped_user_addr)
		{
			ExFreePool(processInfo);
			processInfo = NULL;

			USB_DEBUG_INFO_LN_EX("Cannot Mapping Memory \r\n ");
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		AddToChainListTail(g_ProcessInfo, processInfo);

		USB_DEBUG_INFO_LN_EX("*mapped_user_address : %I64X \r\n ", processInfo->mapped_user_addr);
	}

	*mapped_user_address = processInfo->mapped_user_addr;
	return STATUS_SUCCESS;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------
NTSTATUS MapUsbDataToUserAddressSpace(
	_In_  	USERCONFIGEX*     UserModeConfigEx,
	_In_    ULONG		      Size
)
{
	PVOID user_mode_addr = 0;
	if (Size != sizeof(USERCONFIGEX))
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (!g_mou_data)
	{
		USB_DEBUG_INFO_LN_EX("MapUsbDataToUserAddressSpace Successfully");
		return STATUS_UNSUCCESSFUL;
	}

/*
	if (!MProbeForWriteUser(UserModeConfigEx, sizeof(USERCONFIGEX)))
	{
		return STATUS_INVALID_PARAMETER;
	}
*/
	if (NT_SUCCESS(MappingUsbMemory(g_mou_data->Header, sizeof(g_mou_data->Header), &user_mode_addr)))
	{
		UserModeConfigEx->Version = USB_PEN_INIT_VERSION;
		UserModeConfigEx->ProtocolVersion = USB_PEN_PROTOCOL_VERSION;
		UserModeConfigEx->UserData = (ULONG64)user_mode_addr;
		UserModeConfigEx->UserDataLen = sizeof(user_mode_addr);
		USB_DEBUG_INFO_LN_EX("MapUsbDataToUserAddressSpace Successfully");
		return STATUS_SUCCESS;
	}

	USB_DEBUG_INFO_LN_EX("MapUsbDataToUserAddressSpace UnSuccessfully");

	return STATUS_UNSUCCESSFUL;
}
//-------------------------------------------------------------------------------------------------------------
ULONG   __fastcall ProcessListActionCallback(USERPROCESSINFO* Header, ULONG Act)
{
	if (Act == CLIST_ACTION_FREE)
	{
		if (!Header)
		{
			return 0;
		}

		if (Header->mdl && Header->mapped_user_addr)
		{
			MmUnmapLockedPages(Header->mapped_user_addr, Header->mdl);
			IoFreeMdl(Header->mdl);
		}

		Header->mdl = NULL;
		Header->mapped_user_addr = NULL;
		Header->proc = NULL;

		ExFreePool(Header);
		Header = NULL;
	}
	return 0;
}


//---------------------------------------------------------------------------------------
ULONG   __fastcall DeleteProcessListCallback(
	USERPROCESSINFO* Header,
	PEPROCESS 		 proc
)
{
	if (Header)
	{
		if (Header->proc == proc)
		{
			USB_DEBUG_INFO_LN_EX("Found n delete Proc");
			return CLIST_FINDCB_DEL;
		}
	}
	return CLIST_FINDCB_CTN;
}

//----------------------------------------------------------------------------------------//
NTSTATUS OnProcessExitToUsbSystem(PEPROCESS eProcess)
{
	if (g_ProcessInfo)
	{
		//call free_routine when delete a element
		QueryFromChainListByCallback(g_ProcessInfo, &DeleteProcessListCallback, eProcess);
	}
	return STATUS_SUCCESS;
}
//---------------------------------------------------------------------------------------------------------//
NTSTATUS FreeProcessInfoList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	//Free White List

	if (g_ProcessInfo)
	{
		CHAINLIST_SAFE_FREE(g_ProcessInfo);
	}

	status = STATUS_SUCCESS;
	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS InitializeHidPenetrate()
{
	PDRIVER_OBJECT			  pHub2DriverObj = NULL;
	PDRIVER_OBJECT			  pHub3DriverObj = NULL;
	PDRIVER_OBJECT			  pHidDriverObj = NULL;
	NTSTATUS						 status = STATUS_UNSUCCESSFUL;
	ULONG						   ListSize = 0;

	USB_DEBUG_INFO_LN_EX("InitializeHidPenetrate1");

	if (g_bLoaded)
	{
		status = STATUS_SUCCESS;
		return status;
	}

	status = GetDriverObjectByName(HID_USB_DEVICE, &pHidDriverObj);
	if (!NT_SUCCESS(status) || !pHidDriverObj)
	{
		USB_DEBUG_INFO_LN_EX("GetUsbHub Error");
		return status;
	}

	USB_DEBUG_INFO_LN_EX("InitializeHidPenetrate2");

	status = GetUsbHub(USB2, &pHub2DriverObj);	// iusbhub
	if (!NT_SUCCESS(status) || !pHub2DriverObj)
	{
		USB_DEBUG_INFO_LN_EX("No Usb2 driver");
		g_Usb3Count++;
	}

	status = GetUsbHub(USB3, &pHub3DriverObj);	// usb3 - win7
	if (!NT_SUCCESS(status) || !pHub3DriverObj)
	{
		USB_DEBUG_INFO_LN_EX("No Usb3 driver");
		g_Usb3Count++;
		status = GetUsbHub(USB3_NEW, &pHub3DriverObj);	// usb3 plugged driver - win8 up
		if (!NT_SUCCESS(status) || !pHub3DriverObj)
		{
			USB_DEBUG_INFO_LN_EX("No Usb3 new driver");
			g_Usb3Count++;
		}
	}

	if (!pHub3DriverObj && !pHub2DriverObj)
	{
		USB_DEBUG_INFO_LN_EX("No any Usb driver ");
		return status;
	}

	USB_DEBUG_INFO_LN_EX("InitializeHidPenetrate3");

	if (!g_mou_data)
	{
		g_mou_data = NewOpenLoopBuffer(CB_SIZE, sizeof(USERMOUDATA), OPENLOOPBUFF_FALGS_PEASUDOHEADER);
		if (!g_mou_data)
		{
			status = STATUS_UNSUCCESSFUL;
			return status;
		}
	}

	if (!g_ProcessInfo)
	{
		g_ProcessInfo = NewChainListHeaderEx(LISTFLAG_FTMUTEXLOCK | LISTFLAG_AUTOFREE, ProcessListActionCallback, 0);
		if (!g_ProcessInfo)
		{
			status = STATUS_UNSUCCESSFUL;
			return status;
		}
	}

	USB_DEBUG_INFO_LN_EX("g_mou_data: %I64x g_ProcessInfo : %I64x ", g_mou_data, g_ProcessInfo);

	//Prepare HID PipeList
	status = InitHidSubSystem(&ListSize);
	if (!NT_SUCCESS(status) || !ListSize)
	{ 
		FreeProcessInfoList();
		USB_DEBUG_INFO_LN_EX("No keyboard Or Mouse");
		return status;
	}

	USB_DEBUG_INFO_LN_EX("HID SubSystem Initalization Successfully ist size: %x ", ListSize);

	//Prepare IRP Hook 
	status = InitIrpHookSystem();
	if (!NT_SUCCESS(status))
	{
		UnInitHidSubSystem();
		FreeProcessInfoList();

		return status;
	}

	USB_DEBUG_INFO_LN_EX("IRP Hook SubSystem Initalization Successfully");


	g_HidPnpHandler = (PDRIVER_DISPATCH)DoIrpHook(pHidDriverObj, IRP_MJ_PNP, HidUsbPnpIrpHandler, Start);
	if (!g_HidPnpHandler)
	{
		UnInitIrpHookSystem();
		UnInitHidSubSystem();
		FreeProcessInfoList();

		USB_DEBUG_INFO_LN_EX("DoIrpHook Error ");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	USB_DEBUG_INFO_LN_EX("IRP Hook HidUsb PnP Successfully");

	if (pHub2DriverObj)
	{
		status = AllocateUsb2PendingList();
		if (NT_SUCCESS(status))
		{
			//Do Irp Hook for URB transmit
			g_UsbHub2InternalDeviceControl = (PDRIVER_DISPATCH)DoIrpHook(pHub2DriverObj, IRP_MJ_INTERNAL_DEVICE_CONTROL, Usb2HubInternalDeviceControl, Start);
		}
	}

	if (pHub3DriverObj)
	{
		status = AllocateUsb3PendingList();

		if (NT_SUCCESS(status))
		{
			//Do Irp Hook for URB transmit
			g_UsbHub3InternalDeviceControl = (PDRIVER_DISPATCH)DoIrpHook(pHub3DriverObj, IRP_MJ_INTERNAL_DEVICE_CONTROL, Usb3HubInternalDeviceControl, Start);
		}
	}

	if (!g_UsbHub3InternalDeviceControl &&
		!g_UsbHub2InternalDeviceControl)
	{
		UnInitializeHidPenetrate();
		USB_DEBUG_INFO_LN_EX("This computer don't supported USB2 or USB3 ");
		status = STATUS_UNSUCCESSFUL;
	}


	USB_DEBUG_INFO_LN_EX("IRP Hooked Internal Device Control");

	USB_DEBUG_INFO_LN_EX("InitializeHidPenetrate7");

	g_bLoaded = TRUE;

	return status;
}

//----------------------------------------------------------------------------------------//
NTSTATUS UnInitializeHidPenetrate()
{
	NTSTATUS status;
	int i = 0;
	USB_DEBUG_INFO_LN_EX("UnInitializeHidPenetrate");
	g_bUnloaded = TRUE;
	g_bLoaded = FALSE;

	//Recovery Irp all hook and Free the List
	UnInitIrpHookSystem();

	//Recovery Completion hook and Free the List
	FreeUsb2PendingList();

	//Recovery Completion hook and Free the List
	FreeUsb3PendingList();

	//Free the HID Client PDO pipe list 
	UnInitHidSubSystem();

	//Free Process Info List
	FreeProcessInfoList();

	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------------------------------------------------------------
BOOLEAN IsReportHidRawDesc()
{
	return g_IsReportRawDesc;
}
 
//----------------------------------------------------------------------------------------------------------------------------------------------
NTSTATUS UsbPenerateConfigInit()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	USB_DEBUG_INFO_LN_EX("UsbPenerateConfigInit");
	InitializeHidPenetrate();
	return ntStatus;
}