 
#include <fltKernel.h>

#include "UsbUtil.h" 

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
DRIVER_DISPATCH*  g_pDispatchInternalDeviceControl = NULL;
DRIVER_DISPATCH*  g_pDispatchPnP  = NULL;
PHID_DEVICE_NODE* g_pHidWhiteList = NULL;
volatile LONG	  g_irp_count	  = 0;
volatile LONG	  g_current_index = 0;
  







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
			InterlockedExchange64((LONG64 volatile *)&pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL], (LONG64)g_pDispatchInternalDeviceControl);
			STACK_TRACE_DEBUG_INFO("Unloaded Driver");
		}
	}  
	
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
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_opt_ PVOID          Context
)
{ 
	HIJACK_CONTEXT* pContext = (HIJACK_CONTEXT*)Context;
	  
 	//DumpUrb(pContext->urb);
	/*
	if (&pContext->pending_irp->entry)
	{
		ExFreePool(&pContext->pending_irp->entry);
	}
	*/
	// Extract HIJACK_CONTEXT  

	InterlockedDecrement(&g_irp_count);

	return pContext->routine(DeviceObject, Irp, pContext->Context);
}
 
/*
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
*/
//----------------------------------------------------------------------------------------// 

BOOLEAN CheckIfDeviceObjectExist(PDEVICE_OBJECT hid_device_object)
{
	BOOLEAN exist = FALSE;
	int i = 0;
	while (g_pHidWhiteList[i])
	{
 		if (g_pHidWhiteList[i]->device_object == hid_device_object)
		{
			STACK_TRACE_DEBUG_INFO("Matched DeviceObject in Array: %I64X Input Device: %I64X\r\n", g_pHidWhiteList[i]->device_object, hid_device_object);
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
	//HUB_DEVICE_EXTENSION* ext = NULL;
	PIO_STACK_LOCATION irpStack = NULL; 
	PURB urb = NULL;
	irpStack = IoGetCurrentIrpStackLocation(Irp);
	urb = (PURB)irpStack->Parameters.Others.Argument1;
	if (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
	{ 
		 
 		
		if (urb->UrbHeader.Function == URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
		{ 
			//WdfUsb
			HIJACK_CONTEXT* hijack	   = (HIJACK_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIJACK_CONTEXT), 'kcaj'); 
			//PENDINGIRP*	new_entry  = (PENDINGIRP*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRP), 'kcaj');		
			PDEVICE_OBJECT	device_obj = NULL;

			device_obj = DeviceObject;

			while (device_obj) 
			{  
				WCHAR device_name[256] = { 0 };
				GetDeviceName(device_obj, device_name);
			
				//STACK_TRACE_DEBUG_INFO("DriverObj: %I64X DriverName: %ws \r\n DeviceObj: %I64X DeviceName: %ws \r\n",device_obj->DriverObject, device_obj->DriverObject->DriverName.Buffer, device_obj, device_name, device_obj->AttachedDevice);
		
				if (!device_obj->AttachedDevice)
				{		
					STACK_TRACE_DEBUG_INFO("LastDevice DriverObj: %ws \r\n device_obj:%I64X \r\n", device_obj->DriverObject->DriverName.Buffer, device_obj);
					break;
				}
				device_obj = device_obj->AttachedDevice;
			}

			if (CheckIfDeviceObjectExist(device_obj))
			{
				// TODO: Insert if keyboard / mouse device
				// new_entry->irp = Irp;
				// RTInsertTailList(&head.entry, &new_entry->entry);

				hijack->routine = irpStack->CompletionRoutine;
				hijack->Context = irpStack->Context;
				hijack->DeviceObject = DeviceObject;
				hijack->Irp = Irp;
				hijack->urb = urb;
				//hijack->pending_irp = new_entry;

				irpStack->CompletionRoutine = MyCompletionCallback;
				irpStack->Context = hijack;

				InterlockedIncrement(&g_irp_count);
			} 
		}
	}  
	return g_pDispatchInternalDeviceControl(DeviceObject, Irp);
}
 
//----------------------------------------------------------------------------------------//
NTSTATUS DriverEntry(PDRIVER_OBJECT object, PUNICODE_STRING String)
{

	PDRIVER_OBJECT pDriverObj;

	//RTInitializeListHead(&head.entry);
  
	WCHAR* Usbhub = GetUsbHubDriverNameByVersion(USB);  
	STACK_TRACE_DEBUG_INFO("Usb Hun: %ws \r\n", Usbhub);

	WCHAR* Usbhub2 = GetUsbHubDriverNameByVersion(USB2); 
	STACK_TRACE_DEBUG_INFO("Usb Hun2: %ws \r\n", Usbhub2);

	WCHAR* Usbhub3 = GetUsbHubDriverNameByVersion(USB3); 
	STACK_TRACE_DEBUG_INFO("Usb Hun3: %ws \r\n", Usbhub3);


	if (!g_pHidWhiteList)
	{
		g_pHidWhiteList = (PHID_DEVICE_NODE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PHID_DEVICE_NODE) * ARRAY_SIZE,'ldih');
		RtlZeroMemory(g_pHidWhiteList, sizeof(PHID_DEVICE_NODE)*ARRAY_SIZE);
	}
	 
	pDriverObj = NULL; 

	GetDriverObjectByName(L"\\Driver\\hidusb", &pDriverObj);

	if (pDriverObj)
	{
		STACK_TRACE_DEBUG_INFO("DriverObj: %I64X \r\n", pDriverObj);
		PDEVICE_OBJECT device_object = pDriverObj->DeviceObject;
		while (device_object)
		{
			HID_USB_DEVICE_EXTENSION* mini_extension   = NULL;
			HID_DEVICE_EXTENSION* hid_common_extension = NULL;
			WCHAR DeviceName[256] = { 0 };
			GetDeviceName(device_object, DeviceName);
			STACK_TRACE_DEBUG_INFO("---------------------------------------\r\n");
			STACK_TRACE_DEBUG_INFO("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws \r\n", device_object, device_object->DriverObject->DriverName.Buffer, DeviceName);

			hid_common_extension = (HID_DEVICE_EXTENSION*)device_object->DeviceExtension;
			if (!hid_common_extension)
			{
				return STATUS_UNSUCCESSFUL;
			}

			STACK_TRACE_DEBUG_INFO("Extension_common: %I64X sizeof: %x \r\n", hid_common_extension, sizeof(HID_USB_DEVICE_EXTENSION));

			mini_extension = (HID_USB_DEVICE_EXTENSION*)hid_common_extension->MiniDeviceExtension;
			if (!mini_extension)
			{
				return STATUS_UNSUCCESSFUL;
			}

			DumpHidMiniDriverExtension(hid_common_extension); 

			if (mini_extension->InterfaceDesc->Class == 3 && 
				mini_extension->InterfaceDesc->Protocol == 2)
			{
  
				HID_DEVICE_NODE* node = ExAllocatePool(NonPagedPool, sizeof(HID_DEVICE_NODE));
				RtlZeroMemory(node,sizeof(HID_DEVICE_NODE));
				node->device_object = device_object;
				node->InterfaceDesc = mini_extension->InterfaceDesc;
				g_pHidWhiteList[g_current_index] = node;
				g_current_index++;
				STACK_TRACE_DEBUG_INFO("Inserted one element: %I64x InferfaceDesc: %I64X device_object: %I64x \r\n", node->device_object, node->InterfaceDesc, device_object);
				STACK_TRACE_DEBUG_INFO("Added one element :%x \r\n", g_current_index);  
			}

			STACK_TRACE_DEBUG_INFO("---------------------------------------\r\n");
			device_object = device_object->NextDevice; 
		}
	}

	pDriverObj = NULL;
	GetUsbHub(USB3, &pDriverObj); 

	g_pDispatchInternalDeviceControl = (DRIVER_DISPATCH*)InterlockedExchange64(
			(LONG64 volatile *)&pDriverObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL],
			(LONG64)DispatchInternalDeviceControl
		);

	
	object->DriverUnload = DriverUnload;

	return 0;
}
 