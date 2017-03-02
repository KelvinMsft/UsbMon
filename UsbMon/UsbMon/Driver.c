
#include <fltKernel.h>

#include "UsbUtil.h" 
#include "UsbHid.h"
#include "IrpHook.h"
#include "Urb.h"
#pragma warning (disable : 4100)



/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Types
//// 



typedef struct PENDINGIRP
{
	PIO_STACK_LOCATION		 IrpStack; 
	PVOID					oldContext;
	IO_COMPLETION_ROUTINE*	oldRoutine;  
}PENDINGIRP, *PPENDINGIRP;

typedef struct HIJACK_CONTEXT
{
	PDEVICE_OBJECT		   DeviceObject;
	PURB							urb;
	HID_DEVICE_NODE*			   node;
	PENDINGIRP*			    pending_irp;
}HIJACK_CONTEXT, *PHIJACK_CONTEXT;


typedef struct _PENDINGIRP_LIST
{
	TChainListHeader*	head; 
}PENDINGIRPLIST, *PPENDINGIRPLIST;
 

///////////////////////////////////////////////////////////////////////////////////////////////
////	Marco
////  
////




///////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////
////
DRIVER_DISPATCH*  g_pDispatchInternalDeviceControl = NULL;
volatile LONG	  g_current_index      = 0;
BOOLEAN			  g_bUnloaded		   = FALSE;
PENDINGIRPLIST*	  g_pending_irp_header = NULL;
PHID_DEVICE_LIST  g_HidPipeList		   = NULL;


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 
////




/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
//// 
////

//-------------------------------------------------------------------------------------------//
ULONG RemovePendingIrpCallback(
	_In_ PENDINGIRP* pending_irp_node,
	_In_ void*		 context
)
{
	NTSTATUS    status = STATUS_UNSUCCESSFUL;
	PENDINGIRP* pending_irp = pending_irp_node;
	if (pending_irp)
	{ 
		InterlockedExchange64(&pending_irp->IrpStack->Context, pending_irp->oldContext);
		InterlockedExchange64(&pending_irp->IrpStack->CompletionRoutine, pending_irp->oldRoutine);
		STACK_TRACE_DEBUG_INFO("RemovePendingIrpCallback Once %I64x \r\n", pending_irp_node);
	} 
	return CLIST_FINDCB_CTN;
}
//----------------------------------------------------------------------------------------- 
NTSTATUS RemoveAllPendingIrpFromList()
{
	NTSTATUS						   status = STATUS_UNSUCCESSFUL;  
	if (QueryFromChainListByCallback(g_pending_irp_header->head, RemovePendingIrpCallback, NULL))
	{
		status = STATUS_SUCCESS;
	} 
	return status;
} 
//-----------------------------------------------------------------------------------------
VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
)
{

	UNREFERENCED_PARAMETER(DriverObject);

	g_bUnloaded = TRUE;

	//Repair all Irp hook
	RemoveAllIrpObject();

	//Repair all completion hook
	RemoveAllPendingIrpFromList();
	 
	if (g_pending_irp_header) 
	{
		if (g_pending_irp_header->head)
		{
			CHAINLIST_SAFE_FREE(g_pending_irp_header->head);
		}
		ExFreePool(g_pending_irp_header);
		g_pending_irp_header = NULL;
	}
	
	//Free White List
	if (g_HidPipeList)
	{
		if (g_HidPipeList->head)
		{
			CHAINLIST_SAFE_FREE(g_HidPipeList->head);
		}
		ExFreePool(g_HidPipeList);
		g_HidPipeList = NULL;
	}

	return;
} 

//----------------------------------------------------------------------------------------// 
PENDINGIRP* GetRealPendingIrpByIrp(
	_In_ PIRP irp
)
{ 
	return QueryFromChainListByULONGPTR(g_pending_irp_header->head, (ULONG_PTR)irp);
}

//----------------------------------------------------------------------------------------//
ULONG FindInterefaceCallback(
	_In_ HID_DEVICE_NODE* node,
	_In_ void*		 context
)
{
	if (node)
	{
		if (node->device_object == context) 
		{
			return CLIST_FINDCB_RET;
		}
	}
	return CLIST_FINDCB_CTN;
}
//----------------------------------------------------------------------------------------//
NTSTATUS  MyCompletionCallback(
	_In_     PDEVICE_OBJECT DeviceObject,			//Class Driver-Created Device Object by MiniDriver DriverObject
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
)
{
	KIRQL					   irql = 0;
	HIJACK_CONTEXT*		   pContext = (HIJACK_CONTEXT*)Context;
	PVOID					context = NULL;
	IO_COMPLETION_ROUTINE* callback = NULL;
	WCHAR			DeviceName[256] = { 0 };

	if (!pContext)
	{
		STACK_TRACE_DEBUG_INFO("EmptyContext");
		return STATUS_UNSUCCESSFUL;
	}

	if (!pContext->pending_irp)
	{
		STACK_TRACE_DEBUG_INFO("Empty pending_irp"); 
		return STATUS_UNSUCCESSFUL;
	}

	if (!pContext)
	{
		return STATUS_UNSUCCESSFUL;
	}

	context  = pContext->pending_irp->oldContext;
	callback = pContext->pending_irp->oldRoutine;

	// Completion func has been called when driver unloading.
	if (g_bUnloaded)
	{	
		PENDINGIRP* entry = GetRealPendingIrpByIrp(Irp);
		context			  = entry->oldContext;
		callback		  = entry->oldRoutine; 		
		STACK_TRACE_DEBUG_INFO("Safety Call old Routine: %I64x Context: %I64x\r\n", callback, context); 
	}

	//Rarely call here when driver unloading, Safely call because driver_unload 
	//will handle all element in the list. we dun need to free and unlink it,
	//otherwise, system crash.
	if (g_bUnloaded && callback && context)
	{ 
		return callback(DeviceObject, Irp, context);
	}

	//If Driver is not unloading , delete it 
	if (!DelFromChainListByPointer(g_pending_irp_header->head, pContext->pending_irp))
	{
		STACK_TRACE_DEBUG_INFO("FATAL: Delete element FAILED \r\n");
	}

	STACK_TRACE_DEBUG_INFO("Class: %x Protocol: %x \r\n" , pContext->node->mini_extension->InterfaceDesc->Class, pContext->node->mini_extension->InterfaceDesc->Protocol);

	/*for (i = 0; i < hidDescriptor->bNumDescriptors; i++) {
		if (hidDescriptor->DescriptorList[i].bReportType == HID_REPORT_DESCRIPTOR_TYPE) {
			rawReportDescriptorLength = (ULONG)hidDescriptor->DescriptorList[i].wReportLength;
			break;
		}
	}*/
	//DumpUrb(pContext->urb); 

	GetDeviceName(DeviceObject, DeviceName);
	STACK_TRACE_DEBUG_INFO("Mouse/Keyboard DeviceName: %ws DriverName: %ws \r\n", DeviceObject->DriverObject->DriverName.Buffer, DeviceName);

	//Free it 
	if (pContext)
	{
		ExFreePool(pContext);
		pContext = NULL;
	}

	return callback(DeviceObject, Irp, context);
}

//----------------------------------------------------------------------------------------//
NTSTATUS CheckPipeHandle(
	_In_ USBD_PIPE_HANDLE pipe_handle_from_urb, 
	_In_ USBD_PIPE_INFORMATION* pipe_handle_from_whitelist, 
	_In_ ULONG NumberOfPipes
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	ULONG i;
	for (i = 0; i < NumberOfPipes; i++)
	{
		if (pipe_handle_from_urb == pipe_handle_from_whitelist[i].PipeHandle)
		{		
		 	status = STATUS_SUCCESS;
			break;
		}
	}

	return status;
}


//----------------------------------------------------------------------------------------//
ULONG QueryPipeFromPipeList(PHID_DEVICE_NODE Data, void* Context)
{
	NTSTATUS status = CheckPipeHandle(Context,
		Data->mini_extension->InterfaceDesc->Pipes,
		Data->mini_extension->InterfaceDesc->NumberOfPipes
	);
	if (!NT_SUCCESS(status))
	{
		return 	CLIST_FINDCB_CTN;
	}
	return CLIST_FINDCB_RET;
}


//----------------------------------------------------------------------------------------// 
BOOLEAN CheckIfPipeHandleExist(
	_In_ USBD_PIPE_HANDLE handle,
	_Out_ PHID_DEVICE_NODE* node
)
{	
	BOOLEAN exist = FALSE; 

	*node = QueryFromChainListByCallback(g_HidPipeList->head, QueryPipeFromPipeList, handle);
	
	if (*node)
	{
		exist = TRUE;
	}

	return exist;
}


//----------------------------------------------------------------------------------------// 
NTSTATUS DispatchInternalDeviceControl(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	do
	{
		HIJACK_CONTEXT* hijack = NULL;
		PIO_STACK_LOCATION irpStack = NULL;
		PURB urb = NULL;
		PENDINGIRP*	new_entry = NULL;

		irpStack = IoGetCurrentIrpStackLocation(Irp);
		if (irpStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
		{
			break;
		}

		urb = (PURB)irpStack->Parameters.Others.Argument1;
		if (urb->UrbHeader.Function != URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
		{
			break;
		}
		PHID_DEVICE_NODE node = NULL;

		//If Urb pipe handle is used by HID mouse / kbd device. 
		if (CheckIfPipeHandleExist(urb->UrbBulkOrInterruptTransfer.PipeHandle,&node))
		{
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
			new_entry->oldRoutine = irpStack->CompletionRoutine;
			new_entry->oldContext = irpStack->Context; 
			new_entry->IrpStack   = irpStack;
			AddToChainListTail(g_pending_irp_header->head, new_entry);

			//Fake Context for Completion Routine
			hijack->DeviceObject = DeviceObject;
			//hijack->Irp			 = Irp;
			hijack->urb			 = urb;
			hijack->node		 = node;
			hijack->pending_irp  = new_entry;

			//Completion Routine hook
			irpStack->CompletionRoutine = MyCompletionCallback;
			irpStack->Context = hijack;
			
		}

	} while (0);

	return g_pDispatchInternalDeviceControl(DeviceObject, Irp);
}
//----------------------------------------------------------------------------------------//
NTSTATUS InitPendingIrpLinkedList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!g_pending_irp_header)
	{
		g_pending_irp_header = (PENDINGIRPLIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRPLIST), 'kcaj');
	}

	if (g_pending_irp_header)
	{
		//STACK_TRACE_DEBUG_INFO("Allocation Error \r\n");
		RtlZeroMemory(g_pending_irp_header, sizeof(PENDINGIRPLIST));
		g_pending_irp_header->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, NULL, 0);
	}

	if (g_pending_irp_header->head)
	{
		status = STATUS_SUCCESS;
	}

	return status;

}

//----------------------------------------------------------------------------------------//
NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT object, 
	_In_ PUNICODE_STRING String)
{
	PDRIVER_OBJECT			  pDriverObj = NULL;
	NTSTATUS					  status = STATUS_UNSUCCESSFUL;
	  
	object->DriverUnload = DriverUnload;

	status = InitHidRelation(&g_HidPipeList, &(ULONG)g_current_index);

	if (!NT_SUCCESS(status) || (!g_HidPipeList && !g_current_index))
	{
		STACK_TRACE_DEBUG_INFO("No keyboard Or Mouse \r\n");  
		return status;
	}

	STACK_TRACE_DEBUG_INFO("Done Init --- Device_object_list: %I64X Size: %x \r\n", g_HidPipeList, g_current_index);

	status = GetUsbHub(USB2, &pDriverObj);	// iusbhub
	if (!NT_SUCCESS(status) || !pDriverObj)
	{
		STACK_TRACE_DEBUG_INFO("GetUsbHub Error \r\n");
		return status;
	}
	
	//Init Irp Linked-List
	status = InitPendingIrpLinkedList();
	if (!NT_SUCCESS(status))
	{
		STACK_TRACE_DEBUG_INFO("InitPendingIrpLinkedList Error \r\n"); 
		return status;
	}
	//Init Irp Hook for URB transmit
	status = InitIrpHook();
	if (!NT_SUCCESS(status))
	{
		STACK_TRACE_DEBUG_INFO("InitIrpHook Error \r\n");
		return status;
	}

	//Do Irp Hook for URB transmit
	g_pDispatchInternalDeviceControl = (PDRIVER_DISPATCH)DoIrpHook(pDriverObj,IRP_MJ_INTERNAL_DEVICE_CONTROL,DispatchInternalDeviceControl, Start);
	if (!g_pDispatchInternalDeviceControl)
	{
		STACK_TRACE_DEBUG_INFO("DoIrpHook Error \r\n"); 
		return status;
	}
	return status;
}
