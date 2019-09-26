#pragma once

#include <ntddk.h> 
#include "CommonUtil.h"
#include "TList.h"
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
	IO_COMPLETION_ROUTINE*	newRoutine;			//new IRP routine address
	PVOID					newContext;			//new IRP routine argument
}PENDINGIRP, *PPENDINGIRP;
#pragma pack()
 
#pragma pack (8)
typedef struct IRPHOOKOBJ
{  
	PDRIVER_OBJECT driver_object;
	ULONG				 IrpCode;
	PDRIVER_DISPATCH oldFunction;
	PDRIVER_DISPATCH newFunction;
	BOOLEAN 		 IsAttacked;
	BOOLEAN 		 IsAttacked2;
	ULONG64			ObjCheckSum;
}IRPHOOKOBJ, *PIRPHOOKOBJ;
#pragma pack() 

#pragma pack (8) 
 typedef struct _PENDINGIRP_LIST
{
	TChainListHeader*	head;
	ULONG				RefCount;
	KTIMER				m_Timer;
	KDPC				m_DPCP ;
	LARGE_INTEGER		m_Timeout ;
	BOOLEAN         	bIsStartedCheckSum;	
	BOOLEAN				bIsAttackReported;
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
NTSTATUS InitIrpHookSystem(BOOLEAN IsChecksum);



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
NTSTATUS IrpVerifyPendingIrpCompletionHookByIrp(	
	_In_	PENDINGIRPLIST* ListHeader,
	_In_	PIRP Irp
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

//--------------------------------------------------------------------------------------
//	
//  
VOID StartTimeBoom(void* params);
