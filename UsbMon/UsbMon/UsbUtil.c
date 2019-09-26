#include "UsbUtil.h"
#include "ReportUtil.h"

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

BOOLEAN g_IsTestReported[0xFF] = {0};
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
	WCHAR* Usbhub = NULL;
	
	
	Usbhub = GetUsbHubDriverNameByVersion(USB);
	USB_DEBUG_INFO_LN_EX("Usb Hub: %ws ", Usbhub);

	Usbhub = GetUsbHubDriverNameByVersion(USB2);
	USB_DEBUG_INFO_LN_EX("Usb Hub2: %ws ", Usbhub);

	Usbhub = GetUsbHubDriverNameByVersion(USB3);
	USB_DEBUG_INFO_LN_EX("Usb Hub3: %ws ", Usbhub);

	Usbhub = GetUsbHubDriverNameByVersion(USB3_NEW);
	USB_DEBUG_INFO_LN_EX("Usb Hub3_w8 above: %ws", Usbhub);

	Usbhub = GetUsbHubDriverNameByVersion(USB_COMPOSITE);
	USB_DEBUG_INFO_LN_EX("Usb CCGP: %ws ", Usbhub);

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
	HID_DESCRIPTOR* hid_desc = NULL;
	
	USB_DEBUG_INFO_LN_EX("-----------------------------------------------------------------------------\r\n");
	USB_DEBUG_INFO_LN_EX(" --->PhysicalDeviceObject: %I64X \r\n", hid_common_extension->PhysicalDeviceObject);
	USB_DEBUG_INFO_LN_EX(" --->DriverObject: %I64X \r\n", hid_common_extension->PhysicalDeviceObject->DriverObject);
	USB_DEBUG_INFO_LN_EX(" --->NextDeviceObject: %I64X \r\n", hid_common_extension->NextDeviceObject);
	USB_DEBUG_INFO_LN_EX("  --->Status: %I64X \r\n", mini_extension->status);
	USB_DEBUG_INFO_LN_EX("  --->DeviceDesc: %I64X \r\n", mini_extension->DeviceDesc);
	USB_DEBUG_INFO_LN_EX("		--->bDeviceClass: %I64X \r\n", mini_extension->DeviceDesc->bDeviceClass);
	USB_DEBUG_INFO_LN_EX("		--->bDeviceSubClass: %I64X \r\n", mini_extension->DeviceDesc->bDeviceSubClass);
	USB_DEBUG_INFO_LN_EX("		--->bDeviceProtocol: %I64X \r\n", mini_extension->DeviceDesc->bDeviceProtocol);
	USB_DEBUG_INFO_LN_EX("		--->bDescriptorType: %I64X \r\n", mini_extension->DeviceDesc->bDescriptorType);
	USB_DEBUG_INFO_LN_EX("		--->bLength: %I64X \r\n", mini_extension->DeviceDesc->bLength);
	USB_DEBUG_INFO_LN_EX("		--->bMaxPacketSize0: %I64X \r\n", mini_extension->DeviceDesc->bMaxPacketSize0);
	USB_DEBUG_INFO_LN_EX("		--->bcdUSB: %I64X \r\n", mini_extension->DeviceDesc->bcdUSB);
	USB_DEBUG_INFO_LN_EX("		--->idProduct: %I64X \r\n", mini_extension->DeviceDesc->idProduct);
	USB_DEBUG_INFO_LN_EX("		--->idVendor: %I64X \r\n", mini_extension->DeviceDesc->idVendor);
	USB_DEBUG_INFO_LN_EX("  --->InterfaceDescDesc: %I64X \r\n", mini_extension->InterfaceDesc);
	USB_DEBUG_INFO_LN_EX("		--->Class: %I64X \r\n", mini_extension->InterfaceDesc->Class);
	USB_DEBUG_INFO_LN_EX("		--->SubClass: %I64X \r\n", mini_extension->InterfaceDesc->SubClass);
	USB_DEBUG_INFO_LN_EX("		--->Protocol: %I64X \r\n", mini_extension->InterfaceDesc->Protocol);
	USB_DEBUG_INFO_LN_EX("		--->NumOfPipes: %I64X \r\n", mini_extension->InterfaceDesc->NumberOfPipes);
	USB_DEBUG_INFO_LN_EX("		--->Length: %I64X \r\n", mini_extension->InterfaceDesc->Length);
	USB_DEBUG_INFO_LN_EX("		--->AlternateSetting: %I64X \r\n\r\n", mini_extension->InterfaceDesc->AlternateSetting);

 	for (i = 0; i < mini_extension->InterfaceDesc->NumberOfPipes; i++)
	{
		USBD_PIPE_INFORMATION* Pipe_Information = &mini_extension->InterfaceDesc->Pipes[i];
		USB_DEBUG_INFO_LN_EX("  --->Pipe id: %x Information: %I64X ",i, Pipe_Information);
		USB_DEBUG_INFO_LN_EX("		--->PipeHandle: %I64X ", Pipe_Information->PipeHandle); 
		USB_DEBUG_INFO_LN_EX("		--->PipeAddress: %I64X ", Pipe_Information->EndpointAddress); 
		USB_DEBUG_INFO_LN_EX("		--->PipeInterval: %I64X ", Pipe_Information->Interval);
		USB_DEBUG_INFO_LN_EX("		--->PipeInterval: %I64X ", Pipe_Information->MaximumPacketSize);
		USB_DEBUG_INFO_LN_EX("		--->PipeInterval: %I64X ", Pipe_Information->MaximumTransferSize);
	}


	hid_desc = &(mini_extension)->HidDescriptor; 
	 
	USB_DEBUG_INFO_LN_EX("  --->HidDescriptor: %IX ", &hid_desc);
	USB_DEBUG_INFO_LN_EX("		--->bcdHID: %X ", hid_desc->bcdHID);
	USB_DEBUG_INFO_LN_EX("		--->bCountry: %X ", hid_desc->bCountry);
	USB_DEBUG_INFO_LN_EX("		--->bDescriptorType: %X ", hid_desc->bDescriptorType);
	USB_DEBUG_INFO_LN_EX("		--->bLength: %X ", hid_desc->bLength);
	USB_DEBUG_INFO_LN_EX("		--->bNumDescriptors: %X ", hid_desc->bNumDescriptors);
	USB_DEBUG_INFO_LN_EX("		--->bReportType: %X ", hid_desc->DescriptorList->bReportType);
	USB_DEBUG_INFO_LN_EX("		--->wReportLength: %X ", hid_desc->DescriptorList->wReportLength);

	USB_DEBUG_INFO_LN_EX("-----------------------------------------------------------------------------");

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

	USB_DEBUG_INFO_LN_EX("---------------URB HEADER------------- ");
	USB_DEBUG_INFO_LN_EX("- Length: %x ", urb_header.Length);
	USB_DEBUG_INFO_LN_EX("- Function: %x ", urb_header.Function);
	USB_DEBUG_INFO_LN_EX("- Status: %x ", urb_header.Status);
	USB_DEBUG_INFO_LN_EX("- UsbdDeviceHandle: %x ", urb_header.UsbdDeviceHandle);
	USB_DEBUG_INFO_LN_EX("- UsbdFlags: %x ", urb_header.UsbdFlags);
	USB_DEBUG_INFO_LN_EX("---------------URB Function------------- ");

	switch (urb_header.Function)
	{
		/*
#if (NTDDI_VERSION >= NTDDI_WIN8)
	case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER_USING_CHAINED_MDL:
		break;
	case URB_FUNCTION_OPEN_STATIC_STREAMS:
		break;
	case URB_FUNCTION_CLOSE_STATIC_STREAMS:
		break;
#endif*/
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

		//			USB_MON_COMMON_DBG_BREAK("_URB_BULK_OR_INTERRUPT_TRANSFER \r\n"); 
		/*			if (data->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
		{
		USB_MON_COMMON_DBG_BREAK("USBD_TRANSFER_DIRECTION_IN \r\n");
		}
		if (data->TransferFlags & USBD_TRANSFER_DIRECTION_OUT)
		{
		USB_MON_COMMON_DBG_BREAK("USBD_TRANSFER_DIRECTION_OUT \r\n");

		}
		if (data->TransferFlags & USBD_SHORT_TRANSFER_OK)
		{
		USB_MON_COMMON_DBG_BREAK("USBD_SHORT_TRANSFER_OK \r\n");
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
 
//-----------------------------------------------------------------------------------//
PHIDP_REPORT_IDS GetReportIdentifier(	
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc, 
	_In_ ULONG 				  reportId
)
{
    PHIDP_REPORT_IDS result = NULL;
    PHIDP_DEVICE_DESC deviceDesc = pDesc;
    ULONG i;
	if(!deviceDesc)
	{
		return result;
	}
	if(!deviceDesc->ReportIDs)
	{
		return result;
	}
	
	 
    for (i = 0; i < deviceDesc->ReportIDsLength; i++)
	{
        if (deviceDesc->ReportIDs[i].ReportID == reportId)
		{
            result = &deviceDesc->ReportIDs[i];
            break;
		}
    }  
    return result;
}

//-----------------------------------------------------------------------------------//
PHIDP_COLLECTION_DESC GetCollectionDesc(	
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_In_ ULONG 				collectionId
)
{
    PHIDP_COLLECTION_DESC result = NULL;
    PHIDP_DEVICE_DESC deviceDesc =  pDesc;
    ULONG i;

    if (deviceDesc->CollectionDesc){
        for (i = 0; i < deviceDesc->CollectionDescLength; i++){
            if (deviceDesc->CollectionDesc[i].CollectionNumber == collectionId){
                result = &deviceDesc->CollectionDesc[i];
                break;
            }
        }
    }
    return result;
}
//-----------------------------------------------------------------------------------// 
NTSTATUS VerifyUsageInCollection( 
	_In_ ULONG					 Usage,
	_In_ UCHAR*					 pSourceReport, 
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_Out_ PHIDP_COLLECTION_DESC* pCollection
)
{
	NTSTATUS status  = STATUS_UNSUCCESSFUL;
	ULONG 	colIndex = 0;
	UCHAR ReportId = 0;
    PHIDP_REPORT_IDS 		reportIdentifier;
	PHIDP_COLLECTION_DESC	collectionDesc; 
 
	if(!pSourceReport  || !pDesc || !pCollection)
	{	
		USB_DEBUG_INFO_LN_EX("pSourceReport: %I64x  pDesc: %I64x pCollection: %I64x",
			pSourceReport, 
			pDesc, 
			pCollection
		); 
		
		if(pCollection)
		{
			*pCollection = NULL;	
		}
		
 		return status;
	} 
	
	if(pDesc->ReportIDs[0].ReportID == 0)
	{
		ReportId = 0; 
	}
	else
	{
		ReportId = (ULONG)*pSourceReport; 
	}
	
	USB_DEBUG_INFO_LN_EX("Report ID: %x", ReportId);

 	reportIdentifier = GetReportIdentifier(pDesc, ReportId);	
	
	USB_DEBUG_INFO_LN_EX("Report Identifier: %I64x ", reportIdentifier);
	
    if (!reportIdentifier)
	{       
		USB_DEBUG_INFO_LN_EX("reportIdentifier: %I64x",reportIdentifier );
		
		*pCollection = NULL;	
		status =  STATUS_UNSUCCESSFUL;
		return status;
	}	

    collectionDesc = GetCollectionDesc(pDesc, reportIdentifier->CollectionNumber);
	
	USB_DEBUG_INFO_LN_EX("CollectionDesc: %I64x  reportIdentifier->CollectionNumber: %x ", collectionDesc,  reportIdentifier->CollectionNumber);

	if(!collectionDesc)
	{	
		USB_DEBUG_INFO_LN_EX("collectionDesc: %I64x",collectionDesc);
		
		*pCollection = NULL;	
		status =  STATUS_UNSUCCESSFUL;
		return status;
	}
	
	if(collectionDesc->Usage == Usage  && collectionDesc->UsagePage == HID_GENERIC_DESKTOP_PAGE)
	{
		if(!g_IsTestReported[Usage])
		{
			USB_DEBUG_INFO_LN_EX("It is mou collection");
			g_IsTestReported[Usage] = TRUE;			
		} 
		
		*pCollection = collectionDesc;
		status = STATUS_SUCCESS; 
	}	
	else
	{	
		*pCollection = NULL;		
		status =  STATUS_UNSUCCESSFUL;
	}
	return status;
}
 
//----------------------------------------------------------------------------------------//
 
LONG ReadHidCoordinate(
	_In_	PCHAR Src , 
	_In_	ULONG ByteOffset, 
	_In_	ULONG BitOffset,  
	_In_	ULONG BitLen)
{
	LONG64    Ret=0;
	UCHAR*    lpret = (UCHAR*)&Ret ; 
	long      higthNullbitlen=64-(BitLen+BitOffset);
	
	if(!Src)
	{
		return 0; 
	}
	
	memcpy(lpret,Src+ByteOffset,(BitLen+BitOffset+7)/8);
	
	Ret=(Ret<<higthNullbitlen);
	
	if(Ret&0x8000000000000000)
	{
		ULONGLONG tmp = -1;
		Ret=(Ret>>(higthNullbitlen+BitOffset));
		Ret=Ret|(tmp<<BitLen);
	}
	else 
	{
		Ret=(Ret>>(higthNullbitlen+BitOffset));
	}
	
	return (long)Ret;
}  

//-----------------------------------------------------------------------------------//
