
#include <fltKernel.h> 
#include "HidHijack.h"

/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Types
////  

///////////////////////////////////////////////////////////////////////////////////////////////
////	Marco Utilities
////  
////  

///////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////
//// 
 
/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Prototype
//// 
////

/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Implementation
//// 
////

#define USBMON_WIN32_DEVICE_NAME_A		"\\\\.\\UsbMon"
#define USBMON_WIN32_DEVICE_NAME_W		L"\\\\.\\UsbMon"
#define USBMON_DEVICE_NAME_A			"\\Device\\UsbMon"
#define USBMON_DEVICE_NAME_W			L"\\Device\\UsbMon"
#define USBMON_DOS_DEVICE_NAME_A		"\\DosDevices\\UsbMon"
#define USBMON_DOS_DEVICE_NAME_W		L"\\DosDevices\\UsbMon"

typedef struct _DEVICE_EXTENSION
{
	ULONG  StateVariable;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define FILE_DEVICE_HIDE	0x8000

#define IOCTL_BASE	0x800

#define CTL_CODE_HIDE(i)	\
	CTL_CODE(FILE_DEVICE_HIDE, IOCTL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_USB_MAPPING_START		CTL_CODE_HIDE(1)			//初始化
#define IOCTL_USB_MAPPING_STOP		CTL_CODE_HIDE(2)			//初始化
#define IOCTL_HIDE_STOP				CTL_CODE_HIDE(3)			//初始化


//--------------------------------------------------------------------------------------//
NTSTATUS DispatchUsbInternalCtrl(
	IN PVOID InputBuffer,
	IN ULONG InputBufferLength,
	IN PVOID OutputBuffer,
	IN ULONG OutputBufferLength,
	IN ULONG IoControlCode,
	IN PIO_STATUS_BLOCK pIoStatus)
{	
	HANDLE hEvent = (HANDLE)InputBuffer;
	ULONG64 user_mode_addr = 0;
	PEPROCESS  hiddenProc;
	NTSTATUS	 status = STATUS_UNSUCCESSFUL;
 
	switch (IoControlCode)
	{
	case IOCTL_USB_MAPPING_START:

		if (!hEvent)
		{
			USB_COMMON_DEBUG_INFO("Null Event handle \r\n");
			break;
		}

		MapUsbDataToUserAddressSpace(&user_mode_addr, hEvent);

		if (user_mode_addr)
		{ 
			USB_COMMON_DEBUG_INFO("user_mode_addr: %I64x \r\n", user_mode_addr);
			*(PULONG64)OutputBuffer = user_mode_addr;
			pIoStatus->Information = sizeof(ULONG64);
			pIoStatus->Status = STATUS_SUCCESS;
		}
		break;

	case IOCTL_USB_MAPPING_STOP:
	{
 
	}
	break;

	case IOCTL_HIDE_STOP:
	{

	}
	break;

	default:
		break;
	}
	USB_COMMON_DEBUG_INFO("Return I/O \r\n");
	return status;
}
//--------------------------------------------------------------------------------------//
NTSTATUS UsbMonDeviceCtrlRoutine(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
)
{
	NTSTATUS			status = STATUS_SUCCESS;
	PIO_STATUS_BLOCK	ioStatus;
	PIO_STACK_LOCATION	pIrpStack;
	PDEVICE_EXTENSION	deviceExtension;
	PVOID				inputBuffer, outputBuffer;
	ULONG				inputBufferLength, outputBufferLength;
	ULONG				ioControlCode;
 
	pIrpStack = IoGetCurrentIrpStackLocation(Irp);
	deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ioStatus = &Irp->IoStatus;
	ioStatus->Status = STATUS_SUCCESS;		// Assume success
 
											//
											// Get the pointer to the input/output buffer and it's length
	inputBuffer = Irp->AssociatedIrp.SystemBuffer;
	inputBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	outputBuffer = Irp->AssociatedIrp.SystemBuffer;
	outputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ioControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;


	DbgPrintEx(0, 0, "start %X IOCTL_USB_MAPPING_START:%x \n", ioControlCode, IOCTL_USB_MAPPING_START);

	switch (pIrpStack->MajorFunction)
	{
	case IRP_MJ_CREATE:
		DbgPrint("[$ARK]<-IRP_MJ_CREATE.\n");
		break;

	case IRP_MJ_CLOSE:
		DbgPrint("[$ARK]->IRP_MJ_CLOSE.\n");
		break;

	case IRP_MJ_SHUTDOWN:
		DbgPrint("[$ARK] IRP_MJ_SHUTDOWN.\n");
		break;

	case IRP_MJ_DEVICE_CONTROL: 
		DispatchUsbInternalCtrl(inputBuffer,
			inputBufferLength,
			outputBuffer,
			outputBufferLength,
			ioControlCode,
			ioStatus);
		break;
	}

	//
	// TODO: if not pending, call IoCompleteRequest and Irp is freed.
	// 
	Irp->IoStatus.Status = ioStatus->Status;
	Irp->IoStatus.Information = ioStatus->Information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT); 

	return  status;
}
//--------------------------------------------------------------------------------------//
NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT* DeviceObject)
{
	PDEVICE_OBJECT		   DeviceObj;
	UNICODE_STRING		ntDeviceName;
	UNICODE_STRING		dosDeviceName;
	NTSTATUS			status;
	RtlInitUnicodeString(&ntDeviceName, USBMON_DEVICE_NAME_W);

	status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION),		// DeviceExtensionSize
		&ntDeviceName,					// DeviceName
		FILE_DEVICE_UNKNOWN,			// DeviceType
		0,								// DeviceCharacteristics
		TRUE,							// Exclusive
		&DeviceObj					// [OUT]
	);

	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(0,0,"[$XTR] IoCreateDevice failed(0x%x).\n", status);
		return STATUS_UNSUCCESSFUL;
	}

	DeviceObj->Flags |= DO_BUFFERED_IO;
	  
	RtlInitUnicodeString(&dosDeviceName, USBMON_DOS_DEVICE_NAME_W);

	status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);

	if (!NT_SUCCESS(status))
	{
		DbgPrint(0,0,"[$XTR] IoCreateSymbolicLink failed(0x%x).\n", status);
		return STATUS_UNSUCCESSFUL;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] =
	DriverObject->MajorFunction[IRP_MJ_CLOSE]  =
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = UsbMonDeviceCtrlRoutine;

	*DeviceObject = DeviceObj;

	return STATUS_SUCCESS;
}
//-----------------------------------------------------------------------------------------
VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
)
{ 
	UNREFERENCED_PARAMETER(DriverObject);
	UnInitializeHidPenetrate();
	return;
} 

//----------------------------------------------------------------------------------------//
NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT object, 
	_In_ PUNICODE_STRING String)
{ 
	NTSTATUS						  status = STATUS_UNSUCCESSFUL; 
	PDEVICE_OBJECT				DeviceObject = NULL; 

	status = InitializeHidPenetrate(USB2);
	if (!NT_SUCCESS(status))
	{
		USB_COMMON_DEBUG_INFO("Init Pene System Error ");
		return status;
	}	
	
	status = CreateDevice(object, &DeviceObject);
	if (!NT_SUCCESS(status))
	{
		USB_COMMON_DEBUG_INFO("CreateDevice Error ");  
		return status;
	}

	object->DriverUnload			     = DriverUnload;
	return status;
} 
//----------------------------------------------------------------------------------------//