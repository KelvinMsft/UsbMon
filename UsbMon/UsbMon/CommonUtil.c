#include <fltKernel.h> 
#include "CommonUtil.h"



/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Marco
//// 



/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Global/Extern Variable
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
//// Prototype
//// 


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
//// 
//----------------------------------------------------------------------------------------//
NTSTATUS KeSleep(LONG msec)
{
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;
	return KeDelayExecutionThread(KernelMode, 0, &my_interval);
}

//----------------------------------------------------------------------------------------//
NTSTATUS GetDriverObjectByName(WCHAR* name, PDRIVER_OBJECT* pDriverObj)
{
	// use such API  
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	UNICODE_STRING DriverName = { 0 };

	if (name && pDriverObj)
	{
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
	}

	return status;
}

//----------------------------------------------------------------------------------------//
NTSTATUS GetDeviceName(PDEVICE_OBJECT device_object, WCHAR* DeviceNameBuf)
{
	UCHAR	 Buffer[sizeof(OBJECT_NAME_INFORMATION) + 512] = { 0 };
	POBJECT_NAME_INFORMATION NameInfo = (PVOID)Buffer;
	ULONG Length	= 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (DeviceNameBuf && device_object)
	{
		status = ObQueryNameString(device_object, NameInfo, sizeof(Buffer), &Length);

		if (NT_SUCCESS(status))
		{
			RtlMoveMemory(DeviceNameBuf, NameInfo->Name.Buffer, NameInfo->Name.Length);
		} 
	} 
	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS USBSymLinkToPath(PUNICODE_STRING pusSymLink, PUNICODE_STRING pusDosPath) 
{
	UNICODE_STRING usSymLinkPath;
	NTSTATUS       ntStatus;
	PFILE_OBJECT   pfVolume;
	PDEVICE_OBJECT pdVolume;

	// Check parameters
	if (!pusSymLink || !pusDosPath) return STATUS_INVALID_PARAMETER;

	// Create symbolic link path
	usSymLinkPath.Length = 0;
	usSymLinkPath.MaximumLength = pusSymLink->MaximumLength + 8;
	usSymLinkPath.Buffer = ExAllocatePool(NonPagedPool, usSymLinkPath.MaximumLength);
	RtlAppendUnicodeToString(&usSymLinkPath, L"\\??\\");
	RtlAppendUnicodeStringToString(&usSymLinkPath, pusSymLink);

	// Get file object and device object for volume
	ntStatus = IoGetDeviceObjectPointer(&usSymLinkPath, FILE_ALL_ACCESS, &pfVolume, &pdVolume);
	ExFreePool(usSymLinkPath.Buffer);
	if (!NT_SUCCESS(ntStatus)) return ntStatus;

	// Get DOS name for volume
	ntStatus = IoVolumeDeviceToDosName(pdVolume, pusDosPath);
	ObDereferenceObject(pfVolume);
	return ntStatus;
}