                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #include "UsbHid.h"
#include "UsbUtil.h"  
#include "Winparse.h"
#include "ReportUtil.h"   
#include "IrpHook.h"
#include "UsbType.h"
#include "HidHijack.h"
#include "OpenLoopBuffer.h"


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
#define HASHSIZE 0x10000
#define HIDP_MAX_UNKNOWN_ITEMS 				4  
/////////////////////////////////////////////////////////////////////////////////////
//// Global/Extern Variable 
//// 
USB_HUB_LIST*		g_UsbHubList	   = NULL;			// container of USBHUBNODE  - APC_LEVEL List
HID_DEVICE_LIST*    g_HidClientPdoList = NULL;  		// container of hid device -- mouse pipe handle APC_LEVEL List
BOOLEAN				g_IsHidModuleInit  = FALSE; 		// it init 
HASHTABLE 			g_ThirdPartyHidDevice[HASHSIZE] = {0};
HASHTABLE 			g_HidDevicePipeHashTable[HASHSIZE] = {0};
HASHTABLE 			g_UsbHubDrvObjHashTable[HASHSIZE] = {0};
BOOLEAN 			g_UsbHubListInit   = FALSE;
BOOLEAN				g_IsReportedVmware = FALSE;
BOOLEAN				g_IsReportedKeyboard = FALSE;
BOOLEAN				g_IsReportedMouse = FALSE;
BOOLEAN				g_bIsHidEnumerated = FALSE;
 
/////////////////////////////////////////////////////////////////////////////////////
//// Prototype
//// 
 
PDEVICE_OBJECT IoGetLowerDeviceObject(IN PDEVICE_OBJECT  DeviceObject); 
  
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
	_In_  PDEVICE_OBJECT				  DeviceObject,	
	_In_  HIDCONTEXT*					  RequiredDevice,
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
	_In_  HIDCONTEXT*	Context,
	_Out_ ULONG*		ClientPdoCount
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
	_In_  HIDCONTEXT*  RequiredDevice,
	_Out_ PULONG 	   ListSize
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
	_In_  HIDCONTEXT*	 RequiredDevice,
	_Out_ PHID_DEVICE_NODE* Node
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
NTSTATUS AllocateUsbHubList();


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Verify a Device is it hid device and add it into linked list

Arguments:

		DeviceObject		- Device to be verify

Return Value:

		NTSTATUS			- STATUS_SUCCESS				, if verify device success
							- STATUS_UNSUCCESSFUL or others	, if verify device error

-----------------------------------------------------------------------------------*/
NTSTATUS FreeUsbHubList();


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

	- Enumerate all hid mini driver
	
Arguments:

	 fdoExt - Device Extension of Fdo (_HID000000x)

Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
			 - STATUS_XXXXXXXXXXXX   , Depended on old completion
-----------------------------------------------------------------------------------*/ 
NTSTATUS ReportAllHidMiniDriver(FDO_EXTENSION* fdoExt);

NTSTATUS GetAllUsbHub();


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
////
 
 
//------------------------------------------------------------------------------------------//
PDO_EXTENSION*	GetClientPdoExtension(
	_In_ HIDCLASS_DEVICE_EXTENSION* HidCommonExt
)
{
	PDO_EXTENSION* pdoExt  = NULL;
	if(!HidCommonExt)
	{
		return pdoExt;
	}	
	if(!HidCommonExt->isClientPdo)
	{ 		
		return pdoExt;
	}
	pdoExt = &HidCommonExt->pdoExt;
	return pdoExt ;
}

//-----------------------------------------------------------------------------------------//
FDO_EXTENSION* GetFdoExtByClientPdoExt(
	_In_ PDO_EXTENSION* pdoExt
)
{
	FDO_EXTENSION* fdoExt = NULL;
	if(!pdoExt)
	{
		return fdoExt;
	}
	if(!pdoExt->deviceFdoExt)
	{ 	
		return fdoExt;
	}
		
	fdoExt = &pdoExt->deviceFdoExt->fdoExt; 
	return fdoExt; 
}
 
//----------------------------------------------------------------------------------------//
ULONG GetIndexByID(ULONG_PTR ID, ULONG TableSize) 
{
   return ID % TableSize;
}
 //----------------------------------------------------------------------------------------//
void ClearHashById(HASHTABLE* HashTable, ULONG TableSize , ULONG_PTR ID)
{
	ULONG Index = GetIndexByID(ID, TableSize); 
	HashTable[Index].ID = 0;
	HashTable[Index].Value = NULL; 
	USB_DEBUG_INFO_LN_EX("Clear - Index: %x Id: %I64x ", Index, ID);
}

//----------------------------------------------------------------------------------------//
void SetHash(HASHTABLE* HashTable, ULONG TableSize, ULONG_PTR ID, PVOID lpNode)
{
	ULONG Index = GetIndexByID(ID, TableSize); 
	HashTable[Index].ID = ID;
	HashTable[Index].Value = lpNode;
	USB_DEBUG_INFO_LN_EX("Add - Index: %x Id: %I64x ", Index, ID);
}

//----------------------------------------------------------------------------------------//
BOOLEAN GetHashIndexById(HASHTABLE* HashTable, ULONG TableSize, ULONG_PTR ID, PVOID* lpNode)
{
	ULONG_PTR Index = GetIndexByID(ID, TableSize); 
	if(HashTable[Index].ID == ID)
	{
		*lpNode =  HashTable[Index].Value;
		return TRUE;
	}	
	return FALSE;
}
//--------------------------------------------------------------------------------------------//
VOID ClearPipeHandleHashByHidNode(PHID_DEVICE_NODE HidNode)
{
	ULONG i = 0;
	USBD_PIPE_INFORMATION* PipeHandleFromWhiteLists = HidNode->mini_extension->InterfaceDesc->Pipes;
	ULONG 	NumberOfPipes =  HidNode->mini_extension->InterfaceDesc->NumberOfPipes;    
	for (i = 0; i < NumberOfPipes; i++)
	{
		USB_DEBUG_INFO_LN_EX("ppHandle: %I64x", PipeHandleFromWhiteLists[i].PipeHandle);
		ClearHashById(g_HidDevicePipeHashTable, HASHSIZE ,(ULONG_PTR)PipeHandleFromWhiteLists[i].PipeHandle); 
	} 
}
//--------------------------------------------------------------------------------------------//
ULONG __fastcall SearchHidNodeCallback(
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
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ HIDCONTEXT*	Context
)
{
	HID_DEVICE_NODE* node = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (!DeviceObject || !g_HidClientPdoList)
	{
		return status;
	}
	 
	status = VerifyDevice(DeviceObject, Context,  &node);
	
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
LOOKUP_STATUS __fastcall LookupPipeHandleCallback(
	PHID_DEVICE_NODE Data,
	PARAM* Context
)
{	NTSTATUS								   status = STATUS_UNSUCCESSFUL;
	USBD_PIPE_HANDLE				PipeHandleFromUrb = NULL;
	USBD_PIPE_INFORMATION*	 PipeHandleFromWhiteLists = NULL;
	ULONG								NumberOfPipes = 0;
	ULONG 										   i  = 0 ;
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
	USB_DEBUG_INFO_LN_EX("PipeDevice: %p", Data->device_object);
	for (i = 0; i < NumberOfPipes; i++)
	{ 
		if (PipeHandleFromUrb == PipeHandleFromWhiteLists[i].PipeHandle)
		{	
			USB_DEBUG_INFO_LN_EX("Found Pipe: %I64x %I64x",PipeHandleFromUrb, PipeHandleFromWhiteLists[i].PipeHandle); 
			status = STATUS_SUCCESS;
			Context->Ret = TRUE;
			break;
		}
		USB_DEBUG_INFO_LN_EX("Try to find : %I64x %I64x",PipeHandleFromUrb, PipeHandleFromWhiteLists[i].PipeHandle); 
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
	BOOLEAN ret = GetHashIndexById(g_HidDevicePipeHashTable, HASHSIZE, (ULONG_PTR)PipeHandle, &Node) ;
	if(ret && Node)
	{
		USB_DEBUG_INFO_LN_EX("Get Value By Hash %I64x ", Node );
		*lpNode = Node;
		return TRUE;
	}
	else
	{ 	
		PARAM params = { FALSE , PipeHandle };
		Node = QueryFromChainListByCallback(PipeListHeader, LookupPipeHandleCallback, &params);  
		if(Node && params.Ret)
		{		 
			*lpNode = Node;
			SetHash(g_HidDevicePipeHashTable,HASHSIZE,(ULONG_PTR)PipeHandle, Node);
			return TRUE;
		}
		else
		{	
			*lpNode = NULL;
			SetHash(g_HidDevicePipeHashTable,HASHSIZE,(ULONG_PTR)PipeHandle, NULL);
		}
	}	 
	return FALSE;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS HidGetRawDescriptorByClientPdo(
	_In_  		DEVICE_OBJECT* 	 ClientPdo,
	_Out_ 		PCHAR*		 HidDescriptor,
	_Inout_opt_ ULONG*	 HidDescriptorHash
)
{
	HIDCLASS_DEVICE_EXTENSION*  HidClassExtension = NULL;
	PDO_EXTENSION* 				pdoExt = NULL;
	FDO_EXTENSION* 				fdoExt = NULL; 
	HIDCLASS_DEVICE_EXTENSION*  fdoBackPtr = NULL;
	ULONG						DescriptorHash = 0;
	ULONG 						i=0; 
	NTSTATUS 			   		status = STATUS_SUCCESS;
	CHAR* 						HidDesc = NULL;
	do
	{ 
		if (!ClientPdo || !HidDescriptor)
		{
			//USB_DEBUG_INFO_LN_EX("STATUS_INVALID_PARAMETER 1 ");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		HidClassExtension = ClientPdo->DeviceExtension;
		if(!HidClassExtension)
		{	
			//USB_DEBUG_INFO_LN_EX("STATUS_INVALID_PARAMETER2 ");
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		pdoExt = &HidClassExtension->pdoExt; 
		if (!pdoExt)
		{
			//USB_DEBUG_INFO_LN_EX("STATUS_INVALID_PARAMETER3 ");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		fdoBackPtr = (HIDCLASS_DEVICE_EXTENSION*)pdoExt->deviceFdoExt;
		if (!fdoBackPtr)
		{
			//USB_DEBUG_INFO_LN_EX("STATUS_INVALID_PARAMETER4");
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		fdoExt = &fdoBackPtr->fdoExt;
		if (!fdoExt)
		{	
			//USB_DEBUG_INFO_LN_EX("STATUS_INVALID_PARAMETER5");
			status = STATUS_INVALID_PARAMETER;
			break;
		} 
		   
		HidDesc = (CHAR*)ExAllocatePoolWithTag(NonPagedPool ,  fdoExt->rawReportDescriptionLength * 2 + 1 , 'rept' ); 
		if(!HidDesc)
		{	
			//USB_DEBUG_INFO_LN_EX("STATUS_INVALID_PARAMETER6");
			break;
		}

		RtlZeroMemory(HidDesc, fdoExt->rawReportDescriptionLength * 2 + 1);
		
//		binToBcd(fdoExt->rawReportDescription, fdoExt->rawReportDescriptionLength, HidDesc ,fdoExt->rawReportDescriptionLength * 2 + 1 );
			
		
		for(i = 0 ; i < fdoExt->rawReportDescriptionLength ; i++)
		{
			DescriptorHash = DescriptorHash + ((*HidDesc * i) ^ 0x32567711);
		} 
		
		//Free By Caller
		*HidDescriptor = HidDesc; 
		
		if(HidDescriptorHash)
		{
			*HidDescriptorHash = DescriptorHash;
		}
	
	} while (FALSE);

	return status;
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
	char* str = NULL;
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
		if(KeGetCurrentIrql() < DISPATCH_LEVEL)
		{
			USB_DEBUG_INFO_LN_EX("Pdo->fdo DriverName: %ws ", fdoExt->fdo->DriverObject->DriverName.Buffer);
		}
		else
		{
			USB_DEBUG_INFO_LN_EX("IRQL TOO HIGH ");
		}
	
		USB_DEBUG_INFO_LN_EX("Pdo->CollectionIndex: %I64x", pdoExt->collectionIndex);
		USB_DEBUG_INFO_LN_EX("Pdo->CollectionNum: %I64x", pdoExt->collectionNum);

		status = GetCollectionDescription(fdoExt->rawReportDescription, fdoExt->rawReportDescriptionLength, NonPagedPool, &hid_device_desc);//==(HIDP_DEVICE_DESC*)((PUCHAR)fdoExt + 0x58);
		
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
	node->device_object  		= DeviceObject;
	node->mini_extension 		= MiniExtension;
	node->ExtractedData[0]      = NULL;
	node->ExtractedData[1]      = NULL;
	node->ExtractedData[2]      = NULL;
	node->Collection			= NULL;
	
	RtlMoveMemory(&node->parsedReport , ParsedReport, sizeof(HIDP_DEVICE_DESC));

	return node;
} 
//---------------------------------------------------------------------------------------------------------//
NTSTATUS VerifyDevice(
	_In_  PDEVICE_OBJECT DeviceObject, 
	_In_  HIDCONTEXT*	 Context,
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

		if (!IsHidTypeDevice(DeviceObject, Context, &MiniExtension))
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
		
		RtlZeroMemory(ParsedReport, sizeof(HIDP_DEVICE_DESC));

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
	_In_  PDEVICE_OBJECT				  DeviceObject,
	_In_  HIDCONTEXT*					  Context,
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

		//USB_MON_DEBUG_INFO("Extension_common: %I64X sizeof: %x \r\n", HidCommonExt, sizeof(HID_USB_DEVICE_EXTENSION));
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
		
		//HidClass Device
		if(pMiniExtension->InterfaceDesc->Class	!= 3)
		{
			ret = FALSE;
			break;
		}
		
		if ((pMiniExtension->InterfaceDesc->Protocol == 2) && (Context->RequiredDevice & MOUSE_FLAGS))	 
		{
			USB_DEBUG_INFO_LN_EX("Found Mouse Device %p ", DeviceObject);
			if(Context->MouseCallback && DeviceObject->AttachedDevice)
			{
				ret = Context->MouseCallback(NULL);
				*MiniExtension = pMiniExtension;
			}	 
			ret = TRUE;
		} 
		if((pMiniExtension->InterfaceDesc->Protocol == 1) && (Context->RequiredDevice & KEYBOARD_FLAGS))
		{			
			USB_DEBUG_INFO_LN_EX("Found Keyboard Device %p ", DeviceObject);
			if(Context->KeyboardCallback && DeviceObject->AttachedDevice )
			{
				ret = Context->KeyboardCallback(NULL);
				*MiniExtension = pMiniExtension;
			}
			ret = TRUE;
		}
		
		USB_DEBUG_INFO_LN_EX("----------------------------------------------------------------------------------------------------");
		USB_DEBUG_INFO_LN_EX("Protocol: %x ",pMiniExtension->InterfaceDesc->Protocol);
		USB_DEBUG_INFO_LN_EX("DeviceObject: %I64x HidCommonExt: %I64x", DeviceObject, pClassExtension); 
		if(KeGetCurrentIrql() < DISPATCH_LEVEL)
		{ 
			GetDeviceName(DeviceObject, DeviceName);
			USB_DEBUG_INFO_LN_EX("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws", DeviceObject, DeviceObject->DriverObject->DriverName.Buffer, DeviceName);
		}
		else
		{
			USB_DEBUG_INFO_LN_EX("IRQL TOO HIGH ");
		}

		USB_DEBUG_INFO_LN_EX("collectionIndex: %x collectionNum: %x", pClassExtension->pdoExt.collectionIndex, pClassExtension->pdoExt.collectionNum);
		USB_DEBUG_INFO_LN_EX("----------------------------------------------------------------------------------------------------");

			
		if(pMiniExtension->InterfaceDesc->Class	== 3 &&			//HidClass Device
		   pMiniExtension->InterfaceDesc->Protocol == 0 &&
		   !g_IsReportedVmware)
		{
			//VMWare 
			g_IsReportedVmware = TRUE;
		}
	
		if(pMiniExtension->InterfaceDesc->Class	== 3 &&			//HidClass Device
		   pMiniExtension->InterfaceDesc->Protocol == 1 &&
		   !g_IsReportedKeyboard)
		{
			//VMWare 
			g_IsReportedKeyboard = TRUE;
		}
		
		
		if(pMiniExtension->InterfaceDesc->Class	== 3 &&			//HidClass Device
		   pMiniExtension->InterfaceDesc->Protocol == 2 &&
		   !g_IsReportedMouse)
		{
			//VMWare 
			g_IsReportedMouse = TRUE;
		}
		
	} while (FALSE);

	return ret;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS VerifyAllHidDevice(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_  HIDCONTEXT*	Context,
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
		if(NT_SUCCESS(VerifyAndInsertIntoHidList(DeviceObject, Context)))
		{ 
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
	_In_  HIDCONTEXT*  Context,
	_Out_ PULONG 	  ListSize
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

		
		if (!NT_SUCCESS(VerifyAllHidDevice(HidDriverObj, Context , ListSize)))
		{
			USB_DEBUG_INFO_LN_EX("VerifyHidDeviceObject Error");
			status = STATUS_UNSUCCESSFUL;
			break; 
		}  
	} while (FALSE);
	
	if(HidDriverObj)
	{
		ReleaseDriverObject(HidDriverObj);
		HidDriverObj = NULL;
	}
	
	return status;
}
//----------------------------------------------------------------------------------------------------------//
ULONG   __fastcall FreeHidNodeCallback(PHID_DEVICE_NODE Header,ULONG Act)
{
	if(Act == CLIST_ACTION_FREE)
	{
		if(Header->ExtractedData[HidP_Input])
		{
			ExFreePool(Header->ExtractedData[HidP_Input]);
			Header->ExtractedData[HidP_Input] = NULL;
		}
		
		if(Header->PreviousUsageList)
		{
			ExFreePool(Header->PreviousUsageList);
			Header->PreviousUsageList = NULL;
		}
	}
	return 0 ;
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
NTSTATUS AllocateUsbHubList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do
	{
		if (!g_UsbHubList)
		{
			g_UsbHubList = (USB_HUB_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(USB_HUB_LIST), 'ldih');	
			if (!g_UsbHubList)
			{
 				status = STATUS_UNSUCCESSFUL;
				break;
			}
			RtlZeroMemory(g_UsbHubList, sizeof(USB_HUB_LIST));
		} 
		if (!g_UsbHubList->head)
		{
			g_UsbHubList->head = NewChainListHeaderEx(LISTFLAG_FTMUTEXLOCK | LISTFLAG_AUTOFREE, NULL, 0);
			if (!g_UsbHubList->head)
			{
				ExFreePool(g_UsbHubList);
				g_UsbHubList = NULL;
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
		USB_DEBUG_INFO_LN_EX("UsbHid Resource Already Freed");
		return status;
	}

	status = FreeHidClientPdoList();
	
	status = FreeUsbHubList();
	
	g_IsHidModuleInit = FALSE;
	
	return status;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS CreateUsbHubNode(
	_In_  PDRIVER_OBJECT DriverObject,
	_Out_ PUSBHUBNODE* Node
)
{
	USBHUBNODE* HubNode = NULL; 
	if(!Node || !DriverObject)
	{
		return STATUS_UNSUCCESSFUL;
	}
 
	HubNode = (USBHUBNODE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(USBHUBNODE), 'bnhu');  
	if(HubNode)
	{	
 		if(KeGetCurrentIrql() < DISPATCH_LEVEL)
		{
			USB_DEBUG_INFO_LN_EX("DriverObject->DriverName.Buffer: %ws", DriverObject->DriverName.Buffer);
		}
		else
		{
			USB_DEBUG_INFO_LN_EX("IRQL TOO HIGH ");
		}
		RtlZeroMemory(HubNode, sizeof(USBHUBNODE));
		HubNode->HubNameLen   =  DriverObject->DriverName.Length;
		wcsncpy(HubNode->HubName, DriverObject->DriverName.Buffer, DriverObject->DriverName.Length);
		HubNode->HubDriverObject =  DriverObject; 
		HubNode->IsHooked		= FALSE;
	} 		 
	
	*Node = HubNode;
	
	return STATUS_SUCCESS;
}
//--------------------------------------------------------------------------------------------//
NTSTATUS AddUsbHubToList(
	_In_ PDRIVER_OBJECT HubDriverObj
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PVOID Value = NULL; 
	if(!HubDriverObj || !g_UsbHubList)
	{
		return status; 
	}
	
	if(!GetHashIndexById(g_UsbHubDrvObjHashTable, HASHSIZE, (ULONG_PTR)HubDriverObj, &Value))
	{
		CHAR DriverName[256]= {0};
		USBHUBNODE* HubNode = NULL;			
		status = CreateUsbHubNode(HubDriverObj, &HubNode); 
		if(HubNode && NT_SUCCESS(status))
		{
			SetHash(g_UsbHubDrvObjHashTable, HASHSIZE, (ULONG_PTR)HubDriverObj, HubDriverObj);
			if(AddToChainListTail(g_UsbHubList->head, HubNode))
			{
#ifdef DBG		
				if(KeGetCurrentIrql() < DISPATCH_LEVEL)
				{ 
					USB_DEBUG_INFO_LN_EX("Insert Success --- DriverName: %ws DriverObject: %I64x ", HubNode->HubName, HubNode->HubDriverObject);
				}
				else
				{
					USB_DEBUG_INFO_LN_EX("IRQL TOO HIGH ");
				}			
 #endif 					
					 
				g_UsbHubList->currentSize++;
					
				status = STATUS_SUCCESS; 
			}	 
		}
	}
	else
	{
		status = STATUS_SUCCESS;
	}
	
	return status; 
}
//--------------------------------------------------------------------------------------------//
NTSTATUS GetLowestDeviceObject(
	_In_  PDEVICE_OBJECT   DeviceObject,
	_Out_ PDEVICE_OBJECT* LowestDeviceObject
)
{
	WCHAR DeviceName[256] ={0};
	PDEVICE_OBJECT BackupDeviceObject = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL; 
	if(!DeviceObject || !LowestDeviceObject)
	{
		return status; 
	}
	
	while(DeviceObject)
	{		 
		BackupDeviceObject	= DeviceObject;
#ifdef DBG
		if(KeGetCurrentIrql() < DISPATCH_LEVEL)
		{
			RtlZeroMemory(DeviceName, 256) ; 
			GetDeviceName(BackupDeviceObject, DeviceName);
			USB_DEBUG_INFO_LN_EX("UsbHub Stack: %ws DriverName: %ws", DeviceName, BackupDeviceObject->DriverObject->DriverName.Buffer);
		}
		else
		{
			USB_DEBUG_INFO_LN_EX("IRQL TOO HIGH ");
		}
#endif	
		DeviceObject = IoGetLowerDeviceObject(BackupDeviceObject);   
		if (DeviceObject)
	    {
	       ObDereferenceObject(DeviceObject);
	    }
	} 
	
	if(!DeviceObject)
	{
		*LowestDeviceObject = BackupDeviceObject;
		status = STATUS_SUCCESS;
	}
	
	return status ;
}
//--------------------------------------------------------------------------------------------//
/*
	
	Case 1: 						 
									 
		 ----------------		 
		| HID Client PDO |		 
		 ----------------        
				 |               
		------------------       
		| HID CLASS FDO  |       
		------------------       
				 |               
		------------------       
		|    UsbHub Fdo  |       
		------------------       
		                         
	

	Case 2 :	
	
		 ----------------		
		| HID Client PDO |		
         ----------------
        		 |
        ------------------
        | HID CLASS FDO  |
        ------------------
        		 |
        ------------------	 Next 	------------------	Next   ------------------
        |  UsbCcgp C-Pdo  | -----> |  UsbCcgp C-Pdo2  | ----> |  UsbCcgp C-Pdo3  |	
        ------------------          ------------------         ------------------ 
        		 /
       	------------------  
        | ccgp class Fdo1 |  <--- created by UsbHub  
       	------------------ 
				 | 			 <--- attach to Usbhub (AddDevice)
		------------------ 
		|   UsbHub's Fdo  |
		------------------ 
		
See: https://msdn.microsoft.com/en-us/library/windows/hardware/ff551670(v=vs.85).aspx


IRQL:	APC_LEVEL 

*/                      
ULONG __fastcall SearchUsbhubFromHidNodeCallback(
	_In_ PHID_DEVICE_NODE HidNode,
	_In_ void* 		  	  context
)
{ 
	
	PDRIVER_OBJECT UsbCcgpDO  = NULL ;   
	WCHAR DeviceName[256] = {0}; 
	PDEVICE_OBJECT HidClientPdo 		= NULL;
	PDEVICE_OBJECT HidFdo				= NULL;
	PDEVICE_OBJECT BackupDeviceObject	= NULL; 
	PDEVICE_OBJECT HubDeviceObject  	= NULL; 
	PDO_EXTENSION* PdoExt = NULL;
	FDO_EXTENSION* FdoExt = NULL;
	HIDCLASS_DEVICE_EXTENSION* 	HidCommonExt = NULL;
	
	if(context){}
	
	GetDriverObjectByName(L"\\Driver\\Usbccgp", &UsbCcgpDO);
	
	if(!g_UsbHubList || !HidNode->device_object)
	{
		return CLIST_FINDCB_RET;
	}
  
	//1. HID Basic 
	HidClientPdo = HidNode->device_object; //Hid Client Pdo
	
	HidCommonExt = (HIDCLASS_DEVICE_EXTENSION*)(HidNode->device_object->DeviceExtension);
	
	if(!HidCommonExt)
	{
		return CLIST_FINDCB_CTN;
	}
	
	PdoExt = GetClientPdoExtension(HidCommonExt);
	if(!PdoExt)
	{
		return CLIST_FINDCB_CTN;
	}
	
	FdoExt = GetFdoExtByClientPdoExt(PdoExt);
	if(!FdoExt)
	{
		return CLIST_FINDCB_CTN;
	}
	
	//1. Get Each Hid Fdo which we need to monitor  (name : HID_0000000X)
	HidFdo = FdoExt->fdo ; 
	
	//2. Get each HID lower-level device	
	if(!NT_SUCCESS(GetLowestDeviceObject(HidFdo, &BackupDeviceObject)))
	{
		return CLIST_FINDCB_CTN;
	}
	
	if(!BackupDeviceObject)
	{
		return CLIST_FINDCB_CTN;
	}
	
	//3. If Lowest level is ccgp ,  continue to get hub
	//case 2
 	if(BackupDeviceObject->DriverObject == UsbCcgpDO)
	{
		PDEVICE_OBJECT UsbCcgpClientPdo = UsbCcgpDO->DeviceObject;		
		
		//4. Enumerate Ccgp Device list
		while(UsbCcgpClientPdo)
		{		
			//change meaning
			PDEVICE_OBJECT UsbCcgpParentFdo = UsbCcgpClientPdo;  
			
			//5. Not UsbHub FDO , skip
			if(UsbCcgpClientPdo->AttachedDevice != NULL)
			{					
				//try to find next usb hub.
				UsbCcgpClientPdo = UsbCcgpClientPdo->NextDevice;  		
				continue;
			}
			
			//Found UsbHub FDO
			//6. enumerate its device stack unti the end , then it is going to UsbHub.
			if(!NT_SUCCESS(GetLowestDeviceObject(UsbCcgpParentFdo, &BackupDeviceObject)))
			{		
				UsbCcgpClientPdo = UsbCcgpClientPdo->NextDevice;  		
				continue;
			}
	
			AddUsbHubToList(BackupDeviceObject->DriverObject);
			g_UsbHubListInit = TRUE;
			
			//try to find next usb hub.
			UsbCcgpClientPdo = UsbCcgpClientPdo->NextDevice;  			
		}  
	}
	//case 1
	else
	{  
		AddUsbHubToList(BackupDeviceObject->DriverObject);
		g_UsbHubListInit = TRUE;
	}
	
	if(UsbCcgpDO)
	{
		ReleaseDriverObject(UsbCcgpDO);
		UsbCcgpDO = NULL;
	}
	return CLIST_FINDCB_CTN;
}

//-----------------------------------------------------------------------------------------------------------//
NTSTATUS GetAllUsbHub()
{ 	    
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	
	USB_DEBUG_INFO_LN_EX("Enum GetAllUsbHub... "); 
	
	QueryFromChainListByCallback(g_HidClientPdoList->head, SearchUsbhubFromHidNodeCallback, NULL);
	
	if(g_UsbHubListInit)
	{	
		status = STATUS_SUCCESS; 
	}
	return status;
} 

//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitHidSubSystem(
	_In_  HIDCONTEXT* Context,
	_Out_ PULONG 	  ClientPdoListSize)
{
 	PDRIVER_OBJECT	    pDriverObj = NULL;
	NTSTATUS			status = STATUS_UNSUCCESSFUL;

	do {   
		if (g_IsHidModuleInit)
		{
			USB_DEBUG_INFO_LN_EX("Module Already Init");  
			status = STATUS_SUCCESS;
			break; 
		}

		if (!NT_SUCCESS(AllocateClientPdoList()))
		{
			USB_DEBUG_INFO_LN_EX("Create List Error");
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		
		if(!NT_SUCCESS(AllocateUsbHubList()))
		{				
			FreeUsbHubList();
			FreeHidClientPdoList(); 
			USB_DEBUG_INFO_LN_EX("Create List Error");
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		if (!NT_SUCCESS(InitClientPdoList(Context, ClientPdoListSize)))
		{
			USB_DEBUG_INFO_LN_EX("Init HID List Error"); 
			FreeUsbHubList();
			FreeHidClientPdoList();
			status = STATUS_UNSUCCESSFUL;
			break;
		} 
	
		if (!NT_SUCCESS(GetAllUsbHub()))
		{			
			USB_DEBUG_INFO_LN_EX("Init USBhub List Error"); 
			FreeUsbHubList();
			FreeHidClientPdoList();
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		
		if (g_HidClientPdoList && g_UsbHubList->currentSize > 0 )
		{
			g_HidClientPdoList->RefCount = 1; 
			USB_DEBUG_INFO_LN_EX("Create Success list: %I64x currentSize: %x ", g_HidClientPdoList, g_UsbHubList->currentSize );
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
		g_HidClientPdoList	  = NULL; 
		status = STATUS_SUCCESS;
	}

	return status;
} 
//---------------------------------------------------------------------------------------------------------//
NTSTATUS FreeUsbHubList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	//Free White List
 
	if (g_UsbHubList)
	{
		if (g_UsbHubList->head)
		{
			CHAINLIST_SAFE_FREE(g_UsbHubList->head);
		}
		ExFreePool(g_UsbHubList);
		g_UsbHubList	  = NULL; 
		status = STATUS_SUCCESS;
		
		USB_DEBUG_INFO_LN_EX("Free Hub List ");
	}
	  
	RtlZeroMemory(g_UsbHubDrvObjHashTable, HASHSIZE);
	USB_DEBUG_INFO_LN_EX("Free Hub hash table ");
	
	RtlZeroMemory(g_HidDevicePipeHashTable, HASHSIZE);
	USB_DEBUG_INFO_LN_EX("Free Device hash table ");

	RtlZeroMemory(g_ThirdPartyHidDevice	, HASHSIZE);
	USB_DEBUG_INFO_LN_EX("Free Device hash table ");
	
	g_UsbHubListInit = FALSE;
	
	return status;
} 
//---------------------------------------------------------------------------------------------------------// 