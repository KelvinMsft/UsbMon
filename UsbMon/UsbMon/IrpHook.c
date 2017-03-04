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
IRPHOOKLIST* ListHeader = NULL;



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
	return QueryFromChainListByCallback(ListHeader->head, SearchIrpHookObjectCallback, &param); 
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
		STACK_TRACE_DEBUG_INFO("RemoveIrpHookCallback Once \r\n");
		hook_obj = NULL;  
	}  
	return CLIST_FINDCB_CTN | CLIST_FINDCB_DEL;
}

//--------------------------------------------------------------------------------------------//
NTSTATUS RemoveAllIrpObject()
{
	NTSTATUS    status = STATUS_UNSUCCESSFUL;  
	if (ListHeader)
	{ 
		QueryFromChainListByCallback(ListHeader->head, RemoveIrpHookCallback, NULL); 
		ExFreePool(ListHeader);
		ListHeader = NULL; 
		status = STATUS_SUCCESS;
	}

	return status;
}
 
//--------------------------------------------------------------------------------------------//
IRPHOOKOBJ* CreateIrpObject(
	_In_ PDRIVER_OBJECT driver_object, 
	_In_ ULONG IrpCode, 
	_In_ PVOID oldFunction, 
	_In_ PVOID newFunction)
{ 
	IRPHOOKOBJ* HookObj = (IRPHOOKOBJ*)ExAllocatePoolWithTag(NonPagedPool, sizeof(IRPHOOKOBJ), 'opri');

	if (!HookObj)
	{
		return NULL; 
	}

	RtlZeroMemory(HookObj, sizeof(IRPHOOKOBJ));

	HookObj->driver_object = driver_object;
	HookObj->IrpCode = IrpCode;
	HookObj->newFunction = newFunction;
	HookObj->oldFunction = oldFunction;
 
	if (!AddToChainListTail(ListHeader->head, HookObj))
	{
		ExFreePool(HookObj);
		HookObj = NULL;
	}
	return HookObj;
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

	if (!ListHeader)
	{
		if (!NT_SUCCESS(InitIrpHookLinkedList()))
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
//ÃèÊö:
// 1. Create Header
// 2. Create List
//
NTSTATUS InitIrpHookLinkedList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (!ListHeader)
	{
		ListHeader = (IRPHOOKLIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(IRPHOOKLIST), 'opri');  
	}

	if (!ListHeader)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	RtlZeroMemory(ListHeader, sizeof(IRPHOOKLIST));
	ListHeader->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, NULL, 0);
	 
	if (!ListHeader->head)
	{
		ExFreePool(ListHeader);
		ListHeader = NULL;			
		status = STATUS_UNSUCCESSFUL;
		return status;
	} 	 
	ListHeader->RefCount = 1;
	status = STATUS_SUCCESS;
	return status;
}
//--------------------------------------------------------------------------------------------//
