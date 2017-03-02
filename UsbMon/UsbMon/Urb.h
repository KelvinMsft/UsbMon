#include <fltKernel.h>
#include "CommonUtil.h"
#include "UsbType.h"

NTSTATUS 
HumCallUSB(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PURB Urb
);

NTSTATUS
Hid_GetDescriptor(
	IN PDEVICE_OBJECT DeviceObject,
	IN USHORT UrbFunction,
	IN USHORT UrbLength,
	IN OUT PVOID *UrbBuffer,
	IN OUT PULONG UrbBufferLength,
	IN UCHAR DescriptorType,
	IN UCHAR Index,
	IN USHORT LanguageIndex
);

NTSTATUS
NTAPI
HidUsb_GetReportDescriptor(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
);

#pragma once
