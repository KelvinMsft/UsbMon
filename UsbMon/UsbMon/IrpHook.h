#ifndef __IRP_HOOK_HEARER_
#define __IRP_HOOK_HEADER_

#include <fltKernel.h> 

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
	PDRIVER_OBJECT driver_object;
	ULONG				 IrpCode;
	PDRIVER_DISPATCH oldFunction;
	PDRIVER_DISPATCH newFunction;
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
NTSTATUS	RemoveAllIrpObject();

//--------------------------------------------------------------------------------------
//	
//
NTSTATUS	InitIrpHookLinkedList();

#endif