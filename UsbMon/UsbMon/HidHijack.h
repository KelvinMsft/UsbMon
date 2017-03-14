#ifndef __HIJACK_HEADER__
#define __HIJACK_HEADER__ 

#include<fltKernel.h>
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
	_In_ USB_HUB_VERSION UsbVersion
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

#endif