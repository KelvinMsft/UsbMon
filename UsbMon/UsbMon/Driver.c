 
#include <fltKernel.h>

#include "UsbUtil.h" 
#include "UsbHid.h"
#include "IrpHook.h"
#pragma warning (disable : 4100)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Types
////

/*
typedef struct PENDINGIRP
{
	RT_LIST_ENTRY entry;
	PIRP irp; 
}PENDINGIRP, *PPENDINGIRP;
*/

typedef struct HIJACK_CONTEXT
{
	PDEVICE_OBJECT		  DeviceObject;
	PIRP				  Irp;
	PVOID				  Context;
	IO_COMPLETION_ROUTINE* routine;
	PURB				  urb;
//	PENDINGIRP*			  pending_irp;
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Marco
////  
#define ARRAY_SIZE      100
 
 
 
//-----------------------------------------------------------------------------------------
/*
 VOID RemoveAllPendingIrpFromList()
 { 
	 RT_LIST_ENTRY* pEntry = head.entry.Flink;
	 while (pEntry != &head.entry && !pEntry)
	 {
		 PENDINGIRP* pStrct;
		 pStrct = CONTAINING_RECORD(pEntry, PENDINGIRP, entry);
		 if (pStrct)
		 {
			 if (pStrct->irp)
			 { 
				 IoCancelIrp(pStrct->irp);
			 }
			 if (&(pStrct->entry))
			 {
				 RTRemoveEntryList(&(pStrct->entry));
			 } 
			 ExFreePool(pStrct);
		 }
		 pEntry = pEntry->Flink;
	 } 
 } 
*/
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
//-----------------------------------------------------------------------------------------
VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	 
	//RemoveAllPendingIrpFromList();

	PDRIVER_OBJECT pDriverObj = NULL; 

	if (NT_SUCCESS(GetUsbHub(USB3, &pDriverObj)))
	{
		if (pDriverObj)
		{
			//InterlockedExchange64((LONG64 volatile *)&pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL], (LONG64)g_pDispatchInternalDeviceControl);
			RemoveAllIrpObject();
			g_pDispatchInternalDeviceControl = NULL;
		}
	}  
/*	if (NT_SUCCESS(GetUsbHub(USB_COMPOSITE, &pDriverObj)))
	{
		if (pDriverObj && g_pDispatchInternalDeviceControlForCcgp)
		{
			InterlockedExchange64((LONG64 volatile *)&pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL], (LONG64)g_pDispatchInternalDeviceControlForCcgp);
			g_pDispatchInternalDeviceControlForCcgp = NULL;
		}
	}
*/
	while (g_irp_count)
	{
		KeSleep(0);
	}
	
	STACK_TRACE_DEBUG_INFO("IRP finished All \r\n"); 

	while (g_current_index > 0)
	{
		if (g_pHidWhiteList[g_current_index])
		{
			ExFreePool(g_pHidWhiteList[g_current_index]);
		}
		InterlockedDecrement(&g_current_index);
	}

	return;
}
//----------------------------------------------------------------------------------------//
NTSTATUS  MyCompletionCallback(
	_In_     PDEVICE_OBJECT DeviceObject,			//Class Driver-Created Device Object by MiniDriver DriverObject
	_In_     PIRP           Irp,
	_In_opt_ PVOID          Context
)
{ 
	HIJACK_CONTEXT* pContext		= (HIJACK_CONTEXT*)Context;
	PVOID context					= pContext->Context;
	IO_COMPLETION_ROUTINE* callback = pContext->routine;
	WCHAR DeviceName[256]			= { 0 };

 	//DumpUrb(pContext->urb);
	/*
	if (&pContext->pending_irp->entry)
	{
		ExFreePool(&pContext->pending_irp->entry);
	}
	*/
	// Extract HIJACK_CONTEXT  

	GetDeviceName(DeviceObject, DeviceName);

	STACK_TRACE_DEBUG_INFO("Come here is mouse deviceName: %ws DriverName: %ws \r\n", DeviceObject->DriverObject->DriverName.Buffer, DeviceName);

	ExFreePool(Context);

	InterlockedDecrement(&g_irp_count);

	return callback(DeviceObject, Irp, context);
}
 
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

		hijack = (HIJACK_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIJACK_CONTEXT), 'kcaj');
		
		if (!hijack)
		{
			break;
		}
		
		RtlZeroMemory(hijack, sizeof(HIJACK_CONTEXT)); 

		if (CheckIfPipeHandleExist(urb->UrbBulkOrInterruptTransfer.PipeHandle))
		{ 
			hijack->routine = irpStack->CompletionRoutine;
			hijack->Context = irpStack->Context;
			hijack->DeviceObject = DeviceObject;
			hijack->Irp = Irp;
			hijack->urb = urb;  
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
	status = GetUsbHub(USB3, &pDriverObj);	// iusbhub

	if (!NT_SUCCESS(status) || !pDriverObj)
	{
		return status;
	}

	InitIrpHook();
	g_pDispatchInternalDeviceControl = (PDRIVER_DISPATCH)DoIrpHook(pDriverObj, IRP_MJ_INTERNAL_DEVICE_CONTROL, DispatchInternalDeviceControl,Start);
	
 	return status;
}
 