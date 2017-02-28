 
#include <fltKernel.h>
#include "Util.h" 
#include "usb.h"
#include "Usbioctl.h"
#include "LinkedList.h"
#include "Hidport.h"
#include "hubbusif.h"
#include "usbbusif.h"

#pragma warning (disable : 4100)

typedef struct PENDINGIRP
{
	RT_LIST_ENTRY entry;
	PIRP irp; 
}PENDINGIRP, *PPENDINGIRP; 

typedef struct _HID_DEVICE_NODE
{
	PDEVICE_OBJECT					device_object;
	PUSBD_INTERFACE_INFORMATION		InterfaceDesc;
}HID_DEVICE_NODE, *PHID_DEVICE_NODE;

typedef struct _HID_USB_DEVICE_EXTENSION {
	ULONG64							   status;			//+0x0	
	PUSB_DEVICE_DESCRIPTOR		    DeviceDesc;			//+0x8
	PUSBD_INTERFACE_INFORMATION		InterfaceDesc;		//+0x10
	USBD_CONFIGURATION_HANDLE 		ConfigurationHandle;//+0x18
	char							reserved[0x37];
	HID_DESCRIPTOR*					descriptorList;		//+0x57
}HID_USB_DEVICE_EXTENSION, *PHID_USB_DEVICE_EXTENSION;
static_assert(sizeof(HID_USB_DEVICE_EXTENSION) == 0x60, "Size Check");
   
typedef struct HIJACK_CONTEXT
{
	PDEVICE_OBJECT		   DeviceObject;
	PIRP				   Irp;
	PVOID				   Context;
	IO_COMPLETION_ROUTINE* routine;
	PURB				   urb;
	PENDINGIRP*			   pending_irp;
}HIJACK_CONTEXT, *PHIJACK_CONTEXT;


typedef enum
{
	USB = 0,
	USB2 ,
	USB3 ,
}USB_HUB_VERSION;

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


typedef struct _PORT_STATUS_CHANGE
{
	USHORT Status;
	USHORT Change;
} PORT_STATUS_CHANGE, *PPORT_STATUS_CHANGE;



DRIVER_DISPATCH*  g_pDispatchInternalDeviceControl = NULL;
DRIVER_DISPATCH*  g_pDispatchPnP = NULL;
PHID_DEVICE_NODE* g_pHidWhiteList = NULL;
volatile LONG	  g_irp_count = 0;
volatile LONG	  g_current_index = 0;
PENDINGIRP		  g_head = { 0 };


#define USB_MAXCHILDREN 127
#define ARRAY_SIZE      100


VOID	   DumpUrb(PURB urb); 
NTSTATUS   GetUsbHub(USB_HUB_VERSION usb_hub_version, PDRIVER_OBJECT* pDriverObj);
 
BOOLEAN SearchByDevice(PDEVICE_OBJECT device_object)
{
	 
	return FALSE;
}

//-----------------------------------------------------------------------------------------
 VOID RemoveAllPendingIrpFromList()
 { 
	 RT_LIST_ENTRY* pEntry = g_head.entry.Flink;
	 while (pEntry != &g_head.entry && !pEntry)
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

	/*

	*/ 
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
	  
 	//DumpUrb(pContext->urb);
 
	if (&pContext->pending_irp->entry)
	{
		ExFreePool(&pContext->pending_irp->entry);
	}
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
	return pDispatchPnP(DeviceObject, Irp);
}
*/
//----------------------------------------------------------------------------------------// 

BOOLEAN CheckDeviceObject(PDEVICE_OBJECT hid_device_object)
{
	 
	HID_DEVICE_NODE* ptr = g_pHidWhiteList[0]; 
	STACK_TRACE_DEBUG_INFO("LastDevice DriverObj: %ws device_obj:%I64X \r\n", ptr->device_object, ptr->InterfaceDesc);

	/*
	if (!hid_device_object)
	{
		return FALSE;
	}

	HID_DEVICE_EXTENSION* hid_common_extension = (HID_DEVICE_EXTENSION*)hid_device_object->DeviceExtension;

	if (!hid_common_extension)
	{
		return FALSE;
	}
	HID_USB_DEVICE_EXTENSION* mini_extension = (HID_USB_DEVICE_EXTENSION*)hid_common_extension->MiniDeviceExtension;

	if (!mini_extension)
	{
		return FALSE;
	}
	USB_INTERFACE_DESCRIPTOR* interfaceDesc = (USB_INTERFACE_DESCRIPTOR*)mini_extension->InterfaceDesc;

	if (!interfaceDesc)
	{
		return FALSE;
	}
	
	STACK_TRACE_DEBUG_INFO("bInterfaceClass: %x bInterfaceProtocol:  %x \r\n", interfaceDesc->bInterfaceClass, interfaceDesc->bInterfaceProtocol);

	if (interfaceDesc->bInterfaceClass == 3 && 
		interfaceDesc->bInterfaceProtocol == 2)
	{
		return TRUE;
	}
	*/
	 
	return TRUE;
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
//			HIJACK_CONTEXT* hijack	   = (HIJACK_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIJACK_CONTEXT), 'kcaj'); 
//			PENDINGIRP*		new_entry  = (PENDINGIRP*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRP), 'kcaj');		
			PDEVICE_OBJECT	device_obj = NULL;

			device_obj = DeviceObject;

			while (device_obj) 
			{  
				WCHAR device_name[256] = { 0 };
				GetDeviceName(device_obj, device_name);

				//STACK_TRACE_DEBUG_INFO("DriverObj: %I64X DriverName: %ws \r\n DeviceObj: %I64X DeviceName: %ws \r\n",
					//device_obj->DriverObject, device_obj->DriverObject->DriverName.Buffer, device_obj, device_name, device_obj->AttachedDevice);
				
				if (!device_obj->AttachedDevice)
				{		
					//STACK_TRACE_DEBUG_INFO("LastDevice DriverObj: %ws device_obj:%I64X \r\n", device_obj->DriverObject->DriverName.Buffer, device_obj);
					break;
				}
				device_obj = device_obj->AttachedDevice;
			}
			
			STACK_TRACE_DEBUG_INFO("LastDevice DriverObj: %ws device_obj:%I64X \r\n", device_obj->DriverObject->DriverName.Buffer, device_obj);

			/*
			if (CheckDeviceObject(device_obj))
			{
				// TODO: Insert if keyboard / mouse device
				// new_entry->irp = Irp;
				// RTInsertTailList(&head.entry, &new_entry->entry);

				hijack->routine = irpStack->CompletionRoutine;
				hijack->Context = irpStack->Context;
				hijack->DeviceObject = DeviceObject;
				hijack->Irp = Irp;
				hijack->urb = urb;
				hijack->pending_irp = new_entry;

				irpStack->CompletionRoutine = MyCompletionCallback;
				irpStack->Context = hijack;

			} 
			*/
			InterlockedIncrement(&g_irp_count); 
		}
	}  
	return g_pDispatchInternalDeviceControl(DeviceObject, Irp);
}

//----------------------------------------------------------------------------------------//
WCHAR* GetUsbHubDriverNameByVersion(USB_HUB_VERSION usb_hub_version) 
{
	WCHAR* ret = L"";
	switch (usb_hub_version)
	{
		case USB:
			ret = L"\\Driver\\usbhub";
			break;
		case USB2:
			ret = L"\\Driver\\usbhub"; 
			break;
		case USB3:
			ret = L"\\Driver\\iusb3hub"; 
			break;
		default:
			break;
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

	RTInitializeListHead(&g_head.entry);
  
	WCHAR* Usbhub = GetUsbHubDriverNameByVersion(USB);  
	STACK_TRACE_DEBUG_INFO("Usb Hun: %ws \r\n", Usbhub);

	WCHAR* Usbhub2 = GetUsbHubDriverNameByVersion(USB2); 
	STACK_TRACE_DEBUG_INFO("Usb Hun2: %ws \r\n", Usbhub2);

	WCHAR* Usbhub3 = GetUsbHubDriverNameByVersion(USB3); 
	STACK_TRACE_DEBUG_INFO("Usb Hun3: %ws \r\n", Usbhub3);


	if (!g_pHidWhiteList)
	{
		g_pHidWhiteList = (PHID_DEVICE_NODE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PHID_DEVICE_NODE) * ARRAY_SIZE,'ldih');
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

			STACK_TRACE_DEBUG_INFO(" --->PhysicalDeviceObject: %I64X \r\n", hid_common_extension->PhysicalDeviceObject);
			STACK_TRACE_DEBUG_INFO(" --->DriverObject: %I64X \r\n", hid_common_extension->PhysicalDeviceObject->DriverObject);
			STACK_TRACE_DEBUG_INFO(" --->NextDeviceObject: %I64X \r\n", hid_common_extension->NextDeviceObject);

			STACK_TRACE_DEBUG_INFO("  --->Status: %I64X \r\n", mini_extension->status);
			STACK_TRACE_DEBUG_INFO("  --->DeviceDesc: %I64X \r\n", mini_extension->DeviceDesc);
			STACK_TRACE_DEBUG_INFO("		--->bDeviceClass: %I64X \r\n", mini_extension->DeviceDesc->bDeviceClass);
			STACK_TRACE_DEBUG_INFO("		--->bDeviceSubClass: %I64X \r\n", mini_extension->DeviceDesc->bDeviceSubClass);
			STACK_TRACE_DEBUG_INFO("		--->bDeviceProtocol: %I64X \r\n", mini_extension->DeviceDesc->bDeviceProtocol);
			STACK_TRACE_DEBUG_INFO("		--->bDescriptorType: %I64X \r\n", mini_extension->DeviceDesc->bDescriptorType);
			STACK_TRACE_DEBUG_INFO("		--->bLength: %I64X \r\n", mini_extension->DeviceDesc->bLength);
			STACK_TRACE_DEBUG_INFO("		--->bMaxPacketSize0: %I64X \r\n", mini_extension->DeviceDesc->bMaxPacketSize0);
			STACK_TRACE_DEBUG_INFO("		--->bcdUSB: %I64X \r\n", mini_extension->DeviceDesc->bcdUSB);
			STACK_TRACE_DEBUG_INFO("		--->idProduct: %I64X \r\n", mini_extension->DeviceDesc->idProduct);
			STACK_TRACE_DEBUG_INFO("		--->idVendor: %I64X \r\n", mini_extension->DeviceDesc->idVendor);
			STACK_TRACE_DEBUG_INFO("  --->InterfaceDescDesc: %I64X \r\n", mini_extension->InterfaceDesc);
			STACK_TRACE_DEBUG_INFO("		--->Class: %I64X \r\n", mini_extension->InterfaceDesc->Class);
			STACK_TRACE_DEBUG_INFO("		--->SubClass: %I64X \r\n", mini_extension->InterfaceDesc->SubClass);
			STACK_TRACE_DEBUG_INFO("		--->Protocol: %I64X \r\n", mini_extension->InterfaceDesc->Protocol);
			STACK_TRACE_DEBUG_INFO("		--->NumOfPipes: %I64X \r\n", mini_extension->InterfaceDesc->NumberOfPipes);
			STACK_TRACE_DEBUG_INFO("		--->Length: %I64X \r\n", mini_extension->InterfaceDesc->Length);
			STACK_TRACE_DEBUG_INFO("		--->AlternateSetting: %I64X \r\n", mini_extension->InterfaceDesc->AlternateSetting);

			/*
			HID_DESCRIPTOR* hid_desc = (HID_DESCRIPTOR*)(mini_extension + 0x57);

			STACK_TRACE_DEBUG_INFO("  --->DescList : %I64X \r\n", FIELD_OFFSET(HID_USB_DEVICE_EXTENSION, descriptorList));
			STACK_TRACE_DEBUG_INFO("  --->DescList : %I64X %I64X \r\n", hid_desc, mini_extension->descriptorList);
			STACK_TRACE_DEBUG_INFO("	--->bcdHID: %I64X \r\n", hid_desc->bcdHID);
			STACK_TRACE_DEBUG_INFO("	--->bCountry: %I64X \r\n", hid_desc->bCountry);
			STACK_TRACE_DEBUG_INFO("	--->bDescriptorType: %I64X \r\n", hid_desc->bDescriptorType);
			STACK_TRACE_DEBUG_INFO("	--->bLength: %I64X \r\n", hid_desc->bLength);
			STACK_TRACE_DEBUG_INFO("	--->bNumDescriptors: %I64X \r\n", hid_desc->bNumDescriptors); 
			*/
			if (mini_extension->InterfaceDesc->Class == 3 && 
				mini_extension->InterfaceDesc->Protocol == 1)
			{
  
				HID_DEVICE_NODE* node = ExAllocatePool(NonPagedPool, sizeof(HID_DEVICE_NODE));
				RtlZeroMemory(node,sizeof(HID_DEVICE_NODE));
				node->device_object = device_object;
				node->InterfaceDesc = mini_extension->InterfaceDesc;
				g_pHidWhiteList[g_current_index] = node;
				g_current_index++;
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
 