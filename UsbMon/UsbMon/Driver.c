
#include <fltKernel.h> 
#include "HidHijack.h"

/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Types
////  

///////////////////////////////////////////////////////////////////////////////////////////////
////	Marco Utilities
////  
////  

///////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////
//// 
 
/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Prototype
//// 
////

/////////////////////////////////////////////////////////////////////////////////////////////// 
////	Implementation
//// 
////

//-----------------------------------------------------------------------------------------
VOID DriverUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
)
{ 
	UNREFERENCED_PARAMETER(DriverObject);
	UnInitializeHidPenetrate();
	return;
} 

//----------------------------------------------------------------------------------------//
NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT object, 
	_In_ PUNICODE_STRING String)
{ 
	NTSTATUS					  status = STATUS_UNSUCCESSFUL; 
	object->DriverUnload			     = DriverUnload;
	status = InitializeHidPenetrate(USB3);
	return status;
} 
//----------------------------------------------------------------------------------------//