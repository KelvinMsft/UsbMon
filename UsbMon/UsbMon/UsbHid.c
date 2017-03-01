#include "UsbHid.h"
#include "UsbUtil.h"


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Global/Extern Variable 
//// 


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Marco
//// 
#define HID_USB_DEVICE L"\\Driver\\hidusb"
#define ARRAY_SIZE					  100


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
//// 

//-------------------------------------------------------------------------------------------//
HID_DEVICE_NODE* CreateHidDeviceNode(
	_In_ PDEVICE_OBJECT device_object, 
	_In_ PUSBD_INTERFACE_INFORMATION interfaces
)
{
	if (!device_object || !interfaces)
	{
		return NULL;
	}

	HID_DEVICE_NODE* node = ExAllocatePool(NonPagedPool, sizeof(HID_DEVICE_NODE));
	if (!node)
	{
		return NULL;
	}

	RtlZeroMemory(node, sizeof(HID_DEVICE_NODE));
	node->device_object = device_object;
	node->InterfaceDesc = interfaces;

	return node;
}
//---------------------------------------------------------------------------------------------------------//
BOOLEAN  IsKeyboardOrMouseDevice(
	_In_  PDEVICE_OBJECT			   device_object, 
	_Out_ PUSBD_INTERFACE_INFORMATION* usb_interfaces
)
{
	HID_USB_DEVICE_EXTENSION* mini_extension    = NULL;
	HID_DEVICE_EXTENSION* hid_common_extension  = NULL;
	WCHAR						DeviceName[256] = { 0 };

	GetDeviceName(device_object, DeviceName);
	STACK_TRACE_DEBUG_INFO("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws \r\n", device_object, device_object->DriverObject->DriverName.Buffer, DeviceName);

	hid_common_extension = (HID_DEVICE_EXTENSION*)device_object->DeviceExtension;
	if (!hid_common_extension)
	{
		return FALSE;
	}

	STACK_TRACE_DEBUG_INFO("Extension_common: %I64X sizeof: %x \r\n", hid_common_extension, sizeof(HID_USB_DEVICE_EXTENSION));
	mini_extension = (HID_USB_DEVICE_EXTENSION*)hid_common_extension->MiniDeviceExtension;
	if (!mini_extension)
	{
		return FALSE;
	}

	DumpHidMiniDriverExtension(hid_common_extension);

	if ( mini_extension->InterfaceDesc->Class == 3 &&			//HidClass Device
		(mini_extension->InterfaceDesc->Protocol == 1 ||		//Keyboard
		 mini_extension->InterfaceDesc->Protocol == 2))			//Mouse
	{
		*usb_interfaces = mini_extension->InterfaceDesc;
		return TRUE;
	}

	return FALSE;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitHidRelation(
	_Out_ PHID_DEVICE_NODE** device_object_list,
	_Out_ PULONG size
)
{
	PDEVICE_OBJECT	 device_object = NULL;
	PDRIVER_OBJECT	    pDriverObj = NULL;
	PHID_DEVICE_NODE* hid_relation = NULL;
	ULONG			  current_size = 0;

	if (!hid_relation)
	{
		hid_relation = (PHID_DEVICE_NODE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PHID_DEVICE_NODE) * ARRAY_SIZE, 'ldih');
	}
	if (!hid_relation)
	{
		return STATUS_UNSUCCESSFUL;
	}

	RtlZeroMemory(hid_relation, sizeof(PHID_DEVICE_NODE) * ARRAY_SIZE);

	GetDriverObjectByName(HID_USB_DEVICE, &pDriverObj);
	if (!pDriverObj)
	{
		return STATUS_UNSUCCESSFUL;
	}

	STACK_TRACE_DEBUG_INFO("DriverObj: %I64X \r\n", pDriverObj);

	device_object = pDriverObj->DeviceObject;
	while (device_object)
	{
		PUSBD_INTERFACE_INFORMATION interfaces = NULL;
		if (IsKeyboardOrMouseDevice(device_object,&interfaces))
		{
			if (interfaces && hid_relation)
			{
				HID_DEVICE_NODE* node = CreateHidDeviceNode(device_object, interfaces);
				hid_relation[current_size] = node;
				current_size++;
				STACK_TRACE_DEBUG_INFO("Inserted one element: %I64x InferfaceDesc: %I64X device_object: %I64x \r\n", node->device_object, node->InterfaceDesc, device_object);
				STACK_TRACE_DEBUG_INFO("Added one element :%x \r\n", current_size);
			}
		}
 		device_object = device_object->NextDevice;
	}

	if (current_size > 0 && hid_relation)
	{
		*device_object_list = hid_relation;
		*size = current_size;
		return STATUS_SUCCESS;
	}  
	
	return STATUS_UNSUCCESSFUL;
}
 