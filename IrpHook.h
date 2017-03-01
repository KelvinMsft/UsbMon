#include <fltKernel.h>
#include "LinkedList.h"

typedef enum
{
	Start = 0,
	Stop,
}Action;


#pragma pack (8)
typedef struct IRPHOOKOBJ
{
	RT_LIST_ENTRY   entry;
	KSPIN_LOCK		lock;
	PDRIVER_OBJECT driver_object;
	ULONG IrpCode;
	PVOID oldFunction;	
	PVOID newFunction;
}IRPHOOKOBJ, *PIRPHOOKOBJ;
#pragma pack() 

PVOID		DoIrpHook(PDRIVER_OBJECT driver_object, ULONG IrpCode, PVOID ProxyFunction,Action action); 
IRPHOOKOBJ*	CreateIrpObject(PDRIVER_OBJECT driver_object, ULONG IrpCode, PVOID oldFunction, PVOID newFunction);
IRPHOOKOBJ* GetIrpHookObject(PDRIVER_OBJECT driver_object, ULONG IrpCode);
NTSTATUS	RemoveIrpObject(IRPHOOKOBJ* IrpObject);
NTSTATUS	RemoveAllIrpObject();
NTSTATUS	InsertIrpObject(IRPHOOKOBJ* HookObj); 
NTSTATUS	InitIrpHook();