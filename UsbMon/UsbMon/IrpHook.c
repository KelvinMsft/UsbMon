                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     #include "IrpHook.h"
#include "TList.h"
#include "CommonUtil.h"


///////////////////////////////////////////////////////////////////////////////////////////////
////	Types
////
////
typedef struct _IRPHOOK_LIST {
	TChainListHeader*  head; 
	ULONG		   RefCount;
}IRPHOOKLIST, *PIRPHOOKLIST;

typedef struct SEARCHPARAM
{
	PDRIVER_OBJECT driver_object;
	ULONG				 IrpCode;
}SEARCHPARAM, *PSEARCHPARAM;


///////////////////////////////////////////////////////////////////////////////////////////////
////	Marco
////
////


///////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////
////
IRPHOOKLIST* g_IrpHookList = NULL;


///////////////////////////////////////////////////////////////////////////////////////////////
////	Prototype
////
////



///////////////////////////////////////////////////////////////////////////////////////////////
////	Implementation
////
////
////
 
 
//--------------------------------------------------------------------------------------------//
ULONG SearchIrpHookObjectCallback(
	_In_ IRPHOOKOBJ* IrpObject,	
	_In_ void* Context
)
{
	SEARCHPARAM* param = (SEARCHPARAM*)Context;
	if (IrpObject->driver_object == param->driver_object && IrpObject->IrpCode == param->IrpCode)
	{	
		return CLIST_FINDCB_RET;
	}
	return CLIST_FINDCB_CTN;
}
//--------------------------------------------------------------------------------------------//
IRPHOOKOBJ* GetIrpHookObject(
	_In_ PDRIVER_OBJECT driver_object, 
	_In_ ULONG IrpCode)
{
	SEARCHPARAM param = { 0 };
	param.driver_object = driver_object;
	param.IrpCode = IrpCode;
	return QueryFromChainListByCallback(g_IrpHookList->head, SearchIrpHookObjectCallback, &param); 
} 

//--------------------------------------------------------------------------------------------//
ULONG RemoveIrpHookCallback(
	_In_ IRPHOOKOBJ* IrpObject,
	_In_ void* Context
)
{
	UNREFERENCED_PARAMETER(Context); 
 	IRPHOOKOBJ* hook_obj = IrpObject;
	if (hook_obj)
	{ 
		DoIrpHook(hook_obj->driver_object, hook_obj->IrpCode, hook_obj->oldFunction, Stop);
		USB_DEBUG_INFO_LN_EX("RemoveIrpHookCallback Once hook_obj->driver_object: %ws ", hook_obj->driver_object->DriverName.Buffer);
		hook_obj = NULL;  
	}  
	return CLIST_FINDCB_CTN | CLIST_FINDCB_DEL;
}

//--------------------------------------------------------------------------------------------//
NTSTATUS RemoveAllIrpObject()
{
	NTSTATUS    status = STATUS_UNSUCCESSFUL;  
	if (g_IrpHookList)
	{ 
		QueryFromChainListByCallback(g_IrpHookList->head, RemoveIrpHookCallback, NULL); 
		ExFreePool(g_IrpHookList);
		g_IrpHookList = NULL; 
		status = STATUS_SUCCESS;
	}

	return status;
}
 
//--------------------------------------------------------------------------------------------//
IRPHOOKOBJ* CreateIrpObject(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ ULONG IrpCode, 
	_In_ PVOID oldFunction, 
	_In_ PVOID newFunction)
{ 
	IRPHOOKOBJ* HookObj = (IRPHOOKOBJ*)ExAllocatePoolWithTag(NonPagedPool, sizeof(IRPHOOKOBJ), 'opri');

	if (!g_IrpHookList)
	{
		USB_DEBUG_INFO_LN_EX("Empty Hook Object \r\n");
		return NULL;
	}
	if (!HookObj)
	{
		USB_DEBUG_INFO_LN_EX("Empty Hook Object \r\n");
		return NULL; 
	}

	RtlZeroMemory(HookObj, sizeof(IRPHOOKOBJ));

	HookObj->driver_object = DriverObject;
	HookObj->IrpCode	   = IrpCode;
	HookObj->newFunction   = newFunction;
	HookObj->oldFunction   = oldFunction;
 
	if (!AddToChainListTail(g_IrpHookList->head, HookObj))
	{
		USB_DEBUG_INFO_LN_EX("Cannot Add to list ");
		ExFreePool(HookObj);
		HookObj = NULL;
	}

	USB_DEBUG_INFO_LN_EX("IRP List Size: %x with DriverName: %ws ", g_IrpHookList->head->Count, DriverObject->DriverName.Buffer);

	return HookObj;
}

//--------------------------------------------------------------------------------------------//
NTSTATUS AllocateIrpHookLinkedList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do {
		if (!g_IrpHookList)
		{
			g_IrpHookList = (IRPHOOKLIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(IRPHOOKLIST), 'opri');
			if (!g_IrpHookList)
			{
				status = STATUS_UNSUCCESSFUL;
				break; 
			}
			RtlZeroMemory(g_IrpHookList, sizeof(IRPHOOKLIST));
		}
		if (!g_IrpHookList->head)
		{
			g_IrpHookList->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, NULL, 0);
			if (!g_IrpHookList->head)
			{
				ExFreePool(g_IrpHookList);
				g_IrpHookList = NULL;
				status = STATUS_UNSUCCESSFUL;
				break;
			}
			g_IrpHookList->RefCount = 1;
		}
		status = STATUS_SUCCESS;
	} while (FALSE);
	return status;
} 

//--------------------------------------------------------------------------------------------//
PVOID DoIrpHook(
	_In_	  PDRIVER_OBJECT driver_object, 
	_In_	  ULONG IrpCode, 
	_In_	  PVOID NewFunction, 
	_In_  	  Action action
)
{
	PVOID old_irp_function = NULL;

	if (!driver_object || !NewFunction)
	{
		return old_irp_function;
	}

	if (!g_IrpHookList)
	{
		if (!NT_SUCCESS(AllocateIrpHookLinkedList()))
		{
			return old_irp_function;
		}
	}

	old_irp_function = (DRIVER_DISPATCH*)InterlockedExchange64(
		(LONG64 volatile *)&driver_object->MajorFunction[IrpCode],
		(LONG64)NewFunction
	);

	if (action == Start && old_irp_function)
	{
		//Create Object
		if (!CreateIrpObject(driver_object, IrpCode, old_irp_function, NewFunction))
		{
			//repair hook, if created fail.
			InterlockedExchange64((LONG64 volatile *)&driver_object->MajorFunction[IrpCode],(LONG64)old_irp_function);
			old_irp_function = NULL;
		}
	} 

	return old_irp_function;
} 
//--------------------------------------------------------------------------------------------//

