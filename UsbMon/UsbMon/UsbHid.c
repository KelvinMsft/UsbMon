#include "UsbHid.h"
#include "UsbUtil.h"

NTSTATUS InitHidRelation(
	PHID_DEVICE_NODE** device_object_list,
	PULONG size
)
{
	PDRIVER_OBJECT	  pDriverObj = NULL;
	PHID_DEVICE_NODE* hid_relation = NULL;
	ULONG			  current_size = 0;
	if (!hid_relation)
	{
		hid_relation = (PHID_DEVICE_NODE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PHID_DEVICE_NODE) * 100, 'ldih');
		RtlZeroMemory(hid_relation, sizeof(PHID_DEVICE_NODE) * 100);
	}

	GetDriverObjectByName(L"\\Driver\\hidusb", &pDriverObj);

	if (!pDriverObj)
	{
		return STATUS_UNSUCCESSFUL;
	}
	STACK_TRACE_DEBUG_INFO("DriverObj: %I64X \r\n", pDriverObj);

	PDEVICE_OBJECT device_object = pDriverObj->DeviceObject;
	while (device_object)
	{
		HID_USB_DEVICE_EXTENSION* mini_extension = NULL;
		HID_DEVICE_EXTENSION* hid_common_extension = NULL;
		WCHAR DeviceName[256] = { 0 };
		GetDeviceName(device_object, DeviceName);
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

		//TODO: Get all interfaces , and all keyboard device
		if (mini_extension->InterfaceDesc->Class == 3 &&
			(mini_extension->InterfaceDesc->Protocol == 1 || mini_extension->InterfaceDesc->Protocol == 2))
		{
			HID_DEVICE_NODE* node = ExAllocatePool(NonPagedPool, sizeof(HID_DEVICE_NODE));
			RtlZeroMemory(node, sizeof(HID_DEVICE_NODE));
			node->device_object = device_object;
			node->InterfaceDesc = mini_extension->InterfaceDesc;
			hid_relation[current_size] = node;
			current_size++;
			STACK_TRACE_DEBUG_INFO("Inserted one element: %I64x InferfaceDesc: %I64X device_object: %I64x \r\n", node->device_object, node->InterfaceDesc, device_object);
			STACK_TRACE_DEBUG_INFO("Added one element :%x \r\n", current_size);
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
 
/*
NTSTATUS SearchAllUsbCcgpRelation(
	PHID_DEVICE_NODE** device_object_list,
	PULONG size
)
{
	PDRIVER_OBJECT	  pDriverObj = NULL;
	PHID_DEVICE_NODE* hid_relation = *device_object_list;
	ULONG			  current_size = *size;
	
	if (!hid_relation)
	{
		hid_relation = (PHID_DEVICE_NODE*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PHID_DEVICE_NODE) * 100, 'ldih');
		RtlZeroMemory(hid_relation, sizeof(PHID_DEVICE_NODE) * 100);
	}

	GetDriverObjectByName(L"\\Driver\\Usbccgp", &pDriverObj);

	if (!pDriverObj)
	{
		return STATUS_UNSUCCESSFUL;
	}
	STACK_TRACE_DEBUG_INFO("DriverObj: %I64X \r\n", pDriverObj);

	PDEVICE_OBJECT device_object = pDriverObj->DeviceObject;
	while (device_object)
	{
		PDEVICE_OBJECT device_object_attached = device_object;
		while (device_object_attached)
		{
			PDRIVER_OBJECT HidDriverObj = NULL;
			GetDriverObjectByName(L"\\Driver\\HidUsb", &HidDriverObj);
			if (device_object_attached->DriverObject == HidDriverObj)
			{
				HID_USB_DEVICE_EXTENSION* mini_extension = NULL;
				HID_DEVICE_EXTENSION* hid_common_extension = NULL;
				WCHAR DeviceName[256] = { 0 };
				GetDeviceName(device_object_attached, DeviceName);
				STACK_TRACE_DEBUG_INFO("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws \r\n", device_object_attached, device_object_attached->DriverObject->DriverName.Buffer, DeviceName);

				hid_common_extension = (HID_DEVICE_EXTENSION*)device_object_attached->DeviceExtension;
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

				//TODO: Get all interfaces , and all keyboard device
				if (mini_extension->InterfaceDesc->Class == 3 &&
					mini_extension->InterfaceDesc->Protocol == 2)
				{
					HID_DEVICE_NODE* node = ExAllocatePool(NonPagedPool, sizeof(HID_DEVICE_NODE));
					RtlZeroMemory(node, sizeof(HID_DEVICE_NODE));
					node->device_object = device_object_attached;
					node->InterfaceDesc = mini_extension->InterfaceDesc;
					hid_relation[current_size] = node;
					current_size++;
					STACK_TRACE_DEBUG_INFO("Inserted one element: %I64x InferfaceDesc: %I64X device_object: %I64x \r\n", node->device_object, node->InterfaceDesc, device_object);
					STACK_TRACE_DEBUG_INFO("Added one element :%x \r\n", current_size);
				}
			}
			device_object_attached = device_object_attached->AttachedDevice;
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

*/