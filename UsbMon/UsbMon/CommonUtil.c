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