 
#include <fltKernel.h>
#include "Util.h" 
#include "usb.h"
#include "Usbioctl.h"
#include "LinkedList.h"
#include "Hidport.h"
#pragma warning (disable : 4100)

extern POBJECT_TYPE *IoDriverObjectType;

extern NTSTATUS ObReferenceObjectByName(
	PUNICODE_STRING ObjectName,
	ULONG Attributes,
	PACCESS_STATE AccessState,
	ACCESS_MASK DesiredAccess,
	POBJECT_TYPE ObjectType,
	KPROCESSOR_MODE AccessMode,
	PVOID ParseContext,
	PVOID *Object
); 

DRIVER_DISPATCH* pDispatchInternalDeviceControl;

volatile LONG g_irp_count = 0;

typedef struct PENDINGIRP
{
	RT_LIST_ENTRY entry;
	PIRP irp; 
}PENDINGIRP, *PPENDINGIRP;
  
typedef struct HIJACK_CONTEXT
{
	PDEVICE_OBJECT		  DeviceObject;
	PIRP				  Irp;
	PVOID				  Context;
	IO_COMPLETION_ROUTINE* routine;
	PURB				  urb;
	PENDINGIRP*			  pending_irp;
}HIJACK_CONTEXT, *PHIJACK_CONTEXT;

typedef enum
{
	USB = 0,
	USB2 ,
	USB3 ,
}USB_HUB_VERSION;

PENDINGIRP head;
VOID	   DumpUrb(PURB urb); 
NTSTATUS   GetUsbHub(USB_HUB_VERSION usb_hub_version, PDRIVER_OBJECT* pDriverObj);
 

//-----------------------------------------------------------------------------------------
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
 
 
 PVOID  ThreadObject = NULL;
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
			InterlockedExchange64((LONG64 volatile *)&pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL], (LONG64)pDispatchInternalDeviceControl);
			STACK_TRACE_DEBUG_INFO("Unloaded Driver");
		}
	} 
	
	while (g_irp_count)
	{
		KeSleep(0);
	}
	 
	STACK_TRACE_DEBUG_INFO("IRP finished All \r\n"); 
	return;
}
//----------------------------------------------------------------------------------------//
NTSTATUS  MyCompletionCallback(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_opt_ PVOID          Context
)
{ 
	HIJACK_CONTEXT* pContext = (HIJACK_CONTEXT*)Context;
	  
 	DumpUrb(pContext->urb);

	RTRemoveEntryList(&pContext->pending_irp->entry);  

	if (&pContext->pending_irp->entry)
	{
		ExFreePool(&pContext->pending_irp->entry);
	}
	// Extract HIJACK_CONTEXT

	// ......................

	// if mouse 
	// ....
	// if keyboard 
	// ....

	//Extract HIJACK_CONTEXT

	InterlockedDecrement(&g_irp_count);

	return pContext->routine(DeviceObject, Irp, pContext->Context);
}
NTSTATUS CheckDeviceObject(PDEVICE_OBJECT device_object) 
{ 

	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------// 
NTSTATUS DispatchInternalDeviceControl(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{

	PIO_STACK_LOCATION irpStack = NULL; 
	PURB urb = NULL;
	irpStack = IoGetCurrentIrpStackLocation(Irp);
	urb = (PURB)irpStack->Parameters.Others.Argument1;
	if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
	{ 
		 
 		
		if (urb->UrbHeader.Function == URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
		{
			 
			HIJACK_CONTEXT* hijack	   = (HIJACK_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIJACK_CONTEXT), 'kcaj'); 
			PENDINGIRP*		new_entry  = (PENDINGIRP*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRP), 'kcaj');		
			
			// TODO: Insert if keyboard / mouse device
			new_entry->irp = Irp;
			RTInsertTailList(&head.entry, &new_entry->entry); 

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
	}  
	return pDispatchInternalDeviceControl(DeviceObject, Irp);
}

//----------------------------------------------------------------------------------------//
WCHAR* GetUsbHubDriverNameByVersion(USB_HUB_VERSION usb_hub_version) 
{
	WCHAR* ret = L"";
	switch (usb_hub_version)
	{
		case USB:
			ret = L"\\Driver\\usbhub";
		case USB2:
			ret = L"\\Driver\\usbhub"; 
		case USB3:
			ret = L"\\Driver\\iusb3hub"; 
	} 
	return ret;
} 
//----------------------------------------------------------------------------------------//
NTSTATUS GetUsbHub(USB_HUB_VERSION usb_hub_version , PDRIVER_OBJECT* pDriverObj)
{
	// use such API  
	NTSTATUS status			  = STATUS_SUCCESS;
	UNICODE_STRING DriverName = { 0 };

	WCHAR* Usbhub			  =	 GetUsbHubDriverNameByVersion(usb_hub_version);

	RtlInitUnicodeString(&DriverName, Usbhub);

	status = ObReferenceObjectByName(
		&DriverName,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		(PVOID*) pDriverObj);

	return status;
}

//----------------------------------------------------------------------------------------//
NTSTATUS DriverEntry(PDRIVER_OBJECT object, PUNICODE_STRING String)
{

	PDRIVER_OBJECT pDriverObj;

	RTInitializeListHead(&head.entry);
  
	WCHAR* Usbhub = GetUsbHubDriverNameByVersion(USB);  
	STACK_TRACE_DEBUG_INFO("Usb Hun: %ws \r\n", Usbhub);

	WCHAR* Usbhub2 = GetUsbHubDriverNameByVersion(USB2); 
	STACK_TRACE_DEBUG_INFO("Usb Hun2: %ws \r\n", Usbhub2);

	WCHAR* Usbhub3 = GetUsbHubDriverNameByVersion(USB3); 
	STACK_TRACE_DEBUG_INFO("Usb Hun3: %ws \r\n", Usbhub3);

	pDriverObj = NULL;
	GetUsbHub(USB3, &pDriverObj);

	if (pDriverObj)
	{
		STACK_TRACE_DEBUG_INFO("DriverObj: %I64X \r\n", pDriverObj);
		pDispatchInternalDeviceControl = (DRIVER_DISPATCH*)InterlockedExchange64(
			(LONG64 volatile *)&pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL],
			(LONG64)DispatchInternalDeviceControl
		);
	}   

	object->DriverUnload = DriverUnload;

	return 0;
}
 