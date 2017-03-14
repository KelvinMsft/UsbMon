                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #include "UsbHid.h"
#include "UsbUtil.h" 
#include "WinParse.h"
#include "ReportUtil.h"
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
HID_DEVICE_LIST*  g_hid_client_pdo_list = NULL; 
ULONG			  g_listSize = NULL;
HIDP_DEVICE_DESC* g_hid_collection = NULL;

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
BOOLEAN  VerifyDevice(
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
NTSTATUS InitPdoListByHidDriver(
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


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
////  

//----------------------------------------------------------------------------------------//
LOOKUP_STATUS LookupPipeHandleCallback(
	PHID_DEVICE_NODE Data,
	void* Context
)
{

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
		USB_MON_DEBUG_INFO("Pdo->fdoExt: %I64x \r\n", fdoExt);
		USB_MON_DEBUG_INFO("Pdo->fdo: %I64x \r\n", fdoExt->fdo);
		USB_MON_DEBUG_INFO("Pdo->fdo DriverName: %ws \r\n", fdoExt->fdo->DriverObject->DriverName.Buffer);
		USB_MON_DEBUG_INFO("Pdo->CollectionIndex: %I64x \r\n", pdoExt->collectionIndex);
		USB_MON_DEBUG_INFO("Pdo->CollectionNum: %I64x \r\n", pdoExt->collectionNum);

		// = (HIDP_DEVICE_DESC*)((PUCHAR)fdoExt + 0x58);
		//ExAllocatePoolWithTag(NonPagedPool, sizeof(HIDP_DEVICE_DESC), 'pdih'); // = (HIDP_DEVICE_DESC*)((PUCHAR)fdoExt + 0x58);

		//USB_MON_DEBUG_INFO("[rawReportDescription] %I64x rawReportDescriptionLength: %xh \r\n", fdoExt->rawReportDescription, fdoExt->rawReportDescriptionLength);

		status = MyGetCollectionDescription(fdoExt->rawReportDescription, fdoExt->rawReportDescriptionLength, NonPagedPool, &hid_device_desc);

		DumpReport(&hid_device_desc);

		RtlMoveMemory(AllReport, &hid_device_desc, sizeof(HIDP_DEVICE_DESC));

	} while (FALSE);

	return status;
} 

//-------------------------------------------------------------------------------------------//
HID_DEVICE_NODE* CreateHidDeviceNode(
	_In_ PDEVICE_OBJECT device_object,
	_In_ HID_USB_DEVICE_EXTENSION* MiniExtension,
	_In_ HIDP_DEVICE_DESC* parsedReport
)
{
	HID_DEVICE_NODE* node = NULL;
	if (!device_object || !MiniExtension || !parsedReport)
	{
		return NULL;
	}

	node = ExAllocatePoolWithTag(NonPagedPool, sizeof(HID_DEVICE_NODE), 'edon');
	if (!node)
	{
		return NULL;
	}

	RtlZeroMemory(node, sizeof(HID_DEVICE_NODE));
	node->device_object  = device_object;
	node->mini_extension = MiniExtension;
	RtlMoveMemory(&node->parsedReport , parsedReport , sizeof(HIDP_DEVICE_DESC));

	return node;
}
 
//---------------------------------------------------------------------------------------------------------//
BOOLEAN  VerifyDevice(
	_In_  PDEVICE_OBJECT				   DeviceObject,
	_Out_ PHID_USB_DEVICE_EXTENSION*      MiniExtension
)
{
	HID_USB_DEVICE_EXTENSION*  pMiniExtension   = NULL;
	HIDCLASS_DEVICE_EXTENSION* pClassExtension  = NULL;
	WCHAR						DeviceName[256] = { 0 };
	GetDeviceName(DeviceObject, DeviceName);

	pClassExtension = (HIDCLASS_DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
	if (!pClassExtension)
	{
		return FALSE;
	}
 
	 
	//USB_MON_DEBUG_INFO("Extension_common: %I64X sizeof: %x \r\n", hid_common_extension, sizeof(HID_USB_DEVICE_EXTENSION));
	pMiniExtension = (HID_USB_DEVICE_EXTENSION*)pClassExtension->hidExt.MiniDeviceExtension;
	if (!pMiniExtension)
	{
		return FALSE;
	} 	
	 
	//DumpHidMiniDriverExtension(hid_common_extension); 

	//Device Name = HID_0000000X
	if (!pClassExtension->isClientPdo)
	{	
		*MiniExtension = NULL;
		return FALSE; 
	}
	 
	if ( pMiniExtension->InterfaceDesc->Class == 3 &&			//HidClass Device
		(pMiniExtension->InterfaceDesc->Protocol == 1 ||		//Keyboard
		 pMiniExtension->InterfaceDesc->Protocol == 2 ||		//Mouse
		 pMiniExtension->InterfaceDesc->Protocol == 0) )			
	{  
		USB_MON_DEBUG_INFO("---------------------------------------------------------------------------------------------------- \r\n");

		USB_MON_DEBUG_INFO("hid_common_extension: %I64x \r\n", pClassExtension);
		USB_MON_DEBUG_INFO("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws \r\n", DeviceObject, DeviceObject->DriverObject->DriverName.Buffer, DeviceName);
		USB_MON_DEBUG_INFO("collectionIndex: %x collectionNum: %x \r\n", pClassExtension->pdoExt.collectionIndex, pClassExtension->pdoExt.collectionNum);

		USB_MON_DEBUG_INFO("---------------------------------------------------------------------------------------------------- \r\n"); 
		*MiniExtension = pMiniExtension; 
		return TRUE;
	} 
	return FALSE;
}
 
//----------------------------------------------------------------------------------------------------------//
NTSTATUS VerifyHidDeviceObject(
	_In_ PDRIVER_OBJECT DriverObject,
	_Out_ ULONG* ClientPdoCount
)
{
	PDEVICE_OBJECT	device_object = DriverObject->DeviceObject;
	ULONG		HidClientPdoCount = 0;
	NTSTATUS			   status = STATUS_UNSUCCESSFUL;

	while (device_object)
	{
		HID_DEVICE_NODE*					node = NULL;
		HIDP_DEVICE_DESC				  report = { 0 };
		HID_USB_DEVICE_EXTENSION* mini_extension = NULL;

		if (!VerifyDevice(device_object, &mini_extension))
		{
			goto Next;
		}
		 
		if (!mini_extension)
		{
			goto Next;
		}

		status = GetAllReportByDeviceExtension(device_object->DeviceExtension, &report);
		if (!NT_SUCCESS(status))
		{
			goto Next;
		}

		node = CreateHidDeviceNode(device_object, mini_extension, &report);
		if (!node)
		{
			goto Next;
		}

		AddToChainListTail(g_hid_client_pdo_list->head, node);
		HidClientPdoCount++;

		USB_MON_DEBUG_INFO("Inserted one element: %I64x InferfaceDesc: %I64X device_object: %I64x \r\n", node->device_object, node->mini_extension, device_object);
		USB_MON_DEBUG_INFO("Added one element :%x \r\n", HidClientPdoCount);
Next:
		device_object = device_object->NextDevice;
	}

	if (HidClientPdoCount)
	{
		*ClientPdoCount = HidClientPdoCount;
		status = STATUS_SUCCESS;
	}

	return status;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitPdoListByHidDriver(
	_Out_ PULONG ListSize
)
{
	NTSTATUS		  	  status = STATUS_UNSUCCESSFUL; 
	PDRIVER_OBJECT driver_object = NULL;
	ULONG			 return_size = 0;

	do {
		if (!ListSize || !g_hid_client_pdo_list)
		{
			USB_MON_DEBUG_INFO("Empty Params \r\n");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(GetDriverObjectByName(HID_USB_DEVICE, &driver_object)))
		{
			USB_MON_DEBUG_INFO("Get Drv Obj Error \r\n");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!driver_object)
		{
			USB_MON_DEBUG_INFO("Empty DrvObj \r\n");
			status = STATUS_UNSUCCESSFUL; 
			break;
		}

		if (!NT_SUCCESS(VerifyHidDeviceObject(driver_object, &g_listSize)))
		{
			USB_MON_DEBUG_INFO("VerifyHidDeviceObject Error \r\n");
			status = STATUS_UNSUCCESSFUL;
			break; 
		}
		if (return_size > 0)
		{
			status = STATUS_SUCCESS;
			*ListSize = return_size;
		}

	} while (FALSE);
	
	return status;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS AllocatePdoList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do
	{
		if (!g_hid_client_pdo_list)
		{
			g_hid_client_pdo_list = (HID_DEVICE_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HID_DEVICE_LIST), 'ldih');
		}

		if (!g_hid_client_pdo_list)
		{
 			status = STATUS_UNSUCCESSFUL;
			break;
		}

		RtlZeroMemory(g_hid_client_pdo_list, sizeof(HID_DEVICE_LIST));

		g_hid_client_pdo_list->head = NewChainListHeaderEx(LISTFLAG_FTMUTEXLOCK | LISTFLAG_AUTOFREE, NULL, 0);
		if (!g_hid_client_pdo_list->head)
		{
			ExFreePool(g_hid_client_pdo_list);
			g_hid_client_pdo_list = NULL;
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		status = STATUS_SUCCESS;
	} while (0);
	return status;
}

//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitHidClientPdoList(
	_Out_ PHID_DEVICE_LIST* ClientPdoList,
	_Out_ PULONG ClientPdoListSize
)
{
 	PDRIVER_OBJECT	    pDriverObj = NULL;
	ULONG			  current_size = 0;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;

	do {  
		if (g_hid_client_pdo_list)
		{
			ClientPdoList = g_hid_client_pdo_list;
			ClientPdoList = g_listSize;
			g_hid_client_pdo_list->RefCount++;
			status = STATUS_SUCCESS;
			break;
		}

		if (!NT_SUCCESS(AllocatePdoList()))
		{
			USB_MON_DEBUG_INFO("Create List Error \r\n");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(InitPdoListByHidDriver(&current_size)))
		{
			USB_MON_DEBUG_INFO("Init List Error \r\n");
			FreeHidClientPdoList();
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		 
		if (current_size > 0 && g_hid_client_pdo_list)
		{
			*ClientPdoList		= g_hid_client_pdo_list;
			*ClientPdoListSize  = g_listSize;

			g_hid_client_pdo_list->RefCount = 1;

			USB_MON_DEBUG_INFO("Create Success\r\n");

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
	if (g_hid_client_pdo_list)
	{
		if (g_hid_client_pdo_list->head)
		{
			CHAINLIST_SAFE_FREE(g_hid_client_pdo_list->head);
		}
		ExFreePool(g_hid_client_pdo_list);
		g_hid_client_pdo_list = NULL;
	}
	return status;
} 

//---------------------------------------------------------------------------------------------------------//
