                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #include "UsbHid.h"
#include "UsbUtil.h" 
#include "WinParse.h"
#include "ReportUtil.h"  
#include "wdmguid.h"
#include "IrpHook.h"

#define HIDP_MAX_UNKNOWN_ITEMS 4
/*
struct _CHANNEL_REPORT_HEADER
{
	USHORT Offset;  // Position in the _CHANNEL_ITEM array
	USHORT Size;    // Length in said array
	USHORT Index;
	USHORT ByteLen; // The length of the data including reportID.
					// This is the longest such report that might be received
					// for the given collection.
};
/*
typedef struct _HIDP_CHANNEL_DESC
{
	USHORT   UsagePage;		  //+0x0
	UCHAR    ReportID;		  //+0x2
	UCHAR    BitOffset;		  // 0 to 8 value describing bit alignment	+0x3
	USHORT   ReportSize;	  // HID defined report size	+0x4
	USHORT   ReportCount;	  // HID defined report count	+0x6
	USHORT   ByteOffset;	  // byte position of start of field in report packet +0x8
	USHORT   BitLength;		  // total bit length of this channel	+0xA
	ULONG    BitField;		  // The 8 (plus extra) bits associated with a main item +0xC
	USHORT   ByteEnd;		  // First byte not containing bits of this channel. +0x10
	USHORT   LinkCollection;  // A unique internal index pointer	+0x12
	USHORT   LinkUsagePage;   // +0x14
	USHORT   LinkUsage;		  // +0x16
	union
	{
		struct
		{
			ULONG  MoreChannels : 1; // Are there more channel desc associated with
									 // this array.  This happens if there is a
									 // several usages for one main item.
			ULONG  IsConst : 1;		// Does this channel represent filler
			ULONG  IsButton : 1;	// Is this a channel of binary usages, not value usages.
			ULONG  IsAbsolute : 1;  // As apposed to relative
			ULONG  IsRange : 1;
			ULONG  IsAlias : 1;		// a usage described in a delimiter
			ULONG  IsStringRange : 1;
			ULONG  IsDesignatorRange : 1;
			ULONG  Reserved : 20;
			ULONG  NumGlobalUnknowns : 4;
		};
		ULONG  all;
	};
	struct _HIDP_UNKNOWN_TOKEN GlobalUnknowns[HIDP_MAX_UNKNOWN_ITEMS];

	union {
		struct {
			USHORT   UsageMin, UsageMax;
			USHORT   StringMin, StringMax;
			USHORT   DesignatorMin, DesignatorMax;
			USHORT   DataIndexMin, DataIndexMax;
		} Range;
		struct {
			USHORT   Usage, Reserved1;
			USHORT   StringIndex, Reserved2;
			USHORT   DesignatorIndex, Reserved3;
			USHORT   DataIndex, Reserved4;
		} NotRange;
	};

	union {
		struct {
			LONG     LogicalMin, LogicalMax;
		} button;
		struct {
			BOOLEAN  HasNull;  // Does this channel have a null report
			UCHAR    Reserved[3];
			LONG     LogicalMin, LogicalMax;
			LONG     PhysicalMin, PhysicalMax;
		} Data;
	};

	ULONG Units;
	ULONG UnitExp;
} HIDP_CHANNEL_DESC, *PHIDP_CHANNEL_DESC;

typedef struct _HIDP_SYS_POWER_INFO {
	ULONG   PowerButtonMask;

} HIDP_SYS_POWER_INFO, *PHIDP_SYS_POWER_INFO;
typedef struct _HIDP_PREPARSED_DATA
{
	LONG   Signature1, Signature2;
	USHORT Usage;
	USHORT UsagePage;

	HIDP_SYS_POWER_INFO;

	// The following channel report headers point to data within
	// the Data field below using array indices.
	struct _CHANNEL_REPORT_HEADER Input;
	struct _CHANNEL_REPORT_HEADER Output;
	struct _CHANNEL_REPORT_HEADER Feature;

	// After the CANNEL_DESC array the follows a LinkCollection array nodes.
	// LinkCollectionArrayOffset is the index given to RawBytes to find
	// the first location of the _HIDP_LINK_COLLECTION_NODE structure array
	// (index zero) and LinkCollectionArrayLength is the number of array
	// elements in that array.
	USHORT LinkCollectionArrayOffset;
	USHORT LinkCollectionArrayLength;

	union {
		HIDP_CHANNEL_DESC    Data[];
		UCHAR                RawBytes[];
	};
} HIDP_PREPARSED_DATA;
*/
 
/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Global/Extern Variable 
//// 
HID_DEVICE_LIST*  g_HidClientPdoList = NULL; 
ULONG			  g_listSize = NULL;
HIDP_DEVICE_DESC* g_hid_collection = NULL; 
PVOID			  g_PnpCallbackEntry = NULL;
DRIVER_DISPATCH*  g_pOldPnpHandler = NULL;
/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Marco
//// 
#define HID_USB_DEVICE				 	 L"\\Driver\\hidusb"
#define ARRAY_SIZE						 100
#define HIDP_PREPARSED_DATA_SIGNATURE1	 'PdiH'
#define HIDP_PREPARSED_DATA_SIGNATURE2   'RDK '
  

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		1. look up a pipe in pipe list
		  
Arguments:
		Data			 - pointer of PHID_DEVICE_NODE, see detail in PHID_DEVICE_NODE
						   declaration.

		Context			 - pointer of Pipe Handle From Urb.
		 
Return Value:	
		NTSTATUS		 - true  , if it initial succesfully.
						 - false , if it initial faile.

-----------------------------------------------------------------------------------*/
LOOKUP_STATUS LookupPipeHandleCallback(
	_In_ PHID_DEVICE_NODE Data,
	_In_ void* Context
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 

Routine Description:
		See a detail in UsbHid.h
		  
Arguments:
		See a detail in UsbHid.h
		 
Return Value:	
		See a detail in UsbHid.h

-----------------------------------------------------------------------------------*/
BOOLEAN IsHidDevicePipe(
	_In_ TChainListHeader* PipeListHeader,
	_In_ USBD_PIPE_HANDLE  PipeHandle,
	_Out_ PHID_DEVICE_NODE*		node
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		Core function it translates a report descriptor into a human-readable structure

Arguments:

		HidClassExtension	- HidClassExtension, which is shared by all hid mini-driver  
		
		AllReport			- Get all pointer from it device

Return Value:
		NTSTATUS			- STATUS_SUCCESS				, if translate success
							- STATUS_UNSUCCESSFUL or others	, if translate error
-----------------------------------------------------------------------------------*/
NTSTATUS GetAllReportByDeviceExtension(
	_In_  HIDCLASS_DEVICE_EXTENSION* HidClassExtension,
	_Out_ HIDP_DEVICE_DESC* AllReport
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		Create a device node for when it is a Hid Client Pdo

Arguments:

		DeviceObject			- Hid Device Object (Hid Client Pdo)
		
		MiniExtension			- MiniExtension stored in ClassExtension

Return Value:
		HID_DEVICE_NODE			- Created Node, if created success
								- NULL		  , if failed			
-----------------------------------------------------------------------------------*/
HID_DEVICE_NODE* CreateHidDeviceNode(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ HID_USB_DEVICE_EXTENSION* MiniExtension,
	_In_ HIDP_DEVICE_DESC* parsedReport
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Deteremine a device objects if it is a Client PDO for Keyboard / Mouse.
		set a MiniExtension and return TRUE. Otherwise, FALSE
		For More Information between Cient PDO and class FDO See Doc.

Arguments:

		DeviceObject			- Hid Device Object (Hid Client Pdo, created by )
		
		MiniExtension			- MiniExtension stored in ClassExtension

Return Value:
		NTSTATUS			- STATUS_SUCCESS				, if translate success
							- STATUS_UNSUCCESSFUL or others	, if translate error

-----------------------------------------------------------------------------------*/
BOOLEAN  IsHidTypeDevice(
	_In_  PDEVICE_OBJECT				   DeviceObject,
	_Out_ PHID_USB_DEVICE_EXTENSION*	  MiniExtension
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		- Verify Hid Device Object, just simply call VerifyDevice by walk through
		  each device object in the HID device object stack.

Arguments:

		DriverObject			- Hid Driver Object
		
		ClientPdoCount			- Total of Client Pdo 		

Return Value:
		NTSTATUS			- STATUS_SUCCESS				, if translate success
							- STATUS_UNSUCCESSFUL or others	, if translate error

-----------------------------------------------------------------------------------*/
NTSTATUS VerifyHidDeviceObject(
	_In_  PDRIVER_OBJECT DriverObject, 
	_Out_  ULONG*		ClientPdoCount
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Init Pdo List, basically get a Hid Driver Object and call VerifyHidDeviceObject.

Arguments:

		ListSize			- Size of List

Return Value:
		NTSTATUS			- STATUS_SUCCESS				, if translate success
							- STATUS_UNSUCCESSFUL or others	, if translate error

-----------------------------------------------------------------------------------*/ 
NTSTATUS InitClientPdoList(
	_Out_ PULONG ListSize
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Allocate Pdo List memory

Arguments:

		No

Return Value:
		NTSTATUS			- STATUS_SUCCESS				, if alloc success
							- STATUS_UNSUCCESSFUL or others	, if alloc error

-----------------------------------------------------------------------------------*/
NTSTATUS AllocatePdoList();

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Interface for Init a Hid Client Pdo List

Arguments:

		ClientPdoList			- As name said
		
		ClientPdoListSize		- As name said

Return Value:
		NTSTATUS			- STATUS_SUCCESS				, if Initial success
							- STATUS_UNSUCCESSFUL or others	, if Initial error

-----------------------------------------------------------------------------------*/
NTSTATUS InitHidClientPdoList(
	_Out_ PHID_DEVICE_LIST* ClientPdoList,
	_Out_ PULONG ClientPdoListSize
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Free Hid Client Pdo List

Arguments:

		No

Return Value:

		NTSTATUS			- STATUS_SUCCESS				, if translate success
							- STATUS_UNSUCCESSFUL or others	, if translate error

-----------------------------------------------------------------------------------*/
NTSTATUS FreeHidClientPdoList();

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Monitor IRP IRP_MN_REMOVE_DEVICE AND IRP_MN_START_DEVICE Event

Arguments:

		No

Return Value:

		NTSTATUS			- STATUS_SUCCESS				, if translate success
							- STATUS_UNSUCCESSFUL or others	, if translate error

-----------------------------------------------------------------------------------*/
NTSTATUS HidUsbPnpIrpHandler(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
);


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Verify a Device is it hid device and add it into linked list

Arguments:

		DeviceObject		- Device to be verify

Return Value:

		NTSTATUS			- STATUS_SUCCESS				, if verify device success
							- STATUS_UNSUCCESSFUL or others	, if verify device error

-----------------------------------------------------------------------------------*/
NTSTATUS VerifyDevice(
	_In_  PDEVICE_OBJECT DeviceObject, 
	_Out_ PHID_DEVICE_NODE* Node);

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
////
//--------------------------------------------------------------------------------------------//
ULONG SearchHidNodeCallback(
	_In_ PHID_DEVICE_NODE HidNode,
	_In_ void* pDeviceObject
)
{
	PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)pDeviceObject;
	if (DeviceObject == HidNode->device_object)
	{
		return CLIST_FINDCB_RET;
	}
	return CLIST_FINDCB_CTN;
}
//--------------------------------------------------------------------------------------------//
IRPHOOKOBJ* GetHidNode(
	_In_ PDRIVER_OBJECT DeviceObject
)
{ 
	return QueryFromChainListByCallback(g_HidClientPdoList->head, SearchHidNodeCallback, DeviceObject);
}

//--------------------------------------------------------------------------------------------//
NTSTATUS HidUsbPnpIrpHandler(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	PIO_STACK_LOCATION irpStack;
	WCHAR* DeviceName[256] = { 0 };
	GetDeviceName(DeviceObject, DeviceName); 
	USB_DEBUG_INFO_LN_EX("Pdo Name: %ws ", DeviceName);
	do
	{
		HIDCLASS_DEVICE_EXTENSION* ClassExt = (HIDCLASS_DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
		if (ClassExt->isClientPdo)
		{ 
			USB_DEBUG_INFO_LN_EX("IsClientPdo: %x", ClassExt->isClientPdo);

			irpStack = IoGetCurrentIrpStackLocation(Irp);
			if (irpStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
			{
				USB_DEBUG_INFO_LN_EX("REMOVE Device ");
				DelFromChainListByPointer(g_HidClientPdoList->head, GetHidNode(DeviceObject));  
				g_HidClientPdoList->currentSize--;
			}

			if (irpStack->MinorFunction == IRP_MN_START_DEVICE)
			{  
				HID_DEVICE_NODE* node = NULL;
				if (!NT_SUCCESS(VerifyDevice(DeviceObject, &node)))
				{
					break;
				}

				if (node)
				{
					AddToChainListTail(g_HidClientPdoList->head, node);
					g_HidClientPdoList->currentSize++; 
				}

				USB_DEBUG_INFO_LN_EX("Add Device");
			}
		}
	} while (FALSE);
	
	IRPHOOKOBJ* HookObj = GetIrpHookObject(DeviceObject->DriverObject, IRP_MJ_PNP);
	if (HookObj)
	{
		if (HookObj->oldFunction)
		{
			return HookObj->oldFunction(DeviceObject, Irp);
		}
	} 
	return g_pOldPnpHandler(DeviceObject, Irp);
}   
//----------------------------------------------------------------------------------------//
LOOKUP_STATUS LookupPipeHandleCallback(
	PHID_DEVICE_NODE Data,
	void* Context
)
{
	if (!Data || !Context)
	{
		USB_DEBUG_INFO_LN_EX("Empty Data / Context");
		return CLIST_FINDCB_CTN;
	}
	if (!Data->mini_extension)
	{
		USB_DEBUG_INFO_LN_EX("Empty mini_extension");
		return CLIST_FINDCB_CTN;
	}
	if (!Data->mini_extension->InterfaceDesc)
	{
		USB_DEBUG_INFO_LN_EX("Empty InterfaceDesc");
		return CLIST_FINDCB_CTN;
	}
	if (!Data->mini_extension->InterfaceDesc->Pipes)
	{
		USB_DEBUG_INFO_LN_EX("Empty Pipes");
		return CLIST_FINDCB_CTN;
	}

	NTSTATUS								   status = STATUS_UNSUCCESSFUL;
	USBD_PIPE_HANDLE				PipeHandleFromUrb = Context;
	USBD_PIPE_INFORMATION*	 PipeHandleFromWhiteLists = Data->mini_extension->InterfaceDesc->Pipes;
	ULONG								NumberOfPipes = Data->mini_extension->InterfaceDesc->NumberOfPipes;
	ULONG i;
	for (i = 0; i < NumberOfPipes; i++)
	{
		if (PipeHandleFromUrb == PipeHandleFromWhiteLists[i].PipeHandle)
		{
			status = STATUS_SUCCESS;
			break;
		}
	}

	if (!NT_SUCCESS(status))
	{
		return 	CLIST_FINDCB_CTN;
	}
	return CLIST_FINDCB_RET;
}

//----------------------------------------------------------------------------------------// 
BOOLEAN IsHidDevicePipe(
	_In_ TChainListHeader* PipeListHeader,
	_In_ USBD_PIPE_HANDLE  PipeHandle,
	_Out_ PHID_DEVICE_NODE*		node
)
{
	BOOLEAN exist = FALSE;
	*node = QueryFromChainListByCallback(PipeListHeader, LookupPipeHandleCallback, PipeHandle);

	if (*node)
	{
		exist = TRUE;
	}

	return exist;
}

//----------------------------------------------------------------------------------------------------------//
NTSTATUS GetAllReportByDeviceExtension(
	_In_  HIDCLASS_DEVICE_EXTENSION* HidClassExtension,
	_Out_ HIDP_DEVICE_DESC* AllReport
)
{
	PDO_EXTENSION* pdoExt = NULL;
	FDO_EXTENSION* fdoExt = NULL; 
	HIDCLASS_DEVICE_EXTENSION* addr = NULL;
	WCHAR name[256] = { 0 };
	HIDP_DEVICE_DESC hid_device_desc = { 0 };
	NTSTATUS status = STATUS_SUCCESS;
	do
	{
		if (!HidClassExtension || !AllReport)
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		pdoExt = &HidClassExtension->pdoExt;

		if (!pdoExt)
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		addr = (HIDCLASS_DEVICE_EXTENSION*)pdoExt->deviceFdoExt;
		if (!addr)
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		fdoExt = &addr->fdoExt;
		if (!fdoExt)
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		GetDeviceName(fdoExt->fdo, name);
		USB_DEBUG_INFO_LN_EX("Pdo->fdoExt: %I64x ", fdoExt);
		USB_DEBUG_INFO_LN_EX("Pdo->fdo: %I64x ", fdoExt->fdo);
		USB_DEBUG_INFO_LN_EX("Pdo->fdo DriverName: %ws ", fdoExt->fdo->DriverObject->DriverName.Buffer);
		USB_DEBUG_INFO_LN_EX("Pdo->CollectionIndex: %I64x", pdoExt->collectionIndex);
		USB_DEBUG_INFO_LN_EX("Pdo->CollectionNum: %I64x", pdoExt->collectionNum);

		status = GetCollectionDescription(fdoExt->rawReportDescription, fdoExt->rawReportDescriptionLength, NonPagedPool, &hid_device_desc);//==(HIDP_DEVICE_DESC*)((PUCHAR)fdoExt + 0x58);

		DumpReport(&hid_device_desc);

		RtlMoveMemory(AllReport, &hid_device_desc, sizeof(HIDP_DEVICE_DESC));

	} while (FALSE);

	return status;
} 

//-------------------------------------------------------------------------------------------//
HID_DEVICE_NODE* CreateHidDeviceNode(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ HID_USB_DEVICE_EXTENSION* MiniExtension,
	_In_ HIDP_DEVICE_DESC* ParsedReport
)
{
	HID_DEVICE_NODE* node = NULL;
	if (!DeviceObject || !MiniExtension || !ParsedReport)
	{
		return NULL;
	}

	node = ExAllocatePoolWithTag(NonPagedPool, sizeof(HID_DEVICE_NODE), 'edon');
	if (!node)
	{
		return NULL;
	}

	RtlZeroMemory(node, sizeof(HID_DEVICE_NODE));
	node->device_object  = DeviceObject;
	node->mini_extension = MiniExtension;
	RtlMoveMemory(&node->parsedReport , ParsedReport, sizeof(HIDP_DEVICE_DESC));

	return node;
} 
//---------------------------------------------------------------------------------------------------------//
NTSTATUS VerifyDevice(
	_In_  PDEVICE_OBJECT DeviceObject, 
	_Out_ PHID_DEVICE_NODE* Node)
{
	HIDP_DEVICE_DESC* report = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL; 
	PHID_USB_DEVICE_EXTENSION MiniExtension = NULL;
	HID_DEVICE_NODE* node = NULL;
	do {

		if (!DeviceObject || !g_HidClientPdoList)
		{
			status = STATUS_UNSUCCESSFUL; 
			break;
		} 
		WCHAR DeviceName[256] = { 0 };
		GetDeviceName(DeviceObject, DeviceName);

		if (!IsHidTypeDevice(DeviceObject, &MiniExtension))
		{ 
			if (!MiniExtension)
			{ 
				status = STATUS_UNSUCCESSFUL; 
				break;
			}
		}

		report = (HIDP_DEVICE_DESC*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIDP_DEVICE_DESC), 'csed');
		if (!report)
		{
			USB_DEBUG_INFO_LN_EX("Empty report");
			status = STATUS_UNSUCCESSFUL; 
			break;
		}

		status = GetAllReportByDeviceExtension(DeviceObject->DeviceExtension, report);
		if (!NT_SUCCESS(status))
		{
			USB_DEBUG_INFO_LN_EX("GetAllReport Failed");
			break;
		}

		node = CreateHidDeviceNode(DeviceObject, MiniExtension, report);
		if (!node)
		{
			USB_DEBUG_INFO_LN_EX("CreateNode Failed");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		*Node = node;
		status = STATUS_SUCCESS;

 
	} while (FALSE);

	if (report)
	{
		ExFreePool(report);
		report = NULL;
	}

	return status; 
}
//---------------------------------------------------------------------------------------------------------//
BOOLEAN  IsHidTypeDevice(
	_In_  PDEVICE_OBJECT				   DeviceObject,
	_Out_ PHID_USB_DEVICE_EXTENSION*      MiniExtension
)
{
	HID_USB_DEVICE_EXTENSION*  pMiniExtension   = NULL;
	HIDCLASS_DEVICE_EXTENSION* pClassExtension  = NULL;
	WCHAR						DeviceName[256] = { 0 };
	BOOLEAN			 					   ret  = FALSE; 
	do
	{
		if (!DeviceObject)
		{
			USB_DEBUG_INFO_LN_EX("Empty Device Object");
			ret = FALSE;
			break;
		}

		pClassExtension = (HIDCLASS_DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
		if (!pClassExtension)
		{
			USB_DEBUG_INFO_LN_EX("Empty pClassExtension");
			ret = FALSE;
			break; 
		}

		//USB_MON_DEBUG_INFO("Extension_common: %I64X sizeof: %x \r\n", hid_common_extension, sizeof(HID_USB_DEVICE_EXTENSION));
		pMiniExtension = (HID_USB_DEVICE_EXTENSION*)pClassExtension->hidExt.MiniDeviceExtension;
		if (!pMiniExtension)
		{
			USB_DEBUG_INFO_LN_EX("Empty pMiniExtension");
			ret = FALSE;
			break; 
		}

		//DumpHidMiniDriverExtension(hid_common_extension); 

		//Device Name = HID_0000000X
		if (!pClassExtension->isClientPdo)
		{
			USB_DEBUG_INFO_LN_EX("is not ClientPdo");
			*MiniExtension = NULL;
			ret = FALSE;
			break; 
		}

		if (pMiniExtension->InterfaceDesc->Class == 3 &&			//HidClass Device
			(pMiniExtension->InterfaceDesc->Protocol == 1 ||		//Keyboard
			 pMiniExtension->InterfaceDesc->Protocol == 2 ||		//Mouse
			 pMiniExtension->InterfaceDesc->Protocol == 0))
		{
			GetDeviceName(DeviceObject, DeviceName);
			
			USB_DEBUG_INFO_LN_EX("----------------------------------------------------------------------------------------------------");
			USB_DEBUG_INFO_LN_EX("hid_common_extension: %I64x", pClassExtension);
			USB_DEBUG_INFO_LN_EX("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws", DeviceObject, DeviceObject->DriverObject->DriverName.Buffer, DeviceName);
			USB_DEBUG_INFO_LN_EX("collectionIndex: %x collectionNum: %x", pClassExtension->pdoExt.collectionIndex, pClassExtension->pdoExt.collectionNum);
			USB_DEBUG_INFO_LN_EX("----------------------------------------------------------------------------------------------------");

			*MiniExtension = pMiniExtension;
			ret =  TRUE;
			break; 
		}

	} while (FALSE);

	return ret;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS VerifyHidDeviceObject(
	_In_ PDRIVER_OBJECT DriverObject,
	_Out_ ULONG*		ClientPdoCount
)
{
	PDEVICE_OBJECT	DeviceObject = NULL;
	ULONG		HidClientPdoCount = 0;
	NTSTATUS			   status = STATUS_UNSUCCESSFUL;

	if (!g_HidClientPdoList || !DriverObject)
	{
		return status;
	}

	DeviceObject = DriverObject->DeviceObject;
	while (DeviceObject)
	{
		HID_DEVICE_NODE* node = NULL ;
		if (!NT_SUCCESS(VerifyDevice(DeviceObject, &node)))
		{
			goto Next;
		} 

		if (node)
		{
			AddToChainListTail(g_HidClientPdoList->head, node);
			g_HidClientPdoList->currentSize++; 
			HidClientPdoCount++;
			USB_DEBUG_INFO_LN_EX("Inserted one element");
		}
Next:
		DeviceObject = DeviceObject->NextDevice;
	}

	if (HidClientPdoCount)
	{
		*ClientPdoCount = HidClientPdoCount;
		status = STATUS_SUCCESS;
	}

	return status;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitClientPdoList(
	_Out_ PULONG ListSize
)
{
	NTSTATUS	   		  status = STATUS_SUCCESS;
	PDRIVER_OBJECT HidDriverObj = NULL;
	ULONG			 return_size = 0;

	do {
		if (!ListSize || !g_HidClientPdoList)
		{
			USB_DEBUG_INFO_LN_EX("Empty Params");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(GetDriverObjectByName(HID_USB_DEVICE, &HidDriverObj)))
		{
			USB_DEBUG_INFO_LN_EX("Get Drv Obj Error");
			status = STATUS_UNSUCCESSFUL;
			break;
		} 

		if (!HidDriverObj)
		{
			USB_DEBUG_INFO_LN_EX("Empty DrvObj");
			status = STATUS_UNSUCCESSFUL; 
			break;
		}

		//Do Irp Hook for URB transmit
		g_pOldPnpHandler = (PDRIVER_DISPATCH)DoIrpHook(HidDriverObj, IRP_MJ_PNP, HidUsbPnpIrpHandler, Start);
		if (!g_pOldPnpHandler)
		{ 
			USB_DEBUG_INFO_LN_EX("DoIrpHook Error ");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(VerifyHidDeviceObject(HidDriverObj, &ListSize)))
		{
			USB_DEBUG_INFO_LN_EX("VerifyHidDeviceObject Error");
			status = STATUS_UNSUCCESSFUL;
			break; 
		}  

	} while (FALSE);
	
	return status;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS AllocateClientPdoList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do
	{
		if (!g_HidClientPdoList)
		{
			g_HidClientPdoList = (HID_DEVICE_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HID_DEVICE_LIST), 'ldih');	
			if (!g_HidClientPdoList)
			{
 				status = STATUS_UNSUCCESSFUL;
				break;
			}
			RtlZeroMemory(g_HidClientPdoList, sizeof(HID_DEVICE_LIST));
		} 
		if (!g_HidClientPdoList->head)
		{
			g_HidClientPdoList->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, NULL, 0);
			if (!g_HidClientPdoList->head)
			{
				ExFreePool(g_HidClientPdoList);
				g_HidClientPdoList = NULL;
				status = STATUS_UNSUCCESSFUL;
				break;
			}
		}
		status = STATUS_SUCCESS;
	} while (0);
	return status;
} 
//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitHidClientPdoList(
	_Out_ PHID_DEVICE_LIST* ClientPdoList,
	_Out_ PULONG		 ClientPdoListSize
)
{
 	PDRIVER_OBJECT	    pDriverObj = NULL;
	ULONG			  current_size = 0;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;

	do {   
		if (!NT_SUCCESS(AllocateClientPdoList()))
		{
			USB_DEBUG_INFO_LN_EX("Create List Error");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(InitClientPdoList(&ClientPdoListSize)))
		{
			USB_DEBUG_INFO_LN_EX("Init List Error");
			FreeHidClientPdoList();
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		 
		if (g_HidClientPdoList)
		{
			*ClientPdoList		= g_HidClientPdoList; 
			g_listSize		    = *ClientPdoListSize;

			g_HidClientPdoList->RefCount = 1; 
			USB_DEBUG_INFO_LN_EX("Create Success list: %I64x size: %x", g_HidClientPdoList, current_size);
			status = STATUS_SUCCESS;

			break;
		}

	} while (FALSE);
	return status;
}

//---------------------------------------------------------------------------------------------------------//
NTSTATUS FreeHidClientPdoList()
{
	NTSTATUS status = STATUS_SUCCESS;
	//Free White List
	if (g_HidClientPdoList)
	{
		if (g_HidClientPdoList->head)
		{
			CHAINLIST_SAFE_FREE(g_HidClientPdoList->head);
		}
		ExFreePool(g_HidClientPdoList);
		g_HidClientPdoList = NULL;
	}

	return status;
} 

//---------------------------------------------------------------------------------------------------------//
