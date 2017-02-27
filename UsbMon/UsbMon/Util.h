#pragma once
#include "usb.h"
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

/// Sets a break point that works only when a debugger is present
#if !defined(HYPERPLATFORM_COMMON_DBG_BREAK)
#define STACK_TRACE_COMMON_DBG_BREAK() \
  if (KD_DEBUGGER_NOT_PRESENT) {         \
  } else {                               \
    __debugbreak();                      \
  }                                      \
  (void*)(0)
#endif

#define STACK_TRACE_DEBUG_INFO(format, ...) DbgPrintEx(0,0,format,__VA_ARGS__)

#define DELAY_ONE_MICROSECOND 	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)

VOID KeSleep(LONG msec)
{
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;
	KeDelayExecutionThread(KernelMode, 0, &my_interval);
} 
//----------------------------------------------------------------------------------------//
NTSTATUS GetDriverObjectByName(WCHAR* name, PDRIVER_OBJECT* pDriverObj)
{
	// use such API  
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING DriverName = { 0 };

	RtlInitUnicodeString(&DriverName, name);

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

void GetDeviceName(PDEVICE_OBJECT device_object, WCHAR* DeviceNameBuf)
{
	UCHAR	 Buffer[sizeof(OBJECT_NAME_INFORMATION) + 512];
	POBJECT_NAME_INFORMATION NameInfo = (PVOID)Buffer;
	ULONG Length;
	if (device_object)
	{
		ObQueryNameString(device_object, NameInfo, sizeof(Buffer), &Length);
	}
	RtlMoveMemory(DeviceNameBuf, NameInfo->Name.Buffer, NameInfo->Name.Length);
}

VOID DumpUrb(PURB urb)
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



