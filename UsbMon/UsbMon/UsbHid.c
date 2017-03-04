#include "UsbHid.h"
#include "UsbUtil.h"


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Global/Extern Variable 
//// 
HID_DEVICE_LIST* g_hid_relation = NULL;


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
	_In_ HID_USB_DEVICE_EXTENSION* mini_extension
)
{
	if (!device_object || !mini_extension)
	{
		return NULL;
	}

	HID_DEVICE_NODE* node = ExAllocatePool(NonPagedPool, sizeof(HID_DEVICE_NODE));
	if (!node)
	{
		return NULL;
	}

	RtlZeroMemory(node, sizeof(HID_DEVICE_NODE));
	node->device_object  = device_object;
	node->mini_extension = mini_extension;

	return node;
}
//---------------------------------------------------------------------------------------------------------//
BOOLEAN  IsKeyboardOrMouseDevice(
	_In_  PDEVICE_OBJECT				   device_object, 
	_Out_ PHID_USB_DEVICE_EXTENSION*   hid_mini_extension
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
		*hid_mini_extension = mini_extension;
		return TRUE;
	}
	 
	return FALSE;
}
//---------------------------------------------------------------------------------------------------------//
NTSTATUS FreeHidRelation()
{
	//Free White List
	if (g_hid_relation)
	{
		if (g_hid_relation->head)
		{
			CHAINLIST_SAFE_FREE(g_hid_relation->head);
		}
		ExFreePool(g_hid_relation);
		g_hid_relation = NULL;
	}
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS ReleaseHidRelation()
{
	NTSTATUS status = STATUS_SUCCESS;
	if (!g_hid_relation)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}
	InterlockedIncrement64(&g_hid_relation->RefCount);
	return status;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS AcquireHidRelation(
	_Out_ PHID_DEVICE_LIST* device_object_list,
	_Out_ PULONG size
)
{
	*device_object_list = g_hid_relation;
	*size = g_hid_relation->currentSize;
	InterlockedIncrement64(&g_hid_relation->RefCount);
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitHidRelation(
	_Out_ PHID_DEVICE_LIST* device_object_list,
	_Out_ PULONG size
)
{
	PDEVICE_OBJECT	 device_object = NULL;
	PDRIVER_OBJECT	    pDriverObj = NULL;
	ULONG			  current_size = 0;
	
	
	if (!g_hid_relation)
	{
		g_hid_relation = (HID_DEVICE_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HID_DEVICE_LIST), 'ldih');
	}

	if (!g_hid_relation)
	{
		return STATUS_UNSUCCESSFUL;
	} 

	RtlZeroMemory(g_hid_relation, sizeof(HID_DEVICE_LIST));

	g_hid_relation->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, NULL, 0);

	if (!g_hid_relation->head)
	{
		return STATUS_UNSUCCESSFUL;
	}

	GetDriverObjectByName(HID_USB_DEVICE, &pDriverObj);
	if (!pDriverObj)
	{
		FreeHidRelation();
		return STATUS_UNSUCCESSFUL;
	}

	STACK_TRACE_DEBUG_INFO("DriverObj: %I64X \r\n", pDriverObj);

	device_object = pDriverObj->DeviceObject;
	while (device_object)
	{
		HID_USB_DEVICE_EXTENSION* mini_extension = NULL; 
		if (IsKeyboardOrMouseDevice(device_object, &mini_extension))
		{
			if (mini_extension && g_hid_relation)
			{
				HID_DEVICE_NODE* node = CreateHidDeviceNode(device_object, mini_extension);
				AddToChainListTail(g_hid_relation->head, node);
				current_size++;
				STACK_TRACE_DEBUG_INFO("Inserted one element: %I64x InferfaceDesc: %I64X device_object: %I64x \r\n", node->device_object, node->mini_extension, device_object);
				STACK_TRACE_DEBUG_INFO("Added one element :%x \r\n", current_size);
			}
		}
 		device_object = device_object->NextDevice;
	}

	if (current_size > 0 && g_hid_relation)
	{
		*device_object_list = g_hid_relation;
		*size = current_size;
		g_hid_relation->RefCount = 1;
		return STATUS_SUCCESS;
	}  

	FreeHidRelation();
	return STATUS_UNSUCCESSFUL;
}
 