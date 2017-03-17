#include <fltKernel.h>
#include "UsbUtil.h" 
#include "UsbHid.h" 
#include "IrpHook.h"
#include "ReportUtil.h"
 
#pragma warning (disable : 4100) 
 

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Types
////  

typedef struct PENDINGIRP
{
	PIRP						  Irp;			//Irp
	PIO_STACK_LOCATION		 IrpStack;			//Old IRP stack
	PVOID					oldContext;			//Old IRP routine argument
	IO_COMPLETION_ROUTINE*	oldRoutine;			//Old IRP routine address
}PENDINGIRP, *PPENDINGIRP;

typedef struct HIJACK_CONTEXT
{
	PDEVICE_OBJECT		   DeviceObject;		// Hid deivce
	PURB							urb;		// Urb packet saved in IRP hook
	HID_DEVICE_NODE*			   node;		// An old context we need 
	PENDINGIRP*			    pending_irp;		// pending irp node for cancellation
}HIJACK_CONTEXT, *PHIJACK_CONTEXT;


typedef struct _PENDINGIRP_LIST
{
	TChainListHeader*	head;
	ULONG			RefCount;
}PENDINGIRPLIST, *PPENDINGIRPLIST;

 

///////////////////////////////////////////////////////////////////////////////////////////////
////	Marco Utilities
////  
//// 

#define IsSafeNode(x)					 (x->node)
//Hijack Context Marco
#define GetCollectionDesc(x, y)			 &x->node->parsedReport.CollectionDesc[y]
#define IsMouseUsage(x, y)				 (x->node->parsedReport.CollectionDesc[y].Usage==HID_MOU_USAGE)
#define IsKeyboardUsage(x, y)			 (x->node->parsedReport.CollectionDesc[y].Usage==HID_KBD_USAGE)
#define IsDesktopUsagePage(x, y)		 (x->node->parsedReport.CollectionDesc[y].UsagePage==HID_GENERIC_DESKTOP_PAGE)

//urb Marco
#define UrbGetFunction(urb)				 (urb->UrbHeader.Function)
#define UrbGetTransferBuffer(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferBuffer)
#define UrbGetTransferFlags(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferFlags)
#define UrbGetTransferLength(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferBufferLength) 
#define UrbGetTransferPipeHandle(urb)	 (urb->UrbBulkOrInterruptTransfer.PipeHandle)
#define UrbIsInputTransfer(urb)			 (UrbGetTransferFlags(urb) & (USBD_TRANSFER_DIRECTION_IN ))
#define UrbIsOutputTransfer(urb)		 (UrbGetTransferFlags(urb) & (USBD_TRANSFER_DIRECTION_OUT | USBD_DEFAULT_PIPE_TRANSFER))

#define GetNodeByPTR(ListHead,NodeValue) QueryFromChainListByULONGPTR(ListHead, (ULONG_PTR)NodeValue);	  
#define GetRealPendingIrpByIrp(irp)		 GetNodeByPTR(g_PendingIrpList->head, irp)

///////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////
////
DRIVER_DISPATCH*  g_pDispatchInternalDeviceControl = NULL;
volatile LONG	  g_HidPipeListSize				   = 0;
BOOLEAN			  g_bUnloaded					   = FALSE;
PENDINGIRPLIST*	  g_PendingIrpList			   = NULL;
PHID_DEVICE_LIST  g_HidPipeList					   = NULL;

 
/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 
////

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		 Allocated Pending IRP Linked-List memory 
		 for saving any IRP-related information

Arguments:
		
		No

Return Value:

		NTSTATUS	- STATUS_SUCCESS if initial successfully

					- STATUS_UNSUCCESSFUL if failed

-------------------------------------------------------*/
NTSTATUS		AllocatePendingIrpLinkedList();

 
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Free Pending IRP list memory and each node

Arguments:

		No

Return Value:

		NTSTATUS	- STATUS_SUCCESS if Free Memory 
					  successfully

					- STATUS_UNSUCCESSFUL if failed

-------------------------------------------------------*/
NTSTATUS		FreePendingIrpList();


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Lookup Pending IRP callback, in callback, we 
		recovered the completion hook 
		 
Arguments:

		pending_irp_node - Each node inside IRP pending list

		context			 - Not used 

Return Value:

		LOOKUP_STATUS	 - CLIST_FINDCB_CTN, traverse whole 
						   list
-------------------------------------------------------*/
LOOKUP_STATUS	LookupPendingIrpCallback(
	_In_ PENDINGIRP* pending_irp_node,
	_In_ PVOID		 context
); 
  
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Remove Pending IRP Linked-List for saving any
		IRP we hooked, and set a Lookup callback
		(LookupPendingIrpCallback)

Arguments:

		No

Return Value:

	NTSTATUS	- STATUS_SUCCESS if recover successfully
				- STATUS_UNSUCCESSFUL if failed
-------------------------------------------------------*/
NTSTATUS		RecoverAllPendingIrp();


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

		NTSTATUS	- STATUS_SUCCESS if Handle successfully
					- STATUS_UNSUCCESSFUL if failed
-------------------------------------------------------*/
NTSTATUS HandleKeyboardData(
	_In_ HIJACK_CONTEXT* pContext, 
	_In_ HIDP_REPORT_TYPE reportType
);


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		Handling And Extract data from mouse URB 

Arguments:

		pContext	- Hijack Context included all we need
					  see HIJACK_CONTEXT declaration.

		reportType	- Hid Report Type
						-	HidP_Input,
						-	HidP_Output,
						-	HidP_Feature
Return Value:

		NTSTATUS	- STATUS_SUCCESS if Handle successfully
					- STATUS_UNSUCCESSFUL if failed
-------------------------------------------------------*/
NTSTATUS HandleMouseData(
	_In_ HIJACK_CONTEXT* pContext, 
	_In_ HIDP_REPORT_TYPE reportType
); 


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		- Proxy Function of USB Hub of Internal Control Irp
		- It intercept all internal IOCTL ,
		I.e. IOCTL_INTERNAL_USB_SUBMIT_URB -> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER 
			1. Check is it mouse / keyboard
			2. Save a IRP Context (included old IRP completion function)
			3. Hook Completion of IRP
			4. Insert into Pending IRP List

Arguments:

		DeviceObject - USB Hub Device Object, iusb3hub , usbhub ,... etc
		Irp			 - IRP come from upper level

Return Value:

		NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
					 - STATUS_XXXXXXXXXXXX   , Depended on IRP Handler

-----------------------------------------------------------------------------------*/
NTSTATUS HidUsb_InternalDeviceControl(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
); 

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

		- Proxy Function of Hid URB IRP Completion
		- Extract Data by specific report format
		- call old IRP completion

Arguments:

		DeviceObject - Hid Device Object , Class Driver-Created Device Object , 
					   device named - HID000000X, a FDO of HidUsb(HidClass)

		Irp			 - IRP of the original request

Return Value:

		NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
					   STATUS_XXXXXXXXXXXX   , Depended on old completion
-----------------------------------------------------------------------------------*/
NTSTATUS  HidUsb_CompletionCallback(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
);

//See HidHijack.h for detail
NTSTATUS InitializeHidPenetrate(
	_In_ USB_HUB_VERSION UsbVersion
);

//See HidHijack.h for detail
NTSTATUS UnInitializeHidPenetrate();

#ifdef ALLOC_PRAGMA
	#pragma alloc_text(PAGE, InitializeHidPenetrate) 
	#pragma alloc_text(PAGE, UnInitializeHidPenetrate)  
#endif


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
//// 
//// 

//----------------------------------------------------------------------------------------//
NTSTATUS AllocatePendingIrpLinkedList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do
	{
		if (!g_PendingIrpList)
		{
			g_PendingIrpList = (PENDINGIRPLIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRPLIST), 'kcaj');
			if (!g_PendingIrpList)
			{
				status = STATUS_UNSUCCESSFUL;
				break;
			} 
			RtlZeroMemory(g_PendingIrpList, sizeof(PENDINGIRPLIST));
		} 

		if (!g_PendingIrpList->head)
		{
			g_PendingIrpList->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, NULL, 0);
			if (!g_PendingIrpList->head)
			{
				ExFreePool(g_PendingIrpList);
				g_PendingIrpList = NULL;
				status = STATUS_UNSUCCESSFUL;
				break;
			}
			g_PendingIrpList->RefCount = 1;
		}

		status = STATUS_SUCCESS; 
	}while (FALSE);
	return status; 
}
//----------------------------------------------------------------------------------------//
NTSTATUS FreePendingIrpList()
{
	if (g_PendingIrpList)
	{
		if (g_PendingIrpList->head)
		{
			CHAINLIST_SAFE_FREE(g_PendingIrpList->head);
		}
		ExFreePool(g_PendingIrpList);
		g_PendingIrpList = NULL;
	}
}

//-------------------------------------------------------------------------------------------//
LOOKUP_STATUS LookupPendingIrpCallback(
	_In_ PENDINGIRP* pending_irp_node,
	_In_ void*		 context
)
{
	UNREFERENCED_PARAMETER(context);

	NTSTATUS    status = STATUS_UNSUCCESSFUL;
	PENDINGIRP* pending_irp = pending_irp_node;
	if (pending_irp)
	{
		InterlockedExchange64(&pending_irp->IrpStack->Context, pending_irp->oldContext);
		InterlockedExchange64(&pending_irp->IrpStack->CompletionRoutine, pending_irp->oldRoutine);
		USB_DEBUG_INFO_LN_EX("RemovePendingIrpCallback Once %I64x" , pending_irp_node);
	}
	return CLIST_FINDCB_CTN;
}

//----------------------------------------------------------------------------------------- 
NTSTATUS RecoverAllPendingIrp()
{
	NTSTATUS						   status = STATUS_UNSUCCESSFUL;
	if (QueryFromChainListByCallback(g_PendingIrpList->head, LookupPendingIrpCallback, NULL))
	{
		status = STATUS_SUCCESS;
	}
	return status;
}
 
//----------------------------------------------------------------------------------------//
NTSTATUS HandleKeyboardData(
	_In_ HIJACK_CONTEXT* pContext, 
	_In_ HIDP_REPORT_TYPE reportType
)
{
	HIDCLASS_DEVICE_EXTENSION* hid_common_extension = (HIDCLASS_DEVICE_EXTENSION*)(pContext->DeviceObject->DeviceExtension);
	PDO_EXTENSION*							 pdoExt = &hid_common_extension->pdoExt;
	EXTRACTDATA								   data = { 0 };
	NTSTATUS								 status = STATUS_UNSUCCESSFUL;
	ULONG								   colIndex = pdoExt->collectionIndex - 1;
	//Top-Collection == Mouse && Usage Page == Desktop
	if (!IsKeyboardUsage(pContext, colIndex) ||
		!IsDesktopUsagePage(pContext, colIndex))
	{
		status = STATUS_SUCCESS;
		return status;
	}

	USB_DEBUG_INFO_LN_EX("IsClientPdo: %x ", hid_common_extension->isClientPdo);
	USB_DEBUG_INFO_LN_EX("collectionIndex: %x collectionNum: %x ", pdoExt->collectionIndex, pdoExt->collectionNum);

	status = ExtractKeyboardData(GetCollectionDesc(pContext, colIndex), reportType, &data);

	USB_DEBUG_INFO_LN_EX("NormalKeyOffset: %X		 Size: %x", data.KBDDATA.NormalKeyOffset, data.KBDDATA.NormalKeySize);
	USB_DEBUG_INFO_LN_EX("SpecialKeyOffset: %X	 Size: %x  ", data.KBDDATA.SpecialKeyOffset, data.KBDDATA.SpecialKeySize);
	USB_DEBUG_INFO_LN_EX("LedKeyOffset: %X		 Size: %x ", data.KBDDATA.LedKeyOffset, data.KBDDATA.LedKeySize);
	ULONG totalSize = data.KBDDATA.NormalKeySize + data.KBDDATA.SpecialKeySize + data.KBDDATA.LedKeySize;
	if (!totalSize)
	{
		status = STATUS_SUCCESS;
		return status;
	}

	char* extractor = ExAllocatePool(NonPagedPool, totalSize);
	if (extractor)
	{
		char* normalKey = extractor;
		char* LedKey = extractor + data.KBDDATA.NormalKeySize;
		char* SpecialKey = extractor + data.KBDDATA.NormalKeySize + data.KBDDATA.LedKeySize;
		char* Padding = extractor + data.KBDDATA.NormalKeySize + data.KBDDATA.LedKeySize + data.KBDDATA.SpecialKeySize;

		RtlZeroMemory(extractor, totalSize);
		RtlMoveMemory(normalKey, (PUCHAR)UrbGetTransferBuffer(pContext->urb) + data.KBDDATA.NormalKeyOffset, data.KBDDATA.NormalKeySize);
		RtlMoveMemory(SpecialKey, (PUCHAR)UrbGetTransferBuffer(pContext->urb) + data.KBDDATA.SpecialKeyOffset, data.KBDDATA.SpecialKeySize);
		RtlMoveMemory(LedKey, (PUCHAR)UrbGetTransferBuffer(pContext->urb) + data.KBDDATA.LedKeyOffset, data.KBDDATA.LedKeySize);

		USB_DEBUG_INFO_LN_EX("Enter: ");

		for (int i = 0; i < data.KBDDATA.SpecialKeySize; i++)
		{
			USB_NATIVE_DEBUG_INFO("%x ", *SpecialKey);
			SpecialKey++;
		}

		for (int i = 0; i < data.KBDDATA.NormalKeySize; i++)
		{
			USB_NATIVE_DEBUG_INFO("%x ", *normalKey);
			normalKey++;
		}

		for (int i = 0; i < data.KBDDATA.LedKeySize; i++)
		{
			USB_NATIVE_DEBUG_INFO("%x ", *LedKey);
			LedKey++;
		}

		USB_DEBUG_INFO_LN();

		ExFreePool(extractor);
		extractor = NULL;
	}

	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS HandleMouseData(
	_In_ HIJACK_CONTEXT* pContext, 
	_In_ HIDP_REPORT_TYPE reportType
)
{
	HIDCLASS_DEVICE_EXTENSION* hid_common_extension = NULL;
	PDO_EXTENSION*							 pdoExt = NULL;
	EXTRACTDATA								   data = { 0 };
	NTSTATUS								 status = STATUS_UNSUCCESSFUL;
	ULONG								   colIndex = 0;
	ULONG								  totalSize = 0;

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

	hid_common_extension = (HIDCLASS_DEVICE_EXTENSION*)(pContext->DeviceObject->DeviceExtension);	

	if (!hid_common_extension)
	{
		USB_DEBUG_INFO_LN_EX("NULL hid_common_extension");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	pdoExt				 = &hid_common_extension->pdoExt; 
 
	if (!pdoExt)
	{
		USB_DEBUG_INFO_LN_EX("NULL pdoExt");
		status = STATUS_UNSUCCESSFUL;
		return status; 
	} 
	colIndex = pdoExt->collectionIndex - 1;

	if (!IsSafeNode(pContext))
	{
		status = STATUS_SUCCESS;
		return status;
	}
	 
	//Top-Collection == Mouse && Usage Page == Desktop 
	if (!IsMouseUsage(pContext, colIndex) ||
		!IsDesktopUsagePage(pContext, colIndex))
	{
		status = STATUS_SUCCESS;
		return status;
	} 

	status = ExtractMouseData(GetCollectionDesc(pContext, colIndex), reportType, &data);

	/*
	USB_DEBUG_INFO_LN_EX("collectionIndex: %x collectionNum: %x", pdoExt->collectionIndex, pdoExt->collectionNum);
	USB_DEBUG_INFO_LN_EX("OffsetX: %X		 Size: %x" , data.MOUDATA.OffsetX, data.MOUDATA.XOffsetSize);
	USB_DEBUG_INFO_LN_EX("OffsetY: %X		 Size: %x", data.MOUDATA.OffsetY, data.MOUDATA.YOffsetSize);
	USB_DEBUG_INFO_LN_EX("OffsetZ: %X		 Size: %x", data.MOUDATA.OffsetZ, data.MOUDATA.ZOffsetSize);
	USB_DEBUG_INFO_LN_EX("OffsetButton: %X Size: %x ", data.MOUDATA.OffsetButton, data.MOUDATA.BtnOffsetSize);
	*/

	totalSize = data.MOUDATA.XOffsetSize + data.MOUDATA.YOffsetSize + data.MOUDATA.ZOffsetSize + data.MOUDATA.BtnOffsetSize;
	if (!totalSize)
	{
		status = STATUS_SUCCESS;
		return status;
	}

	char* extractor = ExAllocatePool(NonPagedPool, totalSize);
	if (extractor)
	{
		char* x = extractor;
		char* y = extractor + data.MOUDATA.XOffsetSize;
		char* wheel = extractor + data.MOUDATA.XOffsetSize + data.MOUDATA.YOffsetSize;
		char* btn = extractor + data.MOUDATA.XOffsetSize + data.MOUDATA.YOffsetSize + data.MOUDATA.ZOffsetSize;

		RtlZeroMemory(extractor, totalSize);
		RtlMoveMemory(x, (PUCHAR)UrbGetTransferBuffer(pContext->urb) + data.MOUDATA.OffsetX, data.MOUDATA.XOffsetSize);
		RtlMoveMemory(y, (PUCHAR)UrbGetTransferBuffer(pContext->urb) + data.MOUDATA.OffsetY, data.MOUDATA.YOffsetSize);
		RtlMoveMemory(wheel, (PUCHAR)UrbGetTransferBuffer(pContext->urb) + data.MOUDATA.OffsetZ, data.MOUDATA.ZOffsetSize);
		//Left  = 1
		//Right = 2
		//Wheel = 3 
		RtlMoveMemory(btn, (PUCHAR)UrbGetTransferBuffer(pContext->urb) + data.MOUDATA.OffsetButton, data.MOUDATA.BtnOffsetSize);

		//TODO: loop the size
		USB_COMMON_DEBUG_INFO("");
		USB_NATIVE_DEBUG_INFO("X : %d ", *x);
		USB_NATIVE_DEBUG_INFO("Y : %d ", *y);
		USB_NATIVE_DEBUG_INFO("Z : %d ", *wheel);
		USB_NATIVE_DEBUG_INFO("btn: %d", *btn);
		USB_DEBUG_INFO_LN();
		ExFreePool(extractor);
		extractor = NULL;
	}

	return status;
}
 
 
NTSTATUS  HidUsb_CompletionCallback(
	_In_     PDEVICE_OBJECT DeviceObject,			 
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
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

	context  = pContext->pending_irp->oldContext;
	callback = pContext->pending_irp->oldRoutine;
	if (!context || !callback)
	{
		return STATUS_UNSUCCESSFUL;
	}

	// Completion func has been called when driver unloading.
	if (g_bUnloaded)
	{
		PENDINGIRP* entry = GetRealPendingIrpByIrp(Irp);
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
	if (!DelFromChainListByPointer(g_PendingIrpList->head, pContext->pending_irp))
	{
		USB_DEBUG_INFO_LN_EX("FATAL: Delete element FAILED");
	}
 
 	DUMP_DEVICE_NAME(pContext->DeviceObject);

	if (UrbIsInputTransfer(pContext->urb))
	{
		USB_COMMON_DEBUG_INFO("Input Data: ");
		PUCHAR UrbTransferBuf = (PUCHAR)UrbGetTransferBuffer(pContext->urb);

		for (int i = 0; i < UrbGetTransferLength(pContext->urb); i++)
		{
			USB_NATIVE_DEBUG_INFO("%x ", *UrbTransferBuf++);
		}
		USB_DEBUG_INFO_LN();
		HandleMouseData(pContext, HidP_Input);
	}

	if (UrbIsOutputTransfer(pContext->urb))
	{
		USB_COMMON_DEBUG_INFO("Output Data: ");
		PUCHAR UrbTransferBuf = (PUCHAR)UrbGetTransferBuffer(pContext->urb);
		for (int i = 0; i < UrbGetTransferLength(pContext->urb); i++)
		{
			USB_NATIVE_DEBUG_INFO("%x ", *UrbTransferBuf++);
		}
		USB_DEBUG_INFO_LN();
	}

	if (pContext)
	{
		ExFreePool(pContext);
		pContext = NULL;
	}  
	return callback(DeviceObject, Irp, context);
}



//----------------------------------------------------------------------------------------//
NTSTATUS HidUsb_InternalDeviceControl(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	do
	{
		HIJACK_CONTEXT*		 hijack = NULL;
		PIO_STACK_LOCATION irpStack = NULL;
		PURB					urb = NULL;
		PENDINGIRP*		  new_entry = NULL;
		PHID_DEVICE_NODE	   node = NULL;

		if (!g_HidPipeList)
		{ 
			break;
		}

		irpStack = IoGetCurrentIrpStackLocation(Irp);
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
		if (IsHidDevicePipe(g_HidPipeList->head, UrbGetTransferPipeHandle(urb), &node))
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
				new_entry->Irp = Irp;
				new_entry->oldRoutine = irpStack->CompletionRoutine;
				new_entry->oldContext = irpStack->Context;
				new_entry->IrpStack = irpStack;
				AddToChainListTail(g_PendingIrpList->head, new_entry);
		
				//Fake Context for Completion Routine
				hijack->DeviceObject = node->device_object;		//USBHUB device
				hijack->urb = urb;
				hijack->node = node;
				hijack->pending_irp = new_entry;

				//Completion Routine hook
				irpStack->Context = hijack;
				irpStack->CompletionRoutine = HidUsb_CompletionCallback;	 
			}
		}
	} while (0);


	IRPHOOKOBJ* object = GetIrpHookObject(DeviceObject->DriverObject, IRP_MJ_INTERNAL_DEVICE_CONTROL);
	if (object)
	{
		if (object->oldFunction)
		{ 
			return object->oldFunction(DeviceObject, Irp);
		}
	} 
	return g_pDispatchInternalDeviceControl(DeviceObject, Irp);
}
 
//----------------------------------------------------------------------------------------//
NTSTATUS InitializeHidPenetrate(
	_In_ USB_HUB_VERSION UsbVersion
)
{
	PDRIVER_OBJECT			  pDriverObj = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	status = GetUsbHub(UsbVersion, &pDriverObj);	// iusbhub
	if (!NT_SUCCESS(status) || !pDriverObj)
	{
		USB_DEBUG_INFO_LN_EX("GetUsbHub Error");
		return status;
	}

	status = InitHidClientPdoList(&g_HidPipeList, &(ULONG)g_HidPipeListSize);
	if (!NT_SUCCESS(status) || (!g_HidPipeList && !g_HidPipeListSize))
	{
		USB_DEBUG_INFO_LN_EX("No keyboard Or Mouse");
		return status;
	}

	USB_DEBUG_INFO_LN_EX("Done Init --- Device_object_list: %I64X Head: %I64x Size: %x", g_HidPipeList, g_HidPipeList->head,g_HidPipeListSize);

	//Init Irp Linked-List
	status = AllocatePendingIrpLinkedList();
	if (!NT_SUCCESS(status))
	{
		USB_DEBUG_INFO_LN_EX("AllocatePendingIrpLinkedList Error");
		FreeHidClientPdoList();
		return status;
	} 

	//Do Irp Hook for URB transmit
	g_pDispatchInternalDeviceControl = (PDRIVER_DISPATCH)DoIrpHook(pDriverObj, IRP_MJ_INTERNAL_DEVICE_CONTROL, HidUsb_InternalDeviceControl, Start);
	if (!g_pDispatchInternalDeviceControl)
	{
		FreeHidClientPdoList();
		FreePendingIrpList();
		USB_DEBUG_INFO_LN_EX("DoIrpHook Error");
		status = STATUS_UNSUCCESSFUL;
		return status;
	} 
}

//----------------------------------------------------------------------------------------//
NTSTATUS UnInitializeHidPenetrate()
{
	g_bUnloaded = TRUE;

	//Repair all Irp hook
	RemoveAllIrpObject();

	//Repair all completion hook
	RecoverAllPendingIrp();

	FreePendingIrpList();

	FreeHidClientPdoList();
}