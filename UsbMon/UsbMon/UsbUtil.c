#include "UsbUtil.h"



/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Global/Extern Variable 
//// 

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


/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Marco
//// 


/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Prototype
//// 

/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Implementation
//// 

//----------------------------------------------------------------------------------------//
VOID PrintAllHidDriverName()
{
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

}

//----------------------------------------------------------------------------------------//
WCHAR* GetUsbHubDriverNameByVersion(
	_In_ USB_HUB_VERSION usb_hub_version
)
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
	case USB3_NEW:
		ret = L"\\Driver\\USBHUB3";
	default:
		break;
	}
	return ret;
}

//----------------------------------------------------------------------------------------//
NTSTATUS GetUsbHub(
	_In_ USB_HUB_VERSION usb_hub_version, 
	_Out_ PDRIVER_OBJECT* pDriverObj
)
{
	// use such API  
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING DriverName = { 0 };

	WCHAR* Usbhub = GetUsbHubDriverNameByVersion(usb_hub_version);

	RtlInitUnicodeString(&DriverName, Usbhub);

	status = ObReferenceObjectByName(
		&DriverName,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		(PVOID*)pDriverObj);

	return status;
}


void DumpHidExtension(
	_In_ HID_DEVICE_EXTENSION* hid_common_extension, 
	_In_ HID_USB_DEVICE_EXTENSION* mini_extension
)
{
	ULONG i = 0; 
	STACK_TRACE_DEBUG_INFO("---------------------------------------\r\n");

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
	STACK_TRACE_DEBUG_INFO("		--->AlternateSetting: %I64X \r\n\r\n", mini_extension->InterfaceDesc->AlternateSetting);

 	for (i = 0; i < mini_extension->InterfaceDesc->NumberOfPipes; i++)
	{
		USBD_PIPE_INFORMATION* Pipe_Information = &mini_extension->InterfaceDesc->Pipes[i];
		STACK_TRACE_DEBUG_INFO("  --->Pipe id: %x Information: %I64X \r\n",i, Pipe_Information);
		STACK_TRACE_DEBUG_INFO("		--->PipeHandle: %I64X \r\n", Pipe_Information->PipeHandle); 
		STACK_TRACE_DEBUG_INFO("		--->PipeAddress: %I64X \r\n", Pipe_Information->EndpointAddress); 
		STACK_TRACE_DEBUG_INFO("		--->PipeInterval: %I64X \r\n", Pipe_Information->Interval);
		STACK_TRACE_DEBUG_INFO("		--->PipeInterval: %I64X \r\n", Pipe_Information->MaximumPacketSize);
		STACK_TRACE_DEBUG_INFO("		--->PipeInterval: %I64X \r\n", Pipe_Information->MaximumTransferSize);
	}


	STACK_TRACE_DEBUG_INFO("---------------------------------------\r\n"); 
	HID_DESCRIPTOR* hid_desc = &(mini_extension)->HidDescriptor; 
	 
	STACK_TRACE_DEBUG_INFO("  --->HidDescriptor: %I64X \r\n", &hid_desc);
	STACK_TRACE_DEBUG_INFO("		--->bcdHID: %I64X \r\n", hid_desc->bcdHID);
	STACK_TRACE_DEBUG_INFO("		--->bCountry: %I64X \r\n", hid_desc->bCountry);
	STACK_TRACE_DEBUG_INFO("		--->bDescriptorType: %I64X \r\n", hid_desc->bDescriptorType);
	STACK_TRACE_DEBUG_INFO("		--->bLength: %I64X \r\n", hid_desc->bLength);
	STACK_TRACE_DEBUG_INFO("		--->bNumDescriptors: %I64X \r\n", hid_desc->bNumDescriptors);
	 
}


VOID	  DumpHidMiniDriverExtension(
	_In_ HID_DEVICE_EXTENSION* hid_common_extension
)
{
	if (!hid_common_extension)
	{
		return;
	}

	if (!hid_common_extension->MiniDeviceExtension)
	{
		return;
	}

	DumpHidExtension(hid_common_extension, hid_common_extension->MiniDeviceExtension);
}

VOID DumpUrb(
	_In_ PURB urb
)
{
	struct _URB_HEADER urb_header = urb->UrbHeader;;

	STACK_TRACE_DEBUG_INFO("---------------URB HEADER------------- \r\n");
	STACK_TRACE_DEBUG_INFO("- Length: %x \r\n", urb_header.Length);
	STACK_TRACE_DEBUG_INFO("- Function: %x \r\n", urb_header.Function);
	STACK_TRACE_DEBUG_INFO("- Status: %x \r\n", urb_header.Status);
	STACK_TRACE_DEBUG_INFO("- UsbdDeviceHandle: %x \r\n", urb_header.UsbdDeviceHandle);
	STACK_TRACE_DEBUG_INFO("- UsbdFlags: %x \r\n", urb_header.UsbdFlags);
	STACK_TRACE_DEBUG_INFO("---------------URB Function------------- \r\n");

	switch (urb_header.Function)
	{
#if (NTDDI_VERSION >= NTDDI_WIN8)
	case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER_USING_CHAINED_MDL:
		break;
	case URB_FUNCTION_OPEN_STATIC_STREAMS:
		break;
	case URB_FUNCTION_CLOSE_STATIC_STREAMS:
		break;
#endif
	case URB_FUNCTION_SELECT_CONFIGURATION:
		break;
	case URB_FUNCTION_SELECT_INTERFACE:
		break;
	case URB_FUNCTION_ABORT_PIPE:
		break;
	case URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL:
		break;
	case URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL:
		break;
	case URB_FUNCTION_GET_FRAME_LENGTH:
		break;
	case URB_FUNCTION_SET_FRAME_LENGTH:
		break;
	case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
		break;
	case URB_FUNCTION_CONTROL_TRANSFER:
		break;
	case URB_FUNCTION_CONTROL_TRANSFER_EX:
		break;
	case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
	{
		//			struct _URB_BULK_OR_INTERRUPT_TRANSFER* data  = (struct _URB_BULK_OR_INTERRUPT_TRANSFER*) &urb->UrbHeader;

		//			STACK_TRACE_COMMON_DBG_BREAK("_URB_BULK_OR_INTERRUPT_TRANSFER \r\n"); 
		/*			if (data->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
		{
		STACK_TRACE_COMMON_DBG_BREAK("USBD_TRANSFER_DIRECTION_IN \r\n");
		}
		if (data->TransferFlags & USBD_TRANSFER_DIRECTION_OUT)
		{
		STACK_TRACE_COMMON_DBG_BREAK("USBD_TRANSFER_DIRECTION_OUT \r\n");

		}
		if (data->TransferFlags & USBD_SHORT_TRANSFER_OK)
		{
		STACK_TRACE_COMMON_DBG_BREAK("USBD_SHORT_TRANSFER_OK \r\n");
		}*/

	}
	break;
	case URB_FUNCTION_RESET_PIPE:
		break;
	case URB_FUNCTION_SYNC_RESET_PIPE:
		break;
	case URB_FUNCTION_SYNC_CLEAR_STALL:
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
		break;
	case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
		break;
	case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
		break;
	case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
		break;
	case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
		break;
	case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
		break;
	case URB_FUNCTION_SET_FEATURE_TO_OTHER:
		break;
	case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
		break;
	case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
		break;
	case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
		break;
	case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
		break;
	case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
		break;
	case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
		break;
	case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
		break;
	case URB_FUNCTION_GET_STATUS_FROM_OTHER:
		break;
	case URB_FUNCTION_VENDOR_DEVICE:
		break;
	case URB_FUNCTION_VENDOR_INTERFACE:
		break;
	case URB_FUNCTION_VENDOR_ENDPOINT:
		break;
	case URB_FUNCTION_VENDOR_OTHER:
		break;
	case URB_FUNCTION_CLASS_DEVICE:
		break;
	case URB_FUNCTION_CLASS_INTERFACE:
		break;
	case URB_FUNCTION_CLASS_ENDPOINT:
		break;
	case URB_FUNCTION_CLASS_OTHER:
		break;
	case URB_FUNCTION_GET_CONFIGURATION:
		break;
	case URB_FUNCTION_GET_INTERFACE:
		break;
	case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
		break;
	case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
		break;
	case URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR:
		break;

	}
}