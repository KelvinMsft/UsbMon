#ifndef __IRP_HOOK_HEARER_
#define __IRP_HOOK_HEADER_

#include <fltKernel.h> 
#include "CommonUtil.h"
#include "Tlist.h" 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Types
////

typedef enum
{
	Start = 0,
	Stop,
}Action;

#pragma pack (8)
typedef struct PENDINGIRP
{
	PIRP						  Irp;			//Irp
	PIO_STACK_LOCATION		 IrpStack;			//Old IRP stack
	PVOID					oldContext;			//Old IRP routine argument
	IO_COMPLETION_ROUTINE*	oldRoutine;			//Old IRP routine address
}PENDINGIRP, *PPENDINGIRP;
#pragma pack()

#pragma pack (8)
typedef struct IRPHOOKOBJ
{
	PDRIVER_OBJECT driver_object;
	ULONG				 IrpCode;
	PDRIVER_DISPATCH oldFunction;
	PDRIVER_DISPATCH newFunction;
}IRPHOOKOBJ, *PIRPHOOKOBJ;
#pragma pack() 

#pragma pack (8) 
typedef struct _PENDINGIRP_LIST
{
	TChainListHeader*	head;
	ULONG			RefCount;
}PENDINGIRPLIST, *PPENDINGIRPLIST;
#pragma pack() 

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////	Prototype
////


//--------------------------------------------------------------------------------------
//	
//
NTSTATUS InsertPendingIrp(
	_In_ PENDINGIRPLIST* ListHeader,
	_In_ PENDINGIRP* PendingIrp
);


//--------------------------------------------------------------------------------------
//	
//
NTSTATUS RemovePendingIrp(
	_In_ PENDINGIRPLIST* Listheader,
	_In_ PENDINGIRP* PendingIrp
);


//--------------------------------------------------------------------------------------
//	
//
NTSTATUS InitIrpHookSystem();



//--------------------------------------------------------------------------------------
//	
//
NTSTATUS UnInitIrpHookSystem();




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
IRPHOOKOBJ*	CreateIrpHookObject(
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
PENDINGIRP* GetRealPendingIrpByIrp(
	_In_	PENDINGIRPLIST* ListHeader,
	_In_ PIRP irp
);

//--------------------------------------------------------------------------------------
//	
// 
NTSTATUS AllocatePendingIrpLinkedList(
	_Inout_ 	PENDINGIRPLIST**  ListHeader
);

//--------------------------------------------------------------------------------------
//	
//  
NTSTATUS FreePendingList(
	_In_ PENDINGIRPLIST* PendingIrpList
);


#endif