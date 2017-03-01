                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     #include "IrpHook.h"
#include "LinkedList.h"
/*
#define HOOKOBJ_COUNT 200
IRPHOOKOBJ* HookObjList[HOOKOBJ_COUNT] = { 0 };
ULONG					  CurrentIndex = 0;
*/ 
typedef struct _IRPHOOK_LIST {
	LIST_ENTRY  head;
	KSPIN_LOCK	lock;
}IRPHOOKLIST, *PIRPHOOKLIST;

IRPHOOKLIST* ListHeader = NULL;
//----------------------------------------------------------------------------------------//.
IRPHOOKOBJ* GetIrpHookObject(PDRIVER_OBJECT driver_object, ULONG IrpCode)
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
NTSTATUS RemoveIrpObject(IRPHOOKOBJ* IrpObject)
{
	NTSTATUS    status = STATUS_UNSUCCESSFUL;
	IRPHOOKOBJ* hook_obj = IrpObject;
	if (hook_obj)
	{
		KIRQL irql = 0;
		ExAcquireSpinLock(&ListHeader->lock, &irql);
		DoIrpHook(hook_obj->driver_object, hook_obj->IrpCode, hook_obj->oldFunction, Stop);
		RTRemoveEntryList(&hook_obj->entry);
		ExReleaseSpinLock(&ListHeader->lock, irql);

		ExFreePool(hook_obj);
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
	return status;
}

//----------------------------------------------------------------------------------------//.
NTSTATUS InsertIrpObject(IRPHOOKOBJ* HookObj)
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
IRPHOOKOBJ* CreateIrpObject(PDRIVER_OBJECT driver_object, ULONG IrpCode, PVOID oldFunction, PVOID newFunction)
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
PVOID DoIrpHook(PDRIVER_OBJECT driver_object, ULONG IrpCode, PVOID NewFunction, Action action)
{
 
	PVOID old_irp_function = NULL;

	if (!driver_object || !NewFunction)
	{
		return old_irp_function;
	}

	if (!ListHeader)
	{
		InitIrpHook();
	}

	old_irp_function = (DRIVER_DISPATCH*)InterlockedExchange64(
		(LONG64 volatile *)&driver_object->MajorFunction[IrpCode],
		(LONG64)NewFunction
	);

	if (action == Start && old_irp_function)
	{
		if (!CreateIrpObject(driver_object, IrpCode, old_irp_function, NewFunction))
		{
			InterlockedExchange64((LONG64 volatile *)&driver_object->MajorFunction[IrpCode],(LONG64)old_irp_function);
		}
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
			status = STATUS_SUCCESS;
		} 
	}
	return status;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            