#ifndef __USB_HID_HEADER__
#define __USB_HID_HEADER__ 

#include <fltKernel.h> 
#include "local.h"


#define HID_USB_DEVICE				 	 L"\\Driver\\hidusb"


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		Determine is it HID device pipe

Arguments:
		TChainListHeader - list header of HID device pipe
		handle			 - handle of HID device pipe 
		node			 - output node which is found
		 
Return Value:	
		BOOLEAN			 - true  , if it handle in list
						 - false , Depended on old completion
-----------------------------------------------------------------------------------*/
BOOLEAN  IsHidDevicePipe(
	_In_ TChainListHeader* PipeListHeader,
	_In_ USBD_PIPE_HANDLE  PipeHandle,
	_Out_ PHID_DEVICE_NODE*  node
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		Free Client Pdo List, and each node

Arguments:
		TChainListHeader - list of HID device pipe
		handle			 - handle of HID device pipe 
		node			 - output node
		 
Return Value:	
		NTSTATUS		 - STATUS_SUCCESS      , if it is Freed
						 - STATUS_UNSUCCESSFUL , if is can't be freed
-----------------------------------------------------------------------------------*/
NTSTATUS UnInitHidSubSystem();

 
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		1. Allocate Client Pdo List needed memory
		2. Init Client Pdo List by getting all of client PDO
	 
Arguments:
		TChainListHeader - list of HID device pipe
		handle			 - handle of HID device pipe 
		node			 - output node
		 
Return Value:	
		NTSTATUS		 - true  , if it initial succesfully
						 - false , if it initial faile
-----------------------------------------------------------------------------------*/
NTSTATUS InitHidSubSystem(
	_Out_ PULONG		ClientPdoListSize
);
 
NTSTATUS VerifyAndInsertIntoHidList(
	_In_ PDEVICE_OBJECT DeviceObject
);

NTSTATUS RemoveNodeFromHidList(
	_In_ PDEVICE_OBJECT DeviceObject
); 
 
#endif