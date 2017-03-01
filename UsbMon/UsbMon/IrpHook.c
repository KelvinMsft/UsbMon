                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     #include "IrpHook.h"
#include "LinkedList.h"



///////////////////////////////////////////////////////////////////////////////////////////////
////	Types
////
////
typedef struct _IRPHOOK_LIST {
	LIST_ENTRY  head;
	KSPIN_LOCK	lock;
}IRPHOOKLIST, *PIRPHOOKLIST;



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

//----------------------------------------------------------------------------------------//.
IRPHOOKOBJ* GetIrpHookObject(
	_In_ PDRIVER_OBJECT driver_object, 
	_In_ ULONG IrpCode)
{
	IRPHOOKOBJ						*pDev    = NULL;
	RT_LIST_ENTRY				*pEntry, *pn = NULL;
	RtEntryListForEachSafe(&ListHeader->head, pEntry, pn)
	{
		pDev = (IRPHOOKOBJ *)pEntry;
		if (pDev)
		{
			if (pDev->driver_object == driver_object &&	pDev->IrpCode == IrpCode)
			{
				break;
			}
		}
	}
	return pDev;
}



//----------------------------------------------------------------------------------------//.
NTSTATUS RemoveIrpObject(
	_In_ IRPHOOKOBJ* IrpObject)
{
	NTSTATUS    status = STATUS_UNSUCCESSFUL;
	IRPHOOKOBJ* hook_obj = IrpObject;
	if (hook_obj)
	{
		KIRQL irql = 0;
	
		DoIrpHook(hook_obj->driver_object, hook_obj->IrpCode, hook_obj->oldFunction, hook_obj , Stop);
		hook_obj = NULL; 
		status = STATUS_SUCCESS;
	}
	return status;
}

//----------------------------------------------------------------------------------------//.
NTSTATUS RemoveAllIrpObject()
{
	NTSTATUS    status					     = STATUS_UNSUCCESSFUL; 
	IRPHOOKOBJ						*pDev    = NULL;
	RT_LIST_ENTRY				*pEntry, *pn = NULL;

	RtEntryListForEachSafe(&ListHeader->head, pEntry, pn)
	{
		pDev = (IRPHOOKOBJ *)pEntry;
		if (pDev)
		{
			RemoveIrpObject(pDev);
		}
		status = STATUS_SUCCESS;
	}

	if (ListHeader)
	{
		ExFreePool(ListHeader);
		ListHeader = NULL;
	}
	return status;
}

//----------------------------------------------------------------------------------------//.
NTSTATUS InsertIrpObject(
	_In_ IRPHOOKOBJ* HookObj)
{	
	NTSTATUS status = STATUS_UNSUCCESSFUL; 
	KIRQL	   irql = 0;
	if (HookObj)
	{
		status = STATUS_SUCCESS;
	} 

	ExAcquireSpinLock(&ListHeader->lock, &irql);
	RTInsertTailList(&ListHeader->head, &HookObj->entry);
	ExReleaseSpinLock(&ListHeader->lock, irql);

	return status;
}


//----------------------------------------------------------------------------------------//.
IRPHOOKOBJ* CreateIrpObject(
	_In_ PDRIVER_OBJECT driver_object, 
	_In_ ULONG IrpCode, 
	_In_ PVOID oldFunction, 
	_In_ PVOID newFunction
)
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
	KeInitializeSpinLock(&ListHeader->lock);
	InsertIrpObject(HookObj);

	return HookObj;
}


//----------------------------------------------------------------------------------------//
VOID DestoryIrpObject(
	_In_ IRPHOOKOBJ* hook_obj)
{
	if (hook_obj)
	{
		KIRQL irql = 0;
		ExAcquireSpinLock(&ListHeader->lock, &irql);
		RTRemoveEntryList(&hook_obj->entry);
		ExReleaseSpinLock(&ListHeader->lock, irql);
		ExFreePool(hook_obj);
		hook_obj = NULL;
	}
	return ;
}


//----------------------------------------------------------------------------------------//
PVOID DoIrpHook(
	_In_	  PDRIVER_OBJECT driver_object, 
	_In_	  ULONG IrpCode, 
	_In_	  PVOID NewFunction, 
	_In_opt_  IRPHOOKOBJ* hook_obj, 
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
		if (!NT_SUCCESS(InitIrpHook()))
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
		}
	}
	else if (action == Stop && hook_obj)
	{
		DestoryIrpObject(hook_obj);
	}
	return old_irp_function;
}

//----------------------------------------------------------------------------------------//
NTSTATUS InitIrpHook()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!ListHeader)
	{
		ListHeader = (IRPHOOKLIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(IRPHOOKLIST), 'opri');
	 
		if (ListHeader)
		{
			RtlZeroMemory(ListHeader, sizeof(IRPHOOKLIST));
			RTInitializeListHead(&ListHeader->head);
			KeInitializeSpinLock(&ListHeader->lock);
			status = STATUS_SUCCESS;
		} 
	}
	return status;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            