#include "UsbHid.h"
#include "UsbUtil.h"  
#include "Winparse.h"
#include "ReportUtil.h"   
#include "IrpHook.h"
#include "UsbType.h"
#define HIDP_MAX_UNKNOWN_ITEMS 4


/////////////////////////////////////////////////////////////////////////////////////
//// Type
//// 

typedef struct _PARAM
{
	BOOLEAN 					 Ret;
	USBD_PIPE_HANDLE	  PipeHandle;
}PARAM, *PPARAM;

typedef struct _HASH_TABLE
{
	ULONG_PTR ID;
	PVOID	  Value;
}HASHTABLE, *PHASHTABLE;

/////////////////////////////////////////////////////////////////////////////////////
//// Marco
//// 
#define ARRAY_SIZE						 100
#define HIDP_PREPARSED_DATA_SIGNATURE1	 'PdiH'
#define HIDP_PREPARSED_DATA_SIGNATURE2   'RDK '
#define  HASHSIZE 0x10000

/////////////////////////////////////////////////////////////////////////////////////
//// Global/Extern Variable 
//// 
HID_DEVICE_LIST*    g_HidClientPdoList = NULL;
BOOLEAN				g_IsHidModuleInit = FALSE;
HASHTABLE 			g_HidDevicePipeHashTable[HASHSIZE] = { 0 };

extern USERCONFIGEX* g_UserCfgEx;
/////////////////////////////////////////////////////////////////////////////////////
//// Prototype
//// 



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
NTSTATUS GetParsedReport(
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
For More Information between Cient PDO and class FDO, See Doc/ README.txt.

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
- Verify Hid Device Object, just simply call VerifyAndInsertIntoHidList and
walk through each device object in the HID device object stack.

Arguments:

DriverObject			- Hid Driver Object

ClientPdoCount			- Total of Client Pdo

Return Value:
NTSTATUS			- STATUS_SUCCESS				, if translate success
- STATUS_UNSUCCESSFUL or others	, if translate error

-----------------------------------------------------------------------------------*/
NTSTATUS VerifyAllHidDevice(
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
NTSTATUS AllocateClientPdoList();

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


//----------------------------------------------------------------------------------------//
ULONG GetIndexByID(ULONG_PTR ID)
{
	return ID % HASHSIZE;
}
//----------------------------------------------------------------------------------------//
void ClearHashById(ULONG_PTR ID)
{
	ULONG Index = GetIndexByID(ID);
	g_HidDevicePipeHashTable[Index].ID = 0;
	g_HidDevicePipeHashTable[Index].Value = NULL;
	USB_DEBUG_INFO_LN_EX("Clear - Index: %x Id: %I64x ", Index, ID);
}

//----------------------------------------------------------------------------------------//
void SetHash(ULONG_PTR ID, PVOID lpNode)
{
	ULONG Index = GetIndexByID(ID);
	g_HidDevicePipeHashTable[Index].ID = ID;
	g_HidDevicePipeHashTable[Index].Value = lpNode;
	USB_DEBUG_INFO_LN_EX("Add - Index: %x Id: %I64x ", Index, ID);
}

//----------------------------------------------------------------------------------------//
BOOLEAN GetHashIndexById(ULONG_PTR ID, PHID_DEVICE_NODE* lpNode)
{
	ULONG_PTR Index = GetIndexByID(ID);
	if (g_HidDevicePipeHashTable[Index].ID == ID)
	{
		*lpNode = (PHID_DEVICE_NODE)g_HidDevicePipeHashTable[Index].Value;
		return TRUE;
	}
	return FALSE;
}
//--------------------------------------------------------------------------------------------//
VOID ClearPipeHandleHashByHidNode(PHID_DEVICE_NODE HidNode)
{
	ULONG i = 0;
	USBD_PIPE_INFORMATION* PipeHandleFromWhiteLists = HidNode->mini_extension->InterfaceDesc->Pipes;
	ULONG 	NumberOfPipes = HidNode->mini_extension->InterfaceDesc->NumberOfPipes;
	for (i = 0; i < NumberOfPipes; i++)
	{
		USB_DEBUG_INFO_LN_EX("ppHandle: %I64x", PipeHandleFromWhiteLists[i].PipeHandle);
		ClearHashById((ULONG_PTR)PipeHandleFromWhiteLists[i].PipeHandle);
	}
}
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
PHID_DEVICE_NODE GetHidNodeByDeviceObject(
	_In_ PDEVICE_OBJECT DeviceObject
)
{
	USB_DEBUG_INFO_LN_EX("GetHidNodeByDeviceObject");
	return QueryFromChainListByCallback(g_HidClientPdoList->head, SearchHidNodeCallback, DeviceObject);
}

//--------------------------------------------------------------------------------------------//
NTSTATUS VerifyAndInsertIntoHidList(
	_In_ PDEVICE_OBJECT DeviceObject
)
{
	HID_DEVICE_NODE* node = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (!DeviceObject || !g_HidClientPdoList)
	{
		return status;
	}

	status = VerifyDevice(DeviceObject, &node);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (!node)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (AddToChainListTail(g_HidClientPdoList->head, node))
	{
		status = STATUS_SUCCESS;
		g_HidClientPdoList->currentSize++;
	}

	return status;
}

//--------------------------------------------------------------------------------------------//
NTSTATUS RemoveNodeFromHidList(
	_In_ PDEVICE_OBJECT DeviceObject
)
{
	PHID_DEVICE_NODE node = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!DeviceObject || !g_HidClientPdoList)
	{
		return status;
	}
	node = GetHidNodeByDeviceObject(DeviceObject);
	if (!node)
	{
		return status;
	}

	ClearPipeHandleHashByHidNode(node);

	if (DelFromChainListByPointer(g_HidClientPdoList->head, node))
	{
		status = STATUS_SUCCESS;
		g_HidClientPdoList->currentSize--;
	}

	return status;
}

//----------------------------------------------------------------------------------------//
LOOKUP_STATUS LookupPipeHandleCallback(
	PHID_DEVICE_NODE Data,
	PARAM* Context
)
{
	NTSTATUS								   status = STATUS_UNSUCCESSFUL;
	USBD_PIPE_HANDLE				PipeHandleFromUrb = NULL;
	USBD_PIPE_INFORMATION*	 PipeHandleFromWhiteLists = NULL;
	ULONG								NumberOfPipes = 0;
	ULONG 										   i = 0;
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

	PipeHandleFromUrb = Context->PipeHandle;
	PipeHandleFromWhiteLists = Data->mini_extension->InterfaceDesc->Pipes;
	NumberOfPipes = Data->mini_extension->InterfaceDesc->NumberOfPipes;

	for (i = 0; i < NumberOfPipes; i++)
	{
		if (PipeHandleFromUrb == PipeHandleFromWhiteLists[i].PipeHandle)
		{
			status = STATUS_SUCCESS;
			Context->Ret = TRUE;
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
	_In_ TChainListHeader* 		PipeListHeader,
	_In_ USBD_PIPE_HANDLE  		PipeHandle,
	_Out_ PHID_DEVICE_NODE*		lpNode
)
{
	PHID_DEVICE_NODE Node = NULL;
	if (GetHashIndexById((ULONG_PTR)PipeHandle, &Node))
	{
		USB_DEBUG_INFO_LN_EX("Get Value By Hash ");
		*lpNode = Node;
		return TRUE;
	}
	else
	{
		PARAM params = { FALSE , PipeHandle };
		Node = QueryFromChainListByCallback(PipeListHeader, LookupPipeHandleCallback, &params);
		if (Node && params.Ret)
		{
			*lpNode = Node;
			SetHash((ULONG_PTR)PipeHandle, Node);
			return TRUE;
		}
		else
		{
			*lpNode = NULL;
			SetHash((ULONG_PTR)PipeHandle, NULL);
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------------------------------------//
NTSTATUS GetParsedReport(
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
			USB_DEBUG_INFO_LN_EX("STATUS_INVALID_PARAMETER");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		pdoExt = &HidClassExtension->pdoExt;

		if (!pdoExt)
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		addr = (HIDCLASS_DEVICE_EXTENSION*)pdoExt->deviceFdoExt;
		if (!addr)
		{
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		fdoExt = &addr->fdoExt;
		if (!fdoExt)
		{
			status = STATUS_INVALID_PARAMETER;
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
	node->device_object = DeviceObject;
	node->mini_extension = MiniExtension;
	node->InputExtractData = NULL;
	RtlMoveMemory(&node->parsedReport, ParsedReport, sizeof(HIDP_DEVICE_DESC));

	return node;
}
//---------------------------------------------------------------------------------------------------------//
NTSTATUS VerifyDevice(
	_In_  PDEVICE_OBJECT DeviceObject,
	_Out_ PHID_DEVICE_NODE* Node)
{
	HIDP_DEVICE_DESC* ParsedReport = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PHID_USB_DEVICE_EXTENSION MiniExtension = NULL;
	HID_DEVICE_NODE* node = NULL;
	do {

		if (!DeviceObject || !g_HidClientPdoList)
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!IsHidTypeDevice(DeviceObject, &MiniExtension))
		{
			USB_DEBUG_INFO_LN_EX("IsHidTypeDevice");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		ParsedReport = (HIDP_DEVICE_DESC*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIDP_DEVICE_DESC), 'csed');
		if (!ParsedReport)
		{
			USB_DEBUG_INFO_LN_EX("Empty report");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		status = GetParsedReport(DeviceObject->DeviceExtension, ParsedReport);
		if (!NT_SUCCESS(status))
		{
			USB_DEBUG_INFO_LN_EX("GetParsedReport Failed");
			break;
		}

		node = CreateHidDeviceNode(DeviceObject, MiniExtension, ParsedReport);
		if (!node)
		{
			USB_DEBUG_INFO_LN_EX("CreateNode Failed");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		*Node = node;
		status = STATUS_SUCCESS;

	} while (FALSE);

	if (ParsedReport)
	{
		ExFreePool(ParsedReport);
		ParsedReport = NULL;
	}

	return status;
}
//---------------------------------------------------------------------------------------------------------//
BOOLEAN  IsHidTypeDevice(
	_In_  PDEVICE_OBJECT				   DeviceObject,
	_Out_ PHID_USB_DEVICE_EXTENSION*      MiniExtension
)
{
	HID_USB_DEVICE_EXTENSION*  pMiniExtension = NULL;
	HIDCLASS_DEVICE_EXTENSION* pClassExtension = NULL;
	WCHAR						DeviceName[256] = { 0 };
	BOOLEAN			 					   ret = FALSE;
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

		if (!g_UserCfgEx)
		{

		}
		//pMiniExtension->InterfaceDesc->Protocol == 1 &&  ||		//Keyboard
		if (pMiniExtension->InterfaceDesc->Class == 3 &&			//HidClass Device
			pMiniExtension->InterfaceDesc->Protocol == 2)//Mouse
		{
			GetDeviceName(DeviceObject, DeviceName);

			USB_DEBUG_INFO_LN_EX("----------------------------------------------------------------------------------------------------");
			USB_DEBUG_INFO_LN_EX("hid_common_extension: %I64x", pClassExtension);
			USB_DEBUG_INFO_LN_EX("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws", DeviceObject, DeviceObject->DriverObject->DriverName.Buffer, DeviceName);
			USB_DEBUG_INFO_LN_EX("collectionIndex: %x collectionNum: %x", pClassExtension->pdoExt.collectionIndex, pClassExtension->pdoExt.collectionNum);
			USB_DEBUG_INFO_LN_EX("----------------------------------------------------------------------------------------------------");

			*MiniExtension = pMiniExtension;
			ret = TRUE;
			break;
		}

	} while (FALSE);

	return ret;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS VerifyAllHidDevice(
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
		if (NT_SUCCESS(VerifyAndInsertIntoHidList(DeviceObject)))
		{
			WCHAR DeviceObjectName[256] = { 0 };
			GetDeviceName(DeviceObject, DeviceObjectName);
			USB_DEBUG_INFO_LN_EX("DeviceObject: %ws", DeviceObjectName);
			HidClientPdoCount++;
		}
		DeviceObject = DeviceObject->NextDevice;
	}

	if (HidClientPdoCount)
	{
		*ClientPdoCount = HidClientPdoCount;
		status = STATUS_SUCCESS;
	} 

	USB_DEBUG_INFO_LN_EX("HidClientPdoCount: %x", HidClientPdoCount);
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

		if (!NT_SUCCESS(VerifyAllHidDevice(HidDriverObj, ListSize)))
		{
			USB_DEBUG_INFO_LN_EX("VerifyHidDeviceObject Error");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

	} while (FALSE);

	return status;
}
//----------------------------------------------------------------------------------------------------------//
ULONG   __fastcall FreeHidNodeCallback(PHID_DEVICE_NODE Header, ULONG Act)
{
	if (Act == CLIST_ACTION_FREE)
	{
		if (Header->InputExtractData)
		{
			ExFreePool(Header->InputExtractData);
			Header->InputExtractData = NULL;
		}
	}
	return 0;
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
			g_HidClientPdoList->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, FreeHidNodeCallback, 0);
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
NTSTATUS UnInitHidSubSystem()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!g_IsHidModuleInit)
	{
		USB_DEBUG_INFO_LN_EX("Module Already Free");
		return status;
	}

	status = FreeHidClientPdoList();

	if (NT_SUCCESS(status))
	{
		g_IsHidModuleInit = FALSE;
	}

	return status;
}

//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitHidSubSystem(
	_Out_ PULONG ClientPdoListSize)
{
	PDRIVER_OBJECT	    pDriverObj = NULL;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;

	do {
		if (g_IsHidModuleInit)
		{
			USB_DEBUG_INFO_LN_EX("Module Already Init");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(AllocateClientPdoList()))
		{
			USB_DEBUG_INFO_LN_EX("Create List Error");
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(InitClientPdoList(ClientPdoListSize)))
		{
			USB_DEBUG_INFO_LN_EX("Init List Error");
			FreeHidClientPdoList();
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (g_HidClientPdoList)
		{
			g_HidClientPdoList->RefCount = 1;
			USB_DEBUG_INFO_LN_EX("Create Success list: %I64x ", g_HidClientPdoList);
			g_IsHidModuleInit = TRUE;
			status = STATUS_SUCCESS;
			break;
		}
	} while (FALSE);
	return status;
}

//---------------------------------------------------------------------------------------------------------//
NTSTATUS FreeHidClientPdoList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	//Free White List

	if (g_HidClientPdoList)
	{
		if (g_HidClientPdoList->head)
		{
			CHAINLIST_SAFE_FREE(g_HidClientPdoList->head);
		}
		ExFreePool(g_HidClientPdoList);
		g_HidClientPdoList = NULL;
		status = STATUS_SUCCESS;
	}

	return status;
}

//---------------------------------------------------------------------------------------------------------//
