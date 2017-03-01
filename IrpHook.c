#include "IrpHook.h"
#include "LinkedList.h"
/*
#define HOOKOBJ_COUNT 200
IRPHOOKOBJ* HookObjList[HOOKOBJ_COUNT] = { 0 };
ULONG					  CurrentIndex = 0;
*/ 
IRPHOOKOBJ* head = NULL;

//----------------------------------------------------------------------------------------//.
IRPHOOKOBJ* GetIrpHookObject(PDRIVER_OBJECT driver_object, ULONG IrpCode)
{
	IRPHOOKOBJ						*pDev    = NULL;
	RT_LIST_ENTRY				*pEntry, *pn = NULL;
	RtEntryListForEachSafe(&head->entry, pEntry, pn)
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
NTSTATUS RemoveAllIrpObject()
{
	NTSTATUS    status					     = STATUS_UNSUCCESSFUL; 
	IRPHOOKOBJ						*pDev    = NULL;
	RT_LIST_ENTRY				*pEntry, *pn = NULL;

	RtEntryListForEachSafe(&head->entry, pEntry, pn)
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
NTSTATUS RemoveIrpObject(IRPHOOKOBJ* IrpObject)
{
	NTSTATUS    status = STATUS_UNSUCCESSFUL;
	IRPHOOKOBJ* hook_obj = IrpObject;
	if (hook_obj)
	{
		KIRQL irql = 0;
		ExAcquireSpinLock(&hook_obj->lock, &irql);
		DoIrpHook(hook_obj->driver_object, hook_obj->IrpCode, hook_obj->oldFunction, Stop);
		RTRemoveEntryList(&hook_obj->entry);
		ExFreePool(hook_obj);
		ExReleaseSpinLock(&hook_obj->lock, irql);
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

	ExAcquireSpinLock(&HookObj->lock, &irql);
	RTInsertTailList(&head->entry, &HookObj->entry);
	ExReleaseSpinLock(&HookObj->lock, irql);

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

	if (!head)
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
	if (!head)
	{
		head = (IRPHOOKOBJ*)ExAllocatePoolWithTag(NonPagedPool, sizeof(IRPHOOKOBJ), 'opri');
		if (head)
		{
			RtlZeroMemory(head, sizeof(IRPHOOKOBJ)); 
			RTInitializeListHead(&head->entry);
			status = STATUS_SUCCESS;
		} 
	}
	return status;
}
