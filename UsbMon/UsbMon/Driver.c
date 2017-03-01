 
#include <fltKernel.h>

#include "UsbUtil.h" 
#include "UsbHid.h"
#include "IrpHook.h"
#pragma warning (disable : 4100)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Types
////


typedef struct PENDINGIRP
{
	RT_LIST_ENTRY				entry;
	PIRP				 		  Irp;
	PIO_STACK_LOCATION		 IrpStack; 
	KSPIN_LOCK				spin_lock;
	IO_COMPLETION_ROUTINE* oldRoutine;
	PVOID				   oldContext;
	BOOLEAN					 handling;
}PENDINGIRP, *PPENDINGIRP;


typedef struct HIJACK_CONTEXT
{
	PDEVICE_OBJECT		   DeviceObject;
	PIRP							Irp;
	PVOID						Context;
	IO_COMPLETION_ROUTINE*		routine;
	PURB							urb;
	PENDINGIRP*			    pending_irp; 
}HIJACK_CONTEXT, *PHIJACK_CONTEXT;    
  
typedef enum _DEVICE_PNP_STATE {
	NotStarted = 0,         // Not started
	Started,                // After handling of START_DEVICE IRP
	StopPending,            // After handling of QUERY_STOP IRP
	Stopped,                // After handling of STOP_DEVICE IRP
	RemovePending,          // After handling of QUERY_REMOVE IRP
	SurpriseRemovePending,  // After handling of SURPRISE_REMOVE IRP
	Deleted,                // After handling of REMOVE_DEVICE IRP
	UnKnown                 // Unknown state
} DEVICE_PNP_STATE;
 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////

//PENDINGIRP	  g_head;
DRIVER_DISPATCH*  g_pDispatchInternalDeviceControlForCcgp = NULL;
DRIVER_DISPATCH*  g_pDispatchInternalDeviceControl = NULL;
DRIVER_DISPATCH*  g_pDispatchPnP  = NULL;
PHID_DEVICE_NODE* g_pHidWhiteList = NULL;
volatile LONG	  g_irp_count	  = 0;
volatile LONG	  g_current_index = 0;
PDRIVER_OBJECT    g_pDriverObj	  = NULL;
BOOLEAN			  g_bUnloaded	  = FALSE;
 
PENDINGIRP*		  g_head = NULL;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Marco
////  
#define ARRAY_SIZE      100

/*---------------------------------------------------------------------
NTSTATUS DispatchPnP(
_Inout_ struct _DEVICE_OBJECT *DeviceObject,
_Inout_ struct _IRP           *Irp
)
{
NTSTATUS Status = Irp->IoStatus.Status;
PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
switch (IoStack->MinorFunction)
{
case IRP_MN_QUERY_INTERFACE:

}
return g_pDispatchPnP(DeviceObject, Irp);
}
-----------------------------------------------------------------------*/

//----------------------------------------------------------------------------------------//  
PDEVICE_OBJECT TraceHidDeviceObject(PDEVICE_OBJECT device_object)
{
	PDEVICE_OBJECT ret_device_obj = device_object;

	if (!g_pDriverObj)
	{
		GetDriverObjectByName(L"\\Driver\\hidusb", &g_pDriverObj);
	}

	if (!g_pDriverObj)
	{
		return NULL;
	}

	while (ret_device_obj)
	{
		WCHAR device_name[256] = { 0 };
		GetDeviceName(ret_device_obj, device_name);
		//	STACK_TRACE_DEBUG_INFO("DriverObj: %I64X DriverName: %ws DeviceObj: %I64X DeviceName: %ws \r\n",
		//	ret_device_obj->DriverObject, ret_device_obj->DriverObject->DriverName.Buffer, ret_device_obj, device_name);

		if (ret_device_obj->DriverObject == g_pDriverObj)
		{
			//STACK_TRACE_DEBUG_INFO("LastDevice DriverObj: %ws \r\n device_obj:%I64X \r\n", device_obj->DriverObject->DriverName.Buffer, device_obj);
			break;
		}
		ret_device_obj = ret_device_obj->AttachedDevice;
	}

	return ret_device_obj;
}
 
//----------------------------------------------------------------------------------------//.
NTSTATUS RemovePendingIrp(PENDINGIRP* pending_irp_node)
{
	NTSTATUS    status = STATUS_UNSUCCESSFUL;
	PENDINGIRP* pending_irp = pending_irp_node;
	if (pending_irp)
	{
		KIRQL irql = 0; 
		ExAcquireSpinLock(&pending_irp->spin_lock, &irql);
		RTRemoveEntryList(&pending_irp->entry);

		g_bUnloaded = TRUE;

		InterlockedExchange64(&pending_irp->IrpStack->Context, pending_irp->oldContext);
		InterlockedExchange64(&pending_irp->IrpStack->CompletionRoutine, pending_irp->oldRoutine);
			
		ExReleaseSpinLock(&pending_irp->spin_lock, irql);
		
		InterlockedDecrement(&g_irp_count); 
		
		status = STATUS_SUCCESS;
	}
	return status;
}
//----------------------------------------------------------------------------------------- 
 NTSTATUS RemoveAllPendingIrpFromList() 
 {
	 NTSTATUS						   status = STATUS_UNSUCCESSFUL;
	 PENDINGIRP							*pDev = NULL;
	 RT_LIST_ENTRY				*pEntry,  *pn = NULL;

	 RtEntryListForEachSafe(&g_head->entry, pEntry, pn)
	 {
		 pDev = (PENDINGIRP *)pEntry;
		 if (pDev)
		 {
			 status = RemovePendingIrp(pDev);
		 }  
	 }
	  
	 return status;
 }
 VOID FreeListMemory()
 {
	 NTSTATUS						   status = STATUS_UNSUCCESSFUL;
	 PENDINGIRP							*pDev = NULL;
	 RT_LIST_ENTRY				*pEntry, *pn = NULL;
	 RtEntryListForEachSafe(&g_head->entry, pEntry, pn)
	 {
		 PENDINGIRP* pDev = (PENDINGIRP *)pEntry;
		 if (pDev)
		 {  
			 ExFreePool(pDev);
			 pDev = NULL;
		 }
	 } 
 }
//-----------------------------------------------------------------------------------------
VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	 
	PDRIVER_OBJECT pDriverObj = NULL; 
	 
	if (NT_SUCCESS(GetUsbHub(USB, &pDriverObj)))
	{
		if (pDriverObj)
		{
			//InterlockedExchange64((LONG64 volatile *)&pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL], (LONG64)g_pDispatchInternalDeviceControl);
			RemoveAllIrpObject();	
			if (RemoveAllPendingIrpFromList())
			{ 
				FreeListMemory();
				g_pDispatchInternalDeviceControl = NULL;
			}
			STACK_TRACE_DEBUG_INFO("IRP finished All \r\n"); 
		}
	}  


	while (g_current_index > 0)
	{
		if (g_pHidWhiteList[g_current_index])
		{
			ExFreePool(g_pHidWhiteList[g_current_index]);
			g_pHidWhiteList[g_current_index] = NULL;
		}
		InterlockedDecrement(&g_current_index);
	}

	return;
}
//----------------------------------------------------------------------------------------// 
HIJACK_CONTEXT* GetRealContextByIrp(PIRP irp)
{
	RT_LIST_ENTRY* pEntry, *pn = NULL;

	RtEntryListForEachSafe(&g_head->entry, pEntry, pn)
	{
		PENDINGIRP* pDev = (PENDINGIRP *)pEntry;
		if (pDev->Irp == irp)
		{
			return pDev->oldContext;
		}
	}
}

//----------------------------------------------------------------------------------------//
NTSTATUS  MyCompletionCallback(
	_In_     PDEVICE_OBJECT DeviceObject,			//Class Driver-Created Device Object by MiniDriver DriverObject
	_In_     PIRP           Irp,
	_In_opt_ PVOID          Context
)
{   
 
	HIJACK_CONTEXT*		   pContext = (HIJACK_CONTEXT*)Context;
	PVOID					context = NULL; 
	IO_COMPLETION_ROUTINE* callback = NULL;   
	WCHAR			DeviceName[256]	= { 0 };

	if (!pContext)
	{
		return STATUS_UNSUCCESSFUL;
	}
	 
	if (!pContext->pending_irp)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (g_bUnloaded)
	{
		pContext = GetRealContextByIrp(Irp);
	}

	if (!pContext)
	{
		return STATUS_UNSUCCESSFUL;
	} 

	context  = pContext->Context;
	callback = pContext->routine;
	  
	if (!pContext->pending_irp->handling)
	{
		pContext->pending_irp->handling = TRUE;

		KIRQL irql = 0;
		ExAcquireSpinLock(&pContext->pending_irp->spin_lock, &irql);
		RTRemoveEntryList(&pContext->pending_irp->entry);
		InterlockedDecrement(&g_irp_count);
		ExReleaseSpinLock(&pContext->pending_irp->spin_lock, irql);

		//DumpUrb(pContext->urb); 
		GetDeviceName(DeviceObject, DeviceName);
		STACK_TRACE_DEBUG_INFO("Come here is mouse deviceName: %ws DriverName: %ws \r\n", DeviceObject->DriverObject->DriverName.Buffer, DeviceName);
		if (Context)
		{
			ExFreePool(Context);
			Context = NULL;
		}
	}
	
	return callback(DeviceObject, Irp, context);
}

//----------------------------------------------------------------------------------------//
NTSTATUS CheckPipeHandle(USBD_PIPE_HANDLE pipe_handle_from_urb, USBD_PIPE_INFORMATION* pipe_handle_from_whitelist, ULONG NumberOfPipes)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do {
		ULONG i;
		for (i = 0; i < NumberOfPipes; i++)
		{
			//STACK_TRACE_DEBUG_INFO("pipe_handle_from_urb: %I64x pipe_handle_from_whitelist[]: %I64x \r\n", pipe_handle_from_urb, pipe_handle_from_whitelist[i].PipeHandle);
			if (pipe_handle_from_urb == pipe_handle_from_whitelist[i].PipeHandle)
			{
				status = STATUS_SUCCESS;
				break;
			}
		}
		 
	} while (FALSE);

	return status; 
}

//----------------------------------------------------------------------------------------// 
BOOLEAN CheckIfPipeHandleExist(USBD_PIPE_HANDLE handle)
{
	BOOLEAN exist = FALSE;
	int i = 0;
	while (g_pHidWhiteList[i])
	{
		NTSTATUS status = CheckPipeHandle(handle,
					g_pHidWhiteList[i]->InterfaceDesc->Pipes,
					g_pHidWhiteList[i]->InterfaceDesc->NumberOfPipes
		);

		if (NT_SUCCESS(status))
		{
			STACK_TRACE_DEBUG_INFO("Handle Matched: %I64x \r\n", handle); 
			exist = TRUE;
		}

		i++;
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
		HIJACK_CONTEXT* hijack		= NULL;
		PIO_STACK_LOCATION irpStack = NULL; 
		PURB urb					= NULL;
		PENDINGIRP*	new_entry		= NULL;  

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

	
		if (CheckIfPipeHandleExist(urb->UrbBulkOrInterruptTransfer.PipeHandle))
		{ 	
			hijack	   = (HIJACK_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIJACK_CONTEXT), 'kcaj'); 
			new_entry  = (PENDINGIRP*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRP), 'kcaj');		

			if (!hijack || !new_entry)
			{
				break;
			}

			RtlZeroMemory(hijack, sizeof(HIJACK_CONTEXT)); 
			RtlZeroMemory(new_entry, sizeof(PENDINGIRP));
		
			new_entry->oldRoutine = irpStack->CompletionRoutine;
			new_entry->oldContext = irpStack->Context; 
			new_entry->IrpStack	  = irpStack;
			new_entry->Irp		  = Irp;
			new_entry->handling   = FALSE;
			KeInitializeSpinLock (&new_entry->spin_lock); 
			RTInsertTailList(&g_head->entry, &new_entry->entry);

			hijack->routine = irpStack->CompletionRoutine;
			hijack->Context = irpStack->Context;
			hijack->DeviceObject = DeviceObject;
			hijack->Irp = Irp;
			hijack->urb = urb;  
			hijack->pending_irp = new_entry;

			irpStack->CompletionRoutine = MyCompletionCallback;
			irpStack->Context = hijack; 
			InterlockedIncrement(&g_irp_count);
		}

	} while (0);
	
	return g_pDispatchInternalDeviceControl(DeviceObject, Irp);
} 
//----------------------------------------------------------------------------------------//
NTSTATUS DriverEntry(PDRIVER_OBJECT object, PUNICODE_STRING String)
{
	//IRPHOOKOBJ* HookObj					 = NULL;
	PDRIVER_OBJECT			  pDriverObj = NULL;
	NTSTATUS					  status = STATUS_UNSUCCESSFUL;   

	WCHAR* Usbhub = GetUsbHubDriverNameByVersion(USB);
	STACK_TRACE_DEBUG_INFO("Usb Hub: %ws \r\n", Usbhub);

	WCHAR* Usbhub2 = GetUsbHubDriverNameByVersion(USB2);
	STACK_TRACE_DEBUG_INFO("Usb Hub2: %ws \r\n", Usbhub2);

	WCHAR* Usbhub3 = GetUsbHubDriverNameByVersion(USB3);
	STACK_TRACE_DEBUG_INFO("Usb Hub3: %ws \r\n", Usbhub3);
	
	WCHAR* Usbhub_new = GetUsbHubDriverNameByVersion(USB3_NEW);
	STACK_TRACE_DEBUG_INFO("Usb Hub3_w8 above: %ws \r\n", Usbhub_new);
	
	WCHAR* Usb_ccgp = GetUsbHubDriverNameByVersion(USB_COMPOSITE);
	STACK_TRACE_DEBUG_INFO("Usb CCGP: %ws \r\n", Usb_ccgp);

	
	object->DriverUnload = DriverUnload;
	
	pDriverObj = NULL;
	status = GetDriverObjectByName(L"\\Driver\\hidusb", &g_pDriverObj);

	if (!NT_SUCCESS(status) || !g_pDriverObj)
	{
		return status;
	}

	status = InitHidRelation(&g_pHidWhiteList, &(ULONG)g_current_index);

	if (!NT_SUCCESS(status) || (!g_pHidWhiteList && !g_current_index))
	{
		return status;
	}
	  
	STACK_TRACE_DEBUG_INFO("device_object_list: %I64X size: %x \r\n", g_pHidWhiteList, g_current_index);
	  
	pDriverObj = NULL; 
	status = GetUsbHub(USB, &pDriverObj);	// iusbhub

	if (!NT_SUCCESS(status) || !pDriverObj)
	{
		return status;
	}
	g_head = (PENDINGIRP*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRP), 'kcaj');
	RTInitializeListHead(&g_head->entry);

	InitIrpHook();
	g_pDispatchInternalDeviceControl = (PDRIVER_DISPATCH)DoIrpHook(pDriverObj, IRP_MJ_INTERNAL_DEVICE_CONTROL, DispatchInternalDeviceControl,Start);
	
 	return status;
}
 