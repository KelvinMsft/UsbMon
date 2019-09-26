#include <ntddk.h> 
#include "CommonUtil.h"
#include "IrpHook.h" 

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Marco
//// 



/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Global/Extern Variable
//// 
extern ULONG g_iClientUUID;
extern ULONG g_iGameID;

extern ULONG64			  g_IsEnableModuleChecksum;  
extern ULONG 			  g_EncryptKey[4];

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

NTSTATUS ObQueryNameString(
	PVOID                    Object,
	POBJECT_NAME_INFORMATION ObjectNameInfo,
	ULONG                    Length,
	PULONG                   ReturnLength
);

ULONG g_MajorVersion = 0 ;
ULONG g_MinorVersion = 0 ;
ULONG g_BuildNumber  = 0 ; 

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 
PVOID NTAPI RtlPcToFileHeader(_In_ PVOID PcValue, _Out_ PVOID *BaseOfImage);

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
////   
// A wrapper of RtlPcToFileHeader
PVOID UtilPcToFileHeader(PVOID pc_value) 
{
  void *base = NULL;
  return RtlPcToFileHeader(pc_value,&base); 
}
 


//----------------------------------------------------------------------------------------//
ULONG GetNextFdoExtOffset()
{
	ULONG Offset = 0;
	PsGetVersion(&g_MajorVersion, &g_MinorVersion, &g_BuildNumber, NULL);

	if(g_BuildNumber == 6000)
	{
		Offset = 0x1D0;
	}
	else if(g_BuildNumber==7600 || g_BuildNumber==7601)//windows7
	{
		Offset = 0x1E0; 
	}
	else if(g_BuildNumber==9200)//windows8
	{
		Offset = 0x1E0; 
	}
	else if(g_BuildNumber==9600)//windows8.1
	{
		Offset = 0x1A8; 
	}
	else if(g_BuildNumber > 10000)
	{
		Offset = 0x1A8;
	} 
	
	return Offset ; 
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
NTSTATUS ReleaseDriverObject(PDRIVER_OBJECT pDriverObj)
{
	// use such API  
	NTSTATUS status = STATUS_SUCCESS;
	if(!pDriverObj)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}
	
	ObDereferenceObject(pDriverObj);
	pDriverObj = NULL; 
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
	usSymLinkPath.Buffer = ExAllocatePoolWithTag(NonPagedPool, usSymLinkPath.MaximumLength, 'symp');
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
