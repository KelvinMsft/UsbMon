#ifndef __IRP_HOOK_HEARER_
#define __IRP_HOOK_HEADER_

#include <fltKernel.h>
#include "LinkedList.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Types
////

typedef enum
{
	Start = 0,
	Stop,
}Action;


#pragma pack (8)
typedef struct IRPHOOKOBJ
{
	RT_LIST_ENTRY   entry;  
	PDRIVER_OBJECT driver_object;
	ULONG IrpCode;
	PVOID oldFunction;	
	PVOID newFunction;
}IRPHOOKOBJ, *PIRPHOOKOBJ;
#pragma pack() 

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Implementation
////

//--------------------------------------------------------------------------------------
//	
//
PVOID	DoIrpHook(
	_In_		PDRIVER_OBJECT	driver_object, 
	_In_		ULONG			IrpCode, 
	_In_		PVOID			NewFunction, 
	_In_opt_	IRPHOOKOBJ*		hook_obj, 
	_In_		Action			action
);
//--------------------------------------------------------------------------------------
//	
//
IRPHOOKOBJ*	CreateIrpObject(
	_In_	PDRIVER_OBJECT driver_object,
	_In_	ULONG		   IrpCode,
	_In_	PVOID		   oldFunction,
	_In_	PVOID		   newFunction
);
//--------------------------------------------------------------------------------------
//	
//
IRPHOOKOBJ* GetIrpHookObject(
	_In_ PDRIVER_OBJECT driver_object,
	_In_ ULONG IrpCode
);
//--------------------------------------------------------------------------------------
//	
//
NTSTATUS	RemoveIrpObject(
	_In_ IRPHOOKOBJ* IrpObject
);
//--------------------------------------------------------------------------------------
//	
//
NTSTATUS	RemoveAllIrpObject();
//--------------------------------------------------------------------------------------
//	
//
NTSTATUS	InsertIrpObject(
	_In_ IRPHOOKOBJ* HookObj
); 
//--------------------------------------------------------------------------------------
//	
//
NTSTATUS	InitIrpHook();

#endif