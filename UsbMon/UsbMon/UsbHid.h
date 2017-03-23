#ifndef __USB_HID_HEADER__
#define __USB_HID_HEADER__ 

#include <fltKernel.h> 
#include "CommonUtil.h"
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
		UnInitialize Hid Sub System, Simply free the list

Arguments:
 		 No 

Return Value:	
		NTSTATUS		 - STATUS_SUCCESS      , if it is Freed
						 - STATUS_UNSUCCESSFUL , if is can't be freed
-----------------------------------------------------------------------------------*/
NTSTATUS UnInitHidSubSystem();

 
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		Initialize Hid Sub System, Simply free the list
			1. Allocate Client Pdo List memory
			2. Init Client Pdo List by getting all of client PDO
	 
Arguments:
		 ULONG			 - Size of List 
		 
Return Value:	
		NTSTATUS		 - true  , if it initial succesfully
						 - false , if it initial faile
-----------------------------------------------------------------------------------*/
NTSTATUS InitHidSubSystem(
	_Out_ PULONG		ClientPdoListSize
);
 


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		1. To verify a Device Object, determine is it that hid type we need to catch up 
		2. If verify success, Insert Node into a list
	 
Arguments:
		PDEVICE_OBJECT	 - Pointer of Deivce object that is going to be identified
		 
Return Value:	
		NTSTATUS		 - STATUS_SUCCESS	   , if it is hid device and Insert succesfully
						 - STATUS_UNSUCCESSFUL , if it is hid device and unsuccessfully
-----------------------------------------------------------------------------------*/
NTSTATUS VerifyAndInsertIntoHidList(
	_In_ PDEVICE_OBJECT DeviceObject
);


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:
		1. Remove a node for the device object in the internal list
	 
Arguments:
		PDEVICE_OBJECT	 - Pointer of Deivce object that is going to be removed

		 
Return Value:	
		NTSTATUS		 - STATUS_SUCCESS	   , if it Remove succesfully
						 - STATUS_UNSUCCESSFUL , if it Remove unsuccessfully
-----------------------------------------------------------------------------------*/
NTSTATUS RemoveNodeFromHidList(
	_In_ PDEVICE_OBJECT DeviceObject
); 
 
#endif