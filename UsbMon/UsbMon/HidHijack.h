#ifndef __HIJACK_HEADER__
#define __HIJACK_HEADER__ 


#include <ntddk.h> 
#include "UsbUtil.h"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

	- Init Usb Penetrate System
		1.	 Create and Initial Hid Client Pdo List
		2.   Init Pending IRP list
		3.	 Init Active IRP Linked List
		4.	 

Arguments:

	 UsbVersion - USB hub version

Return Value:

	 NTSTATUS	- STATUS_SUCCES if initial successed
				- STATUS_UNSUCCESSFUL if failed

-------------------------------------------------------*/
NTSTATUS InitializeHidPenetrate(
	_In_ ULONG RequiredDevice
);


  
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

	- UnInit Usb Penetrate System
		1.	 Set Unloading Flag
		2.   Remove All IRP Hook
		3.	 Remove All Pending IRP (recover Completion hook) 
		
Arguments:

	 No
	
Return Value:

	  NTSTATUS	- STATUS_SUCCESS if initial successed
				- STATUS_UNSUCCESSFIL if initial failed

	-------------------------------------------------------*/
NTSTATUS UnInitializeHidPenetrate();



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

	- MapUsbDataToUserAddressSpace
		1.	Mapped the mouse / keyboard data struct to 
			user mode address space.
			
		2.  Once structure is updated, Set Event object 
		
Arguments:

	  UserModeConfigEx - Pointer to the User Mode Config struct with 
						 mapped address
	
	  Size			   - size of struct
	  
Return Value:	

	  NTSTATUS	- STATUS_SUCCESS if initial successed
				- STATUS_UNSUCCESSFIL if initial failed

	-------------------------------------------------------*/
NTSTATUS MapUsbDataToUserAddressSpace( 
	_Inout_  	void*     UserModeConfigEx,
	_In_		ULONG			  Size
);


/*-----------------------------------------------------
Routine Description:
	-	OnProcessExitToUsbSystem
		
Arguments:

	PEPROCESS   - Pointer of exiting's process
		
Return Value:

	  NTSTATUS	- STATUS_SUCCESS if initial successed
				- STATUS_UNSUCCESSFIL if initial failed
-----------------------------------------------------*/ 
NTSTATUS OnProcessExitToUsbSystem(
	_In_ PEPROCESS eProcess
);
	
	
	

/*-----------------------------------------------------
Routine Description:

	- UsbPenerateConfigInit
			1. Config Init
Arguments:
		
	No 
	
Return Value:

	  NTSTATUS	- STATUS_SUCCESS if initial successed
				- STATUS_UNSUCCESSFIL if initial failed
-----------------------------------------------------*/
NTSTATUS UsbPenerateConfigInit();


NTSTATUS InitializeHidPenetrate();


///////////////////////////////////////////////////////
//// Types
 

///////////////////////////////////////////////////////
//// Marcos and Utilies
 
#endif
