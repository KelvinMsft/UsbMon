                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #include "UsbHid.h"
#include "UsbUtil.h" 


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Global/Extern Variable 
//// 
HID_DEVICE_LIST* g_hid_relation = NULL;

HIDP_DEVICE_DESC* g_hid_collection = NULL;

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
/*
********************************************************************************
*  GetHidclassCollection
********************************************************************************
*
*/
PHIDCLASS_COLLECTION GetHidclassCollection(FDO_EXTENSION *fdoExtension, ULONG collectionId)
{
	PHIDCLASS_COLLECTION result = NULL;
	PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
	ULONG i;
	 
		for (i = 0; i < deviceDesc->CollectionDescLength; i++) {
			if (fdoExtension->classCollectionArray[i].CollectionNumber == collectionId) {
				result = &fdoExtension->classCollectionArray[i];
				break;
			}
		} 
	ASSERT(result);
	return result;
}

/*
********************************************************************************
*  GetCollectionDesc
********************************************************************************
*
*
*/
PHIDP_COLLECTION_DESC GetCollectionDesc(FDO_EXTENSION *fdoExtension, ULONG collectionId)
{
	PHIDP_COLLECTION_DESC result = NULL;
	PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
	ULONG i;

	if (deviceDesc->CollectionDesc) {
		for (i = 0; i < fdoExtension->deviceDesc.CollectionDescLength; i++) {
			if (deviceDesc->CollectionDesc[i].CollectionNumber == collectionId) {
				result = &deviceDesc->CollectionDesc[i];
				break;
			}
		}
	}

	ASSERT(result);
	return result;
}

//---------------------------------------------------------------------------------------------------------//
BOOLEAN  IsKeyboardOrMouseDevice(
	_In_  PDEVICE_OBJECT				   device_object, 
	_Out_ PHID_USB_DEVICE_EXTENSION*   hid_mini_extension
)
{
	HID_USB_DEVICE_EXTENSION* mini_extension    = NULL;
	HIDCLASS_DEVICE_EXTENSION* hid_common_extension  = NULL;
	WCHAR						DeviceName[256] = { 0 };
	int i = 0;
	GetDeviceName(device_object, DeviceName);

 
	hid_common_extension = (HIDCLASS_DEVICE_EXTENSION*)device_object->DeviceExtension;
	if (!hid_common_extension)
	{
		return FALSE;
	}
 
	 
	//STACK_TRACE_DEBUG_INFO("Extension_common: %I64X sizeof: %x \r\n", hid_common_extension, sizeof(HID_USB_DEVICE_EXTENSION));
	mini_extension = (HID_USB_DEVICE_EXTENSION*)hid_common_extension->hidExt.MiniDeviceExtension;
	if (!mini_extension)
	{
		return FALSE;
	} 	
	

 
	//DumpHidMiniDriverExtension(hid_common_extension); 

	if (!hid_common_extension->isClientPdo)
	{	
		*hid_mini_extension = NULL;
		return FALSE; 
	}
	 
	if ( mini_extension->InterfaceDesc->Class == 3 &&			//HidClass Device
		(mini_extension->InterfaceDesc->Protocol == 1 ||		//Keyboard
		 mini_extension->InterfaceDesc->Protocol == 2))			//Mouse
	{

	//	STACK_TRACE_DEBUG_INFO("Signature: %I64x \r\n", hid_common_extension->Signature);
	//	STACK_TRACE_DEBUG_INFO("DescLen: %x \r\n", hid_common_extension->fdoExt.rawReportDescriptionLength); 
		FDO_EXTENSION       *fdoExt = NULL;
		PDO_EXTENSION       *pdoExt = NULL;

		if (hid_common_extension->isClientPdo)
		{
			STACK_TRACE_DEBUG_INFO("---------------------------------------------------------------------------------------------------- \r\n");

			STACK_TRACE_DEBUG_INFO("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws \r\n", device_object, device_object->DriverObject->DriverName.Buffer, DeviceName);

			pdoExt = &hid_common_extension->pdoExt; 
			STACK_TRACE_DEBUG_INFO("sizeof FDO_EXTENSION : %X  \r\n", sizeof(FDO_EXTENSION));

			STACK_TRACE_DEBUG_INFO("hid_common_extension: %I64x \r\n", hid_common_extension);
			STACK_TRACE_DEBUG_INFO("pdoExt: %I64x \r\n", pdoExt); 
			STACK_TRACE_DEBUG_INFO("OFFSET %x \r\n", FIELD_OFFSET(PDO_EXTENSION,deviceFdoExt));
			STACK_TRACE_DEBUG_INFO("pdoExt Offset: %I64x \r\n", &hid_common_extension->pdoExt.deviceFdoExt);
			STACK_TRACE_DEBUG_INFO("pdoExt Offset: %I64x \r\n", ((ULONG64)hid_common_extension) + 0x60);		//For Win7 deviceFdoExt offset by HIDCLASS_DEVICE_EXTENSION->PDO_EXTENSION.backptr
		
			WCHAR name[256] = { 0 };
			HIDCLASS_DEVICE_EXTENSION* addr = (HIDCLASS_DEVICE_EXTENSION*)pdoExt->deviceFdoExt;
			fdoExt = &addr->fdoExt;
			GetDeviceName(fdoExt->fdo, name);
			STACK_TRACE_DEBUG_INFO("Pdo->fdoExt: %I64x \r\n", fdoExt);
			STACK_TRACE_DEBUG_INFO("Pdo->fdo: %I64x \r\n", fdoExt->fdo);
			STACK_TRACE_DEBUG_INFO("Pdo->fdo DriverName: %ws \r\n", fdoExt->fdo->DriverObject->DriverName.Buffer);
			STACK_TRACE_DEBUG_INFO("Pdo->fdo DeviceName: %ws \r\n", name); 
 			STACK_TRACE_DEBUG_INFO("Pdo->CollectionIndex: %I64x \r\n", pdoExt->collectionIndex);
			STACK_TRACE_DEBUG_INFO("Pdo->CollectionNum: %I64x \r\n", pdoExt->collectionNum);

			HIDP_DEVICE_DESC* tmp = (HIDP_DEVICE_DESC*)((PUCHAR)fdoExt + 0x58);
		 

			STACK_TRACE_DEBUG_INFO("deviceDesc(FIELD_OFFSET): %x \r\n", FIELD_OFFSET(FDO_EXTENSION, deviceDesc));  

			STACK_TRACE_DEBUG_INFO("(PUCHAR)fdoExt: %I64x \r\n", (PUCHAR)fdoExt + 0x58);
			STACK_TRACE_DEBUG_INFO("(PUCHAR)fdoExt: %I64x \r\n",  &fdoExt->deviceDesc);
 
			for (int i = 0; i < tmp->CollectionDescLength; i++)
			{
 				PHIDP_COLLECTION_DESC collectionDesc = &tmp->CollectionDesc[i];
				//if (collectionDesc->CollectionNumber == pdoExt->collectionNum)
				//{
					STACK_TRACE_DEBUG_INFO("Usage: %x \r\n", collectionDesc->Usage);
					STACK_TRACE_DEBUG_INFO("UsagePage: %x \r\n", collectionDesc->UsagePage);
					STACK_TRACE_DEBUG_INFO("InputLength: %x \r\n", collectionDesc->InputLength);
					STACK_TRACE_DEBUG_INFO("OutputLength: %x \r\n", collectionDesc->OutputLength);
					STACK_TRACE_DEBUG_INFO("CollectionNumber: %x \r\n", collectionDesc->CollectionNumber);
				//}
			}


			STACK_TRACE_DEBUG_INFO("---------------------------------------------------------------------------------------------------- \r\n");

		}
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
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           