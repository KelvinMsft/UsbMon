#include<fltKernel.h>
#include "Urb.h"
#include "Usbdlib.h"
#define GET_MINIDRIVER_DEVICE_EXTENSION(DO) ((HID_USB_DEVICE_EXTENSION*) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))
#define GET_NEXT_DEVICE_OBJECT(DO) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

NTSTATUS HumCallUSB(IN PDEVICE_OBJECT DeviceObject, IN PURB Urb)
/*++

Routine Description:

Passes a URB to the USBD class driver

Arguments:

DeviceObject - pointer to the device object for this instance of a UTB

Urb - pointer to Urb request block

Return Value:

STATUS_SUCCESS if successful,
STATUS_UNSUCCESSFUL otherwise

--*/
{
	NTSTATUS ntStatus;
	HID_USB_DEVICE_EXTENSION* DeviceExtension;
	PIRP Irp;
	KEVENT event;
	IO_STATUS_BLOCK ioStatus;
	PIO_STACK_LOCATION NextStack;
	
	USB_MON_DEBUG_INFO("HumCallUSB Entry");

	USB_MON_DEBUG_INFO("DeviceObject = %x", DeviceObject);

	DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

	USB_MON_DEBUG_INFO("DeviceExtension = %x", DeviceExtension);

	//
	// issue a synchronous request to read the UTB
	//

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
		GET_NEXT_DEVICE_OBJECT(DeviceObject),
		NULL,
		0,
		NULL,
		0,
		TRUE, /* INTERNAL */
		&event,
		&ioStatus);

	if (Irp) {
		(2, ("Irp = %x", Irp));

		USB_MON_DEBUG_INFO(2, ("PDO = %x", GET_NEXT_DEVICE_OBJECT(DeviceObject)));

		//
		// pass the URB to the USB 'class driver'
		//

		NextStack = IoGetNextIrpStackLocation(Irp);
		ASSERT(NextStack != NULL);

		USB_MON_DEBUG_INFO("NextStack = %x", NextStack);

		NextStack->Parameters.Others.Argument1 = Urb;

		USB_MON_DEBUG_INFO("Calling USBD");

		ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

		USB_MON_DEBUG_INFO("IoCallDriver(USBD) = %x", ntStatus);

		if (ntStatus == STATUS_PENDING) {
			NTSTATUS waitStatus;

			/*
			*  Specify a timeout of 5 seconds for this call to complete.
			*  Negative timeout indicates time relative to now (in 100ns units).
			*
			*  BUGBUG - timeout happens rarely for HumGetReportDescriptor
			*           when you plug in and out repeatedly very fast.
			*           Figure out why this call never completes.
			*/
			static LARGE_INTEGER timeout = { (ULONG)-50000000, 0xFFFFFFFF };

			USB_MON_DEBUG_INFO("Wait for single object");

			waitStatus = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, &timeout);
			if (waitStatus == STATUS_TIMEOUT) {

				USB_MON_DEBUG_INFO("URB timed out after 5 seconds in HumCallUSB() !!");


				// BUGBUG - test timeout with faulty nack-ing device from glens
				// BUGBUG - also need to cancel read irps at HIDCLASS level

				//
				//  Cancel the Irp we just sent.
				//
				IoCancelIrp(Irp);

				//
				//  Now wait for the Irp to be cancelled/completed below
				//
				waitStatus = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);

				/*
				*  Note - Return STATUS_IO_TIMEOUT, not STATUS_TIMEOUT.
				*  STATUS_IO_TIMEOUT is an NT error status, STATUS_TIMEOUT is not.
				*/
				ioStatus.Status = STATUS_IO_TIMEOUT;
			}

			USB_MON_DEBUG_INFO("Wait for single object returned %x", waitStatus);

			//
			// USBD maps the error code for us
			//
			ntStatus = ioStatus.Status;
		}

		USB_MON_DEBUG_INFO("URB status = %x status = %x", Urb->UrbHeader.Status, ntStatus);
	}
	else {
		ntStatus = STATUS_INSUFFICIENT_RESOURCES;
	}

	USB_MON_DEBUG_INFO("HumCallUSB Exit = %x", ntStatus);

	return ntStatus;
}


NTSTATUS
NTAPI
Hid_PnpCompletion(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)
{
	//
	// signal event
	//
	KeSetEvent(Context, 0, FALSE);

	//
	// done
	//
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
Hid_DispatchUrb(
	IN PDEVICE_OBJECT DeviceObject,
	IN PURB Urb)
{
	PIRP Irp;
	KEVENT Event;
	PHID_DEVICE_EXTENSION DeviceExtension;
	IO_STATUS_BLOCK IoStatus;
	PIO_STACK_LOCATION IoStack;
	NTSTATUS Status;

	//
	// init event
	//
	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	//
	// get device extension
	//
	DeviceExtension = DeviceObject->DeviceExtension;

	//
	// build irp
	//
	Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
		DeviceExtension->NextDeviceObject,
		NULL,
		0,
		NULL,
		0,
		TRUE,
		&Event,
		&IoStatus);
	if (!Irp)
	{
		//
		// no memory
		//
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//
	// get next stack location
	//
	IoStack = IoGetNextIrpStackLocation(Irp);

	//
	// store urb
	//
	IoStack->Parameters.Others.Argument1 = Urb;

	//
	// set completion routine
	//
	IoSetCompletionRoutine(Irp, Hid_PnpCompletion, &Event, TRUE, TRUE, TRUE);

	//
	// call driver
	//
	Status = IoCallDriver(DeviceExtension->NextDeviceObject, Irp);

	//
	// wait for the request to finish
	//
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
	}

	//
	// complete request
	//
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	if (Status == STATUS_PENDING)
	{
		//
		// get final status
		//
		Status = IoStatus.Status;
	}
	 


	//
	// done
	//
	return Status;
}
NTSTATUS
Hid_GetDescriptor(
	IN PDEVICE_OBJECT DeviceObject,
	IN USHORT UrbFunction,
	IN USHORT UrbLength,
	IN OUT PVOID *UrbBuffer,
	IN OUT PULONG UrbBufferLength,
	IN UCHAR DescriptorType,
	IN UCHAR Index,
	IN USHORT LanguageIndex)
{
	PURB Urb;
	NTSTATUS Status;
	UCHAR Allocated = FALSE;

	//
	// allocate urb
	//
	Urb = ExAllocatePoolWithTag(NonPagedPool, UrbLength, 'urbt');
	if (!Urb)
	{
		//
		// no memory
		//
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//
	// is there an urb buffer
	//
	if (!*UrbBuffer)
	{
		//
		// allocate buffer
		//
		*UrbBuffer = ExAllocatePoolWithTag(NonPagedPool, *UrbBufferLength, 'urbt');
		if (!*UrbBuffer)
		{
			//
			// no memory
			//
			ExFreePoolWithTag(Urb, 'urbt');
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		//
		// zero buffer
		//
		RtlZeroMemory(*UrbBuffer, *UrbBufferLength);
		Allocated = TRUE;
	}

	//
	// zero urb
	//
	RtlZeroMemory(Urb, UrbLength);

	//
	// build descriptor request
	//
	UsbBuildGetDescriptorRequest(Urb, UrbLength, DescriptorType, Index, LanguageIndex, *UrbBuffer, NULL, *UrbBufferLength, NULL);

	//
	// set urb function
	//
	Urb->UrbHeader.Function = UrbFunction;

	//
	// dispatch urb
	//
	Status = Hid_DispatchUrb(DeviceObject, Urb);

	//
	// did the request fail
	//
	if (!NT_SUCCESS(Status))
	{
		if (Allocated)
		{
			//
			// free allocated buffer
			//
			ExFreePoolWithTag(*UrbBuffer, 'urbt');
			*UrbBuffer = NULL;
		}

		//
		// free urb
		//
		ExFreePoolWithTag(Urb, 'urbt');
		*UrbBufferLength = 0;
		return Status;
	}

	//
	// did urb request fail
	//
	if (!NT_SUCCESS(Urb->UrbHeader.Status))
	{
		if (Allocated)
		{
			//
			// free allocated buffer
			//
			ExFreePoolWithTag(*UrbBuffer, 'urbt');
			*UrbBuffer = NULL;
		}

		//
		// free urb
		//
		ExFreePoolWithTag(Urb, 'urbt');
		*UrbBufferLength = 0;
		return STATUS_UNSUCCESSFUL;
	}

	//
	// store result length
	//
	*UrbBufferLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;

	//
	// free urb
	//
	ExFreePoolWithTag(Urb, 'urbt');

	//
	// completed successfully
	//
	return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
HidUsb_GetReportDescriptor(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PHID_USB_DEVICE_EXTENSION HidDeviceExtension;
	PHID_DEVICE_EXTENSION DeviceExtension;
	PVOID Report = NULL;
	ULONG BufferLength, Length;
	PIO_STACK_LOCATION IoStack;
	NTSTATUS Status;

	//
	// get device extension
	//
	DeviceExtension = DeviceObject->DeviceExtension;
	HidDeviceExtension = DeviceExtension->MiniDeviceExtension;

	//
	// sanity checks
	//
	ASSERT(HidDeviceExtension);
	ASSERT(HidDeviceExtension->HidDescriptor.bNumDescriptors >= 1);
	ASSERT(HidDeviceExtension->HidDescriptor.DescriptorList[0].bReportType == HID_REPORT_DESCRIPTOR_TYPE);
	ASSERT(HidDeviceExtension->HidDescriptor.DescriptorList[0].wReportLength > 0);

	//
	// FIXME: support old hid version
	//
	BufferLength = HidDeviceExtension->HidDescriptor.DescriptorList[0].wReportLength;
	Status = Hid_GetDescriptor(DeviceObject,
		URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE,
		sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
		&Report,
		&BufferLength,
		HidDeviceExtension->HidDescriptor.DescriptorList[0].bReportType,
		0,
		HidDeviceExtension->InterfaceDesc->InterfaceNumber);
	if (!NT_SUCCESS(Status))
	{
		//
		// failed to get descriptor
		// try with old hid version
		//
		BufferLength = HidDeviceExtension->HidDescriptor.DescriptorList[0].wReportLength;
		Status = Hid_GetDescriptor(DeviceObject,
			URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT,
			sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
			&Report,
			&BufferLength,
			HidDeviceExtension->HidDescriptor.DescriptorList[0].bReportType,
			0,
			0 /* FIXME*/);
		if (!NT_SUCCESS(Status))
		{
			//DPRINT("[HIDUSB] failed to get report descriptor with %x\n", Status);
			return Status;
		}
	}

	//
	// get current stack location
	//
	IoStack = IoGetCurrentIrpStackLocation(Irp);
	USB_MON_DEBUG_INFO("[HIDUSB] GetReportDescriptor: Status %x ReportLength %lu OutputBufferLength %lu TransferredLength %lu\n", Status, HidDeviceExtension->HidDescriptor.DescriptorList[0].wReportLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength, BufferLength);

	//
	// get length to copy
	//
	Length = min(IoStack->Parameters.DeviceIoControl.OutputBufferLength, BufferLength);
	ASSERT(Length);

	//
	// copy result
	//
	RtlCopyMemory(Irp->UserBuffer, Report, Length);

	//
	// store result length
	//
	Irp->IoStatus.Information = Length;

	//
	// free the report buffer
	//
	ExFreePoolWithTag(Report, 'usbt');

	//
	// done
	//
	return Status;

}