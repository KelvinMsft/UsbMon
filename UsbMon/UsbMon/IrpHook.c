
#include "IrpHook.h" 

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

typedef struct _PENDINGIRP_LIST
{
	TChainListHeader*	head;
	ULONG			RefCount;
}PENDINGIRPLIST, *PPENDINGIRPLIST;

///////////////////////////////////////////////////////////////////////////////////////////////
////	Marco
////
////

#define GetNodeByPTR(ListHead,NodeValue) QueryFromChainListByULONGPTR(ListHead, (ULONG_PTR)NodeValue)

///////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////
////
IRPHOOKLIST*   	  g_IrpHookList = NULL;
PENDINGIRPLIST*	  g_PendingIrpList = NULL;
BOOLEAN			  g_IsInit = FALSE;


///////////////////////////////////////////////////////////////////////////////////////////////
////	Prototype
////
////


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

Lookup Pending IRP callback, in callback, we
recovered the completion hook

Arguments:

pending_irp_node - Each node inside IRP pending list

context			 - Not used

Return Value:

LOOKUP_STATUS	 - CLIST_FINDCB_CTN, traverse whole
list
-------------------------------------------------------*/
LOOKUP_STATUS	LookupPendingIrpCallback(
	_In_ PENDINGIRP* pending_irp_node,
	_In_ PVOID		 context
);

///////////////////////////////////////////////////////////////////////////////////////////////
////	Implementation
////
////
////
//----------------------------------------------------------------------------------------//
PENDINGIRPLIST* GetPendingList()
{
	return g_PendingIrpList;
}


//----------------------------------------------------------------------------------------//
PENDINGIRP* GetRealPendingIrpByIrp(PIRP irp)
{
	if (!irp || !g_PendingIrpList)
	{
		return NULL;
	}
	return GetNodeByPTR(g_PendingIrpList->head, irp);
}


//----------------------------------------------------------------------------------------//
NTSTATUS InsertPendingIrp(
	_In_ PENDINGIRP* PendingIrp
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!PendingIrp || !g_PendingIrpList)
	{
		return status;
	}
	if (AddToChainListTail(g_PendingIrpList->head, PendingIrp))
	{
		status = STATUS_SUCCESS;
	}
	return status;
}

//----------------------------------------------------------------------------------------//
NTSTATUS RemovePendingIrp(
	_In_ PENDINGIRP* PendingIrp
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!PendingIrp || !g_PendingIrpList)
	{
		return status;
	}
	if (DelFromChainListByPointer(g_PendingIrpList->head, PendingIrp))
	{
		status = STATUS_SUCCESS;
	}
	return status;
}

//----------------------------------------------------------------------------------------//
NTSTATUS AllocatePendingIrpLinkedList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do
	{
		if (!g_PendingIrpList)
		{
			g_PendingIrpList = (PENDINGIRPLIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(PENDINGIRPLIST), 'kcaj');
			if (!g_PendingIrpList)
			{
				status = STATUS_UNSUCCESSFUL;
				break;
			}
			RtlZeroMemory(g_PendingIrpList, sizeof(PENDINGIRPLIST));
		}

		if (!g_PendingIrpList->head)
		{
			g_PendingIrpList->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, NULL, 0);
			if (!g_PendingIrpList->head)
			{
				ExFreePool(g_PendingIrpList);
				g_PendingIrpList = NULL;
				status = STATUS_UNSUCCESSFUL;
				break;
			}
			g_PendingIrpList->RefCount = 1;
		}

		status = STATUS_SUCCESS;
	} while (FALSE);
	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS FreePendingIrpList()
{
	if (g_PendingIrpList)
	{
		if (g_PendingIrpList->head)
		{
			CHAINLIST_SAFE_FREE(g_PendingIrpList->head);
		}
		ExFreePool(g_PendingIrpList);
		g_PendingIrpList = NULL;
	}
}

//-------------------------------------------------------------------------------------------//
LOOKUP_STATUS LookupPendingIrpCallback(
	_In_ PENDINGIRP* pending_irp_node,
	_In_ void*		 context
)
{


	NTSTATUS    status = STATUS_UNSUCCESSFUL;
	PENDINGIRP* pending_irp = pending_irp_node;
	if (pending_irp)
	{
		InterlockedExchange64(&pending_irp->IrpStack->Context, pending_irp->oldContext);
		InterlockedExchange64(&pending_irp->IrpStack->CompletionRoutine, pending_irp->oldRoutine);
		USB_DEBUG_INFO_LN_EX("RemovePendingIrpCallback Once %I64x", pending_irp_node);
	}
	return CLIST_FINDCB_CTN;
}

//----------------------------------------------------------------------------------------- 
NTSTATUS RecoverAllCompletionHook()
{
	NTSTATUS						   status = STATUS_UNSUCCESSFUL;
	if (QueryFromChainListByCallback(g_PendingIrpList->head, LookupPendingIrpCallback, NULL))
	{
		status = STATUS_SUCCESS;
	}
	return status;
}

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
NTSTATUS RecoverAllIrpHook()
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
IRPHOOKOBJ* CreateIrpHookObject(
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
	HookObj->IrpCode = IrpCode;
	HookObj->newFunction = newFunction;
	HookObj->oldFunction = oldFunction;

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
	_In_	  PDRIVER_OBJECT DriverObject,
	_In_	  ULONG IrpCode,
	_In_	  PVOID NewFunction,
	_In_  	  Action action
)
{
	PVOID old_irp_function = NULL;
	if (!DriverObject || !NewFunction || !g_IrpHookList || !g_PendingIrpList)
	{
		return old_irp_function;
	}

	old_irp_function = (DRIVER_DISPATCH*)InterlockedExchange64(
		(LONG64 volatile *)&DriverObject->MajorFunction[IrpCode],
		(LONG64)NewFunction
	);

	if (action == Start && old_irp_function)
	{
		//Create Object
		if (!CreateIrpHookObject(DriverObject, IrpCode, old_irp_function, NewFunction))
		{
			//repair hook, if created fail.
			InterlockedExchange64((LONG64 volatile *)&DriverObject->MajorFunction[IrpCode], (LONG64)old_irp_function);
			old_irp_function = NULL;
		}
	}

	return old_irp_function;
}
//--------------------------------------------------------------------------------------------//
NTSTATUS InitIrpHookSystem()
{
	if (!NT_SUCCESS(AllocatePendingIrpLinkedList()))
	{
		USB_DEBUG_INFO_LN_EX("AllocatePendingIrpLinkedList Error");
		return STATUS_UNSUCCESSFUL;
	}
	if (!NT_SUCCESS(AllocateIrpHookLinkedList()))
	{
		return STATUS_UNSUCCESSFUL;
	}
	g_IsInit = TRUE;
	return STATUS_SUCCESS;
}


//--------------------------------------------------------------------------------------------//
NTSTATUS UnInitIrpHookSystem()
{
	if (!NT_SUCCESS(RecoverAllIrpHook()))
	{
		USB_DEBUG_INFO_LN_EX("AllocatePendingIrpLinkedList Error");
		return STATUS_UNSUCCESSFUL;
	}

	//1. Recovery
	RecoverAllCompletionHook();

	//2. Free Memory
	if (!NT_SUCCESS(FreePendingIrpList()))
	{
		return STATUS_UNSUCCESSFUL;
	}

	g_IsInit = FALSE;

	return STATUS_SUCCESS;
}
