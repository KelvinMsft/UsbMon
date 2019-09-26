
#include <fltKernel.h>  
#include "UsbUtil.h" 
#include "UsbHid.h" 
#include "IrpHook.h"
#include "Usbioctl.h"
#include "ReportUtil.h"
#include "OpenLoopBuffer.h"
#include "WinParse.h"
#include "UsbType.h" 
#include "Tlist.h"

#pragma warning (disable : 4100) 


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Types
////  

#define SYSTEM_PTE_RANGE_START 0xFFFFF88000000000	
#define SYSTEM_PTE_RANGE_END   0xFFFFF89FFFFFFFFF	 

#define INSIDE_SYSTEM_PTE_RANGE(Address) (Address >= SYSTEM_PTE_RANGE_START && Address <= SYSTEM_PTE_RANGE_END)

#define NONE 0xFF

#define KPAD(_X_) 0x ## _X_ ## F0
#define SHFT(_X_) 0x ## _X_ ## F1
#define VEND(_X_) 0x ## _X_ ## F2


#define USB_MOU_DATA_HASH_MAKER(Data1)	\
	(Data1.x * Data1.x * 0x12356121 + Data1.y * Data1.y + 0x32151511 + Data1.z * Data1.z + Data1.Click * Data1.Click)

#define PRESS_MAKE 	0 
#define PRESS_BREAK 1

ULONG HidP_KeyboardToScanCodeTable[0x100] = {
	//
	// This is a straight lookup table
	//
	//     + 00     + 01     + 02     + 03     + 04     + 05     + 06    + 07
	//     + 08         + 09     + 0A     + 0B     + 0C     + 0D     + 0E    + OF
	/*0x00*/ NONE,    NONE,    NONE,    NONE,    0x1E,    0x30,    0x2E,   0x20,
	/*0x08*/ 0x12,    0x21,    0x22,    0x23,    0x17,    0x24,    0x25,   0x26,
	/*0x10*/ 0x32,    0x31,    0x18,    0x19,    0x10,    0x13,    0x1F,   0x14,
	/*0x18*/ 0x16,    0x2F,    0x11,    0x2D,    0x15,    0x2C,    0x02,   0x03,
	/*0x20*/ 0x04,    0x05,    0x06,    0x07,    0x08,    0x09,    0x0A,   0x0B,
	/*0x28*/ 0x1C,    0x01,    0x0E,    0x0F,    0x39,    0x0C,    0x0D,   0x1A,
	/*0x30*/ 0x1B,    0x2B,    0x2B,    0x27,    0x28,    0x29,    0x33,   0x34,
	/*0x38*/ 0x35,    SHFT(8), 0x3B,    0x3C,    0x3D,    0x3E,    0x3F,   0x40,
	/*0x40*/ 0x41,    0x42,    0x43,    0x44,    0x57,    0x58,    KPAD(0),SHFT(9),
	/*0x48*/ 0x451DE1,KPAD(1), KPAD(2), KPAD(3), KPAD(4), KPAD(5), KPAD(6),KPAD(7),
	/*0x50*/ KPAD(8), KPAD(9), KPAD(A), SHFT(A), 0x35E0,  0x37,    0x4A,   0x4E,
	/*0x58*/ 0x1CE0,  0x4F,    0x50,    0x51,    0x4B,    0x4C,    0x4D,   0x47,
	/*0x60*/ 0x48,    0x49,    0x52,    0x53,    0x56,    0x5DE0,  NONE,   0x59,
	/*0x68*/ 0x5d,    0x5e,    0x5f,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0x70*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0x78*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0x80*/ NONE,    NONE,    NONE,    NONE,    NONE,    0x7E,    NONE,   0x73,
	/*0x88*/ 0x70,    0x7d,    0x79,    0x7b,    0x5c,    NONE,    NONE,   NONE,
	/*0x90*/ VEND(0), VEND(1), 0x78,    0x77,    0x76,    NONE,    NONE,   NONE,
	/*0x98*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xA0*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xA8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xB0*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xB8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xC0*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xC8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xd0*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xD8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xE0*/ SHFT(0), SHFT(1), SHFT(2), SHFT(3), SHFT(4), SHFT(5), SHFT(6),SHFT(7),
	/*0xE8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*KPAD*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
	/*0xF8*/ NONE,    NONE,    NONE,    NONE,    NONE,    NONE,    NONE,   NONE,
};


ULONG HidP_XlateKbdPadCodesSubTable[] = {
	/*     + 00    + 01    + 02     + 03    + 04    + 05    + 06     + 07 */
	/*     + 08    + 09    + 0A     + 0B    + 0C    + 0D    + 0E     + OF */
	/*0x40*/                                                  0x37E0,
	/*0x48*/         0x52E0, 0x47E0, 0x49E0,  0x53E0, 0x4FE0, 0x51E0,  0x4DE0,
	/*0x50*/ 0x4BE0, 0x50E0, 0x48E0
};


ULONG HidP_XlateModifierCodesSubTable[] = {
	//
	// NOTE These modifier codes in this table are in a VERY SPECIAL order.
	// that is: they are in the order of appearence in the
	// _HIDP_KEYBOARD_SHIFT_STATE union.
	//
	//     + 00   + 01   + 02   + 03    + 04    + 05   + 06    + 07
	//     + 08   + 09   + 0A   + 0B    + 0C    + 0D   + 0E    + OF
	//       LCtrl  LShft  LAlt   LGUI    RCtrl   RShft  RAlt    RGUI
	/*0xE0*/ 0x1D,  0x2A,  0x38,  0x5BE0, 0x1DE0, 0x36,  0x38E0, 0x5CE0,
	/*0x39 CAPS_LOCK*/     0x3A,
	/*0x47 SCROLL_LOCK*/   0x46,
	/*0x53 NUM_LOCK*/      0x45
	// This table is set up so that indices into this table greater than 7
	// are sticky.  HidP_ModifierCode uses this as an optimization for
	// updating the Modifier state table.
	//
};



typedef struct IRP_HOOK_INFORMATION
{
	PENDINGIRPLIST*   pendingList;
	PVOID			  CompletionRoutine;
	PVOID			  OldRoutine;
	PVOID			  NewRoutine;
	PDRIVER_OBJECT	  DriverObject;		//UsbHub DriverObject
	ULONG			  IrpCode;
}IRPHOOKINFO, *PIRPHOOKINFO;


typedef struct HIJACK_CONTEXT
{
	PENDINGIRP			    pending_irp;		// pending irp node for cancellation 
	PDEVICE_OBJECT		   DeviceObject;		// Hid deivce
	PURB							urb;		// Urb packet saved in IRP hook
	HID_DEVICE_NODE*			   node;		// An old context we need  
}HIJACK_CONTEXT, *PHIJACK_CONTEXT;

/*
multi-process struct
*/
#pragma pack(push, 8)
typedef struct _USERPROCESS_INFO
{
	PEPROCESS proc;
	PVOID     MappedMouseAddr;
	PVOID     MappedKeyboardAddr;
	PMDL      MouseMdl;
	PMDL      KeyboardMdl;
}USERPROCESSINFO, *PUSERPROCESSINFO;
#pragma pack(pop)

#pragma pack(push, 8)
typedef struct _REPORT_MACRO_INFO
{
	LONG	 Data[8];
	CHAR	str[256];
}REPORTMACROINFO, *PREPORTMACROINFO;
#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////////////////////
////	Marco Utilities
////  
//// 	 

#define IsSafeNode(x)					 (x->node)
//Hijack Context Marco

//urb Marco
#define UrbGetFunction(urb)				 (urb->UrbHeader.Function)
#define UrbGetTransferBuffer(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferBuffer)
#define UrbGetTransferFlags(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferFlags)
#define UrbGetTransferLength(urb)		 (urb->UrbBulkOrInterruptTransfer.TransferBufferLength) 
#define UrbGetTransferPipeHandle(urb)	 (urb->UrbBulkOrInterruptTransfer.PipeHandle)
#define UrbIsInputTransfer(urb)			 (UrbGetTransferFlags(urb) & (USBD_TRANSFER_DIRECTION_IN ))
#define UrbIsOutputTransfer(urb)		 (UrbGetTransferFlags(urb) & (USBD_TRANSFER_DIRECTION_OUT | USBD_DEFAULT_PIPE_TRANSFER))

#define USB_PEN_INIT_VERSION			 0x00000001
#define USB_PEN_PROTOCOL_VERSION		 0x00000001

#define IS_CHECK_IRP_HOOK 				  0x0BB91E8A  //Hash(CheckIrpHook):0BB91E8A,196681354,196681354
#define IS_REPORT_RAW_DESC 				  0x94170A9F  //Hash(IsReportRawDesc):94170A9F,2484538015,-1810429281
#define USB_PENERATE_CONFIG 			  0x2443D155  //Hash(UsbPenerate):2443D155,608424277,608424277
#define USB_REQUIRED_DEVICE				  0xAFFE0399  //Hash(RequiredDevice):AFFE0399,2952659865,-1342307431
#define IS_KEYBOARD_COM_MODE_ON			  0xD4E8DF91  //Hash(KeyboardCompatibleMode):D4E8DF91,3572031377,-722935919
#define IS_REPORT_HID_MINI_DRV			  0xACA4BC02  //Hash(IsReportHidMiniDrv):ACA4BC02,2896477186,-1398490110
#define IS_SELF_PROTECTION				  0x4A0105D7  //Hash(SelfProtect):4A0105D7,1241581015,1241581015
#define IS_ENABLE_MODULE_CHECK			  0x71C3DAD6  //Hash(UsbModuleCheck):71C3DAD6,1908660950,1908660950

#define IS_DETECT_HARDWARE_MACRO		  0x819A413C  //Hash(DetectHardwareMacro):819A413C,2174370108,-2120597188
#define HARDWARE_MACRO_PACKET_TIME		  0x0DFE7038  //Hash(MacroPacketInterval):0DFE7038,234778680,234778680
#define HARDWARE_MACRO_REPORT_AVG		  0x27164714  //Hash(MacroReportAvg):27164714,655771412,655771412
#define HARDWARE_MACRO_REPORT_CLICK_TIMES 0x7585B465  //Hash(MacroReportClickTimes):7585B465,1971696741,1971696741



#define USB_MOUDATA_ABS_FLAGS 			 0x80000000
#define CB_SIZE 					 			128  
#define REPORT_MAX_COUNT 						  7
#define MAX_COORDINATE_ERROR					250
#define CONFIG_ARRAY_SIZE						  5 
#define COUNT_OF_REPORTID						  4
#define BITSIZE_OF_UCHAR				 		  8

#define RI_KEY_E0               				 2
#define RI_KEY_E1              					 4

#define IN_MATCH						 0xAF731671
#define NOT_IN_MATCH					 0x35781313
///////////////////////////////////////////////////////////////////////////////////////////////
////	Global Variable
////
////
DRIVER_DISPATCH*  g_HidPnpHandler = NULL;
BOOLEAN			  g_bLoaded = FALSE;
BOOLEAN 		  g_bUnloaded = FALSE;
TChainListHeader* g_ProcessInfo = NULL;
ULONG 			  g_ProcessCount = 0;
CIRCULARBUFFER*	  g_mou_data = NULL;
CIRCULARBUFFER*	  g_kbd_data = NULL;
ULONG 			  g_ReportRate = 8;
BOOLEAN			  g_IsFixedReportRate = FALSE;
BOOLEAN			  g_IsBugReported[2] = { 0 };
BOOLEAN			  g_IsReportRawDesc = FALSE;
BOOLEAN			  g_IsCheckIrpHook = FALSE;
ULONG 			  g_Index = 0;
LONG			  g_CompatibleRateX = 100;
LONG			  g_CompatibleRateY = 100;
LONG 		 	  g_RemainderX = 0;
LONG 			  g_RemainderY = 0;
HIDCONTEXT		  g_HidContext = { 0 };
BOOLEAN			  g_TpDataUsed[3] = { 0 };
BOOLEAN			  g_KeyStatus[512] = { 0 };
BOOLEAN 		  g_IsDifferentReport[2] = { 0 };
BOOLEAN			  g_KeyboardCompatibleMode = FALSE;
BOOLEAN			  g_IsEnableReportHidMiniDrv = FALSE;
BOOLEAN			  g_IsSelfProtect = FALSE;
ULONG64			  g_IsEnableModuleChecksum = 0;
ULONG 			  g_EncryptKey[4] = { 0 };
BOOLEAN			  g_bIsReportedCompFuncAttack = FALSE;
BOOLEAN			  g_bIsReportedCallbackHookAttack = FALSE;
BOOLEAN			  g_bIsReportedCompleteHookAttack = FALSE;

ULONG64 		  g_MaxPacketTime = 0;
BOOLEAN 		  g_EnableClick = 0;
BOOLEAN			  g_IsDetectHardwareMacro = 0;
LONG 			  g_SumOfYaxis = 0;
LONG			  g_LastAvgOfYaxis = 0;
LONG 			  g_CatchedTimes = 0;
LONG 			  g_CheatTimes = 0;
LONG 			  g_ClickTimes = 0;
ULONG64 		  g_StartTime = 0;
ULONG64 		  g_LastClickTime = 0;
ULONG64 		  g_LastTime = 0;
ULONG			  g_TimeOffset = 0;
LONG64			  g_ReportAvgMax = 0;
LONG			  g_TotalAvgOfYaxis = 0;
ULONG64			  g_MaxClickTimes = 0;
ULONG			  g_IsHwMacroReported = 0;
BOOLEAN			  g_LastKeyDown = FALSE;

extern PHID_DEVICE_LIST g_HidClientPdoList;
extern PUSB_HUB_LIST 	g_UsbHubList;


 PVOID UtilPcToFileHeader(
	_In_ PVOID pc_value
);
/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 
////

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Dispatch USB Keyboard Data to User Mode Mapped Buffer
It only called by HandleKeyboardData as it initilize
EXTRACTDATA structure.

Arguments:
pContext	- Hijack Context included all we need
see HIJACK_CONTEXT declaration.

Return Value:
NTSTATUS	- STATUS_SUCCESS 		if Handle successfully
- STATUS_UNSUCCESSFUL 	if failed
-------------------------------------------------------*/
NTSTATUS DispatchKeyboardData(
	_In_ HIJACK_CONTEXT* pContext
);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Dispatch USB Mouse Data to User Mode Mapped Buffer
It only called by HandleMousedData as it initilize
EXTRACTDATA structure.

Arguments:
pContext	- Hijack Context included all we need
see HIJACK_CONTEXT declaration.

Return Value:
NTSTATUS	- STATUS_SUCCESS 		if Handle successfully
- STATUS_UNSUCCESSFUL 	if failed
-------------------------------------------------------*/
NTSTATUS DispatchMouseData(
	_In_ HIJACK_CONTEXT* pContext
);

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

Handling And Extract data from keyboard URB

Arguments:
pContext	- Hijack Context included all we need
see HIJACK_CONTEXT declaration.

reportType	- Hid Report Type
-	HidP_Input,
-	HidP_Output,
-	HidP_Feature

Return Value:
NTSTATUS	- STATUS_SUCCESS 		if Handle successfully
- STATUS_UNSUCCESSFUL 	if failed
-------------------------------------------------------*/
NTSTATUS HandleKeyboardData(
	_In_ HIJACK_CONTEXT* pContext,
	_In_ HIDP_REPORT_TYPE reportType
);


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

Handling And Extract data from mouse URB

Arguments:

pContext	- Hijack Context included all we need ,
see HIJACK_CONTEXT declaration.

reportType  -	Hid Report Type
-	HidP_Input,
-	HidP_Output,
-	HidP_Feature

Return Value:
NTSTATUS	- STATUS_SUCCESS	  if Handle successfully
- STATUS_UNSUCCESSFUL if failed
-------------------------------------------------------*/
NTSTATUS HandleMouseData(
	_In_ HIJACK_CONTEXT* pContext,
	_In_ HIDP_REPORT_TYPE reportType
);


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Proxy Function of USB Hub 2 / 3 of Internal Control Irp

- It intercept all internal IOCTL ,

I.e. IOCTL_INTERNAL_USB_SUBMIT_URB -> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER

1. Check is it mouse / keyboard

2. Save a IRP Context (included old IRP completion function and context)

3. Hook Completion of IRP

4. Insert into Pending IRP List

Arguments:

DeviceObject 		- USB Hub Device Object, iusb3hub , usbhub ,... etc

Irp			 		- IRP come from upper level

pPendingList 		- pre-allocated pending list , each hubs use private list for
storing pendign IRP

pCompletionRoutine  - IRP Completion Function Address

pOldFunction		- CALL Old Function

Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on IRP Handler

-----------------------------------------------------------------------------------*/
NTSTATUS UsbhubIrpHandler(
	_Inout_ struct _DEVICE_OBJECT*	pDeviceObject,
	_Inout_ struct _IRP*			pIrp,
	_In_     PENDINGIRPLIST* 	   	pPendingList,
	_In_ 	 PIO_COMPLETION_ROUTINE pCompletionRoutine,
	_In_	 DRIVER_DISPATCH*		pOldFunction
);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Proxy Function of USB Hub IRP Completion Function (i.e. For URB IRP Request)
- It intercept all completion of Usbhub internal IOCTL ,

I.e. IOCTL_INTERNAL_USB_SUBMIT_URB -> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER

1. Get its Collection first (one report descrirptor can own more than one
collection description) it distinct by reportID, if reportID = 0 ,
it means only one collection, vice versa.

2. Dispatch the data to the UserMode Mapped buffer (if it does exist)

Arguments:

DeviceObject 		- USB Hub Device Object, iusb3hub , usbhub ,... etc

Irp			 		- IRP that is going to be completed

Context 			- IRP Context , we modifiy the Completion context and saved
old context in new context

pPendingList  		- The pending IRP list we need to removed ( usb2 / usb3 uses
independent list. )


Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on IRP Handler

-----------------------------------------------------------------------------------*/
NTSTATUS UsbIrpCompletionHandler(
	_In_     PDEVICE_OBJECT  	   DeviceObject,
	_In_     PIRP            	   Irp,
	_In_	 PVOID           	   Context,
	_In_     PENDINGIRPLIST* 	   pPendingList
);


//See HidHijack.h for detail
NTSTATUS InitializeHidPenetrate();

//See HidHijack.h for detail
NTSTATUS UnInitializeHidPenetrate();



/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Pnp Irp hook handler of UsbHid handler

- it verify the device first

- then insert or remove the node into the pipe list

- usbhub irp hook will next compared with it new device

Arguments:

DeviceObject - Client PDO we need

Irp		  - Irp request.

Return Value:

NTSTATUS	 - STATUS_UNSUCCESSFUL   , If failed of any one of argument check.
- STATUS_XXXXXXXXXXXX   , Depended on old completion

-----------------------------------------------------------------------------------*/
NTSTATUS HidUsbPnpIrpHandler(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
);



/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Routine Description:

- Free User-Mode Mapped Loop buffer

Arguments:

- No

Return Value:

NTSTATUS	 - STATUS_SUCCESS

-----------------------------------------------------------------------------------*/
NTSTATUS FreeMappedLoopBuffer();


//------------------------------------------------------------------------------------------//
extern PDO_EXTENSION*	GetClientPdoExtension(
	_In_ HIDCLASS_DEVICE_EXTENSION* HidCommonExt
);

//-----------------------------------------------------------------------------------------//
extern FDO_EXTENSION* GetFdoExtByClientPdoExt(
	_In_ PDO_EXTENSION* pdoExt
);


/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS  UsbCompletionCallback1(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
);


/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS  UsbCompletionCallback2(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
);

/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS  UsbCompletionCallback3(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
);

/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS  UsbCompletionCallback4(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
);


/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS  UsbCompletionCallback5(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
);



/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS UsbHubInternalDeviceControl1(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
);



/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS UsbHubInternalDeviceControl2(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
);


/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS UsbHubInternalDeviceControl3(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
);



/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS UsbHubInternalDeviceControl4(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
);



/*-----------------------------------------------------------------------------------*/
//	Stub
NTSTATUS UsbHubInternalDeviceControl5(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
);

/*-----------------------------------------------------------------------------------*/


/////////////////////////////////////////////////////////////////////////////////////////////////
/// Hook Config

IRPHOOKINFO g_HookConfig[CONFIG_ARRAY_SIZE] =
{
	{
		NULL,                             //pendingList;
		UsbCompletionCallback1,		      //CompletionCallback ;
		NULL,                             //OldRoutine;
		UsbHubInternalDeviceControl1,     //NewRoutine;
		NULL,                             //DriverObject;		//UsbHub DriverObject
		IRP_MJ_INTERNAL_DEVICE_CONTROL    //IrpCode;
	}
	,
	{
		NULL,						       //pendingList;
		UsbCompletionCallback2,		       //CompletionCallback ;
		NULL,                              //OldRoutine;
		UsbHubInternalDeviceControl2,      //NewRoutine;
		NULL,                              //DriverObject;		//UsbHub DriverObject
		IRP_MJ_INTERNAL_DEVICE_CONTROL     //IrpCode;
	}
	,

	{
		NULL,						       //pendingList;
		UsbCompletionCallback3,		       //CompletionCallback ;
		NULL,                              //OldRoutine;
		UsbHubInternalDeviceControl3,      //NewRoutine;
		NULL,                              //DriverObject;		//UsbHub DriverObject
		IRP_MJ_INTERNAL_DEVICE_CONTROL     //IrpCode;
	}
	,

	{
		NULL,						        //pendingList;
		UsbCompletionCallback4,		        //CompletionCallback ;
		NULL,                               //OldRoutine;
		UsbHubInternalDeviceControl4,       //NewRoutine;
		NULL,                               //DriverObject;		//UsbHub DriverObject
		IRP_MJ_INTERNAL_DEVICE_CONTROL      //IrpCode;
	}
	,

	{
		NULL,						        //pendingList;
		UsbCompletionCallback5,		        //CompletionCallback ;
		NULL,                               //OldRoutine;
		UsbHubInternalDeviceControl5,       //NewRoutine;
		NULL,                               //DriverObject;		//UsbHub DriverObject
		IRP_MJ_INTERNAL_DEVICE_CONTROL      //IrpCode;
	}
};


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
////  



//----------------------------------------------------------------------------------------//
BOOLEAN GetKeyboardCompatibleMode()
{
	return g_KeyboardCompatibleMode;
}

//----------------------------------------------------------------------------------------//
VOID SetMouseDiffRate(
	_In_	LONG RateX,
	_In_	LONG RateY
)
{
	g_CompatibleRateX = RateX;
	g_CompatibleRateY = RateY;
}

//----------------------------------------------------------------------------------------//
NTSTATUS GetAndVerifyKeyboardUsageInCollection(
	_In_ UCHAR*					 pSourceReport,
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_Out_ PHIDP_COLLECTION_DESC* pCollection
)
{
	return VerifyUsageInCollection(HID_KBD_USAGE, pSourceReport, pDesc, pCollection);
}

//----------------------------------------------------------------------------------------//
NTSTATUS GetAndVerifyMouseUsageInCollection(
	_In_ UCHAR*					 pSourceReport,
	_In_ HIDP_DEVICE_DESC* 	 	 pDesc,
	_Out_ PHIDP_COLLECTION_DESC* pCollection
)
{
	return VerifyUsageInCollection(HID_MOU_USAGE, pSourceReport, pDesc, pCollection);
}

//----------------------------------------------------------------------------------------//
NTSTATUS AllocateKeyboardDataStructure(
	_In_ HID_DEVICE_NODE* node,
	_In_ ULONG 			  ReportLen
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!node)
	{
		return STATUS_UNSUCCESSFUL;
	}
	if (node->PreviousUsageList)
	{
		return STATUS_SUCCESS;
	}

	if (!node->PreviousUsageList)
	{
		node->PreviousUsageList = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, 'kbdb');
	}

	if (node->PreviousUsageList)
	{
		RtlZeroMemory(node->PreviousUsageList, PAGE_SIZE);
		node->CurrentUsageList = node->PreviousUsageList + ReportLen;
		node->MakeUsageList = node->CurrentUsageList + ReportLen;
		node->BreakUsageList = node->MakeUsageList + ReportLen;
		status = STATUS_SUCCESS;
	}

	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS DispatchKeyboardModifierState(PUCHAR RawUsbData, HID_DEVICE_NODE* KeyboardNode)
{
	int i = 0;

	if (!KeyboardNode || !RawUsbData)
	{
		return STATUS_UNSUCCESSFUL;
	}

	for (i = 0; i < BITSIZE_OF_UCHAR; i++)
	{
		UCHAR TestBit = 1 << i;
		if ((KeyboardNode->ModifierState.ul & TestBit) != (*(PUCHAR)RawUsbData & TestBit))
		{
			USERKBDDATA kbd_data = { 0 };
			if (*(PUCHAR)RawUsbData & TestBit)
			{
				KeyboardNode->ModifierState.ul |= TestBit;
				kbd_data.ScanCode = HidP_XlateModifierCodesSubTable[i];
				kbd_data.Flags = PRESS_MAKE;
				// if E0 key , special , since we use it for DX
				if ((kbd_data.ScanCode & 0xFF) == 0xE0)
				{
					kbd_data.ScanCode >>= 8;
					kbd_data.Flags |= RI_KEY_E0;
				}
				USB_DEBUG_INFO_LN_EX("Press Modifier[%d] ScanCode: %d ", i, kbd_data.ScanCode);
			}
			else
			{
				KeyboardNode->ModifierState.ul &= ~TestBit;
				kbd_data.ScanCode = HidP_XlateModifierCodesSubTable[i];
				kbd_data.Flags = PRESS_BREAK;

				// if E0 key , special , since we use it for DX
				if ((kbd_data.ScanCode & 0xFF) == 0xE0)
				{
					kbd_data.ScanCode >>= 8;
					kbd_data.Flags |= RI_KEY_E0;
				}

				USB_DEBUG_INFO_LN_EX("Release Modifier[%d] ScanCode: %d ", i, kbd_data.ScanCode);
			}
			OpenLoopBufferWrite(g_kbd_data, (const void*)&kbd_data);
		}
	}
	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------//
NTSTATUS GetUsageMakeAndBreakList(
	_In_  PCHAR  CurrentUsageList,
	_In_  PCHAR  PreviousUsageList,
	_In_  ULONG  NormalKeySizeInByte,
	_Out_ PCHAR  MakeUsageList,
	_Out_ PCHAR  BreakUsageList,
	_Out_ PULONG pMakeIndex,
	_Out_ PULONG pBreakIndex)
{
	BOOLEAN  IsFound = FALSE;
	ULONG  BreakIndex = 0;
	ULONG  MakeIndex = 0;
	ULONG  m, b = 0;
	ULONG i = 0, j = 0;;

	if (!CurrentUsageList || !PreviousUsageList || !MakeUsageList || !BreakUsageList)
	{
		return STATUS_UNSUCCESSFUL;
	}

	//calculate by bytes 
	for (i = 0; i < NormalKeySizeInByte; i++)
	{
		UCHAR UsageId = PreviousUsageList[i];
		IsFound = FALSE;
		for (j = 0; j < NormalKeySizeInByte; j++)
		{
			if (UsageId == CurrentUsageList[j])
			{
				IsFound = TRUE;
				break;
			}
		}
		if (!IsFound)
		{
			BreakUsageList[BreakIndex++] = UsageId;
		}
	}

	//calculate by bytes
	for (i = 0; i < NormalKeySizeInByte; i++)
	{
		UCHAR UsageId = CurrentUsageList[i];
		IsFound = FALSE;
		for (j = 0; j < NormalKeySizeInByte; j++)
		{
			if (UsageId == PreviousUsageList[j])
			{
				IsFound = TRUE;
				break;
			}
		}
		if (!IsFound)
		{
			MakeUsageList[MakeIndex++] = UsageId;
		}
	}

	b = BreakIndex;
	m = MakeIndex;

	while (b < NormalKeySizeInByte) {
		BreakUsageList[b++] = 0;
	}
	while (m < NormalKeySizeInByte) {
		MakeUsageList[m++] = 0;
	}

	if (pMakeIndex)
	{
		*pMakeIndex = MakeIndex;
	}

	if (pBreakIndex)
	{
		*pBreakIndex = BreakIndex;
	}

	return STATUS_SUCCESS;
}
//-----------------------------------------------------------------------------------------//
NTSTATUS PutKeyIntoBuffer(
	_In_ HID_DEVICE_NODE* KeyboardNode,
	_In_ CIRCULARBUFFER* LoopBuffer,
	_In_ UCHAR  HidKey,
	_In_ ULONG  MakeOrBreak
)
{
	USERKBDDATA kbd_data = { 0 };

	if (!LoopBuffer && HidKey <= 0xFF)
	{
		return STATUS_UNSUCCESSFUL;
	}
	//1. First Translation
	kbd_data.ScanCode = HidP_KeyboardToScanCodeTable[HidKey];
	kbd_data.Flags = MakeOrBreak;


	if (kbd_data.ScanCode == 0x451DE1)
	{
		//1. First Translation
		kbd_data.ScanCode = 0xC5;
		kbd_data.Flags = MakeOrBreak | RI_KEY_E0;
		OpenLoopBufferWrite(g_kbd_data, (const void*)&kbd_data);
		return STATUS_SUCCESS;
	}

	//2. Second Translation, if required choose table.
	if ((kbd_data.ScanCode & 0xFF) == 0xF0)
	{
		kbd_data.ScanCode = HidP_XlateKbdPadCodesSubTable[((kbd_data.ScanCode & 0xFF00) >> 8)];
		//All key of the Second translation table also are E0 key.
		//And change it to DIK Code
		kbd_data.ScanCode >>= 8;
		kbd_data.Flags |= RI_KEY_E0;
	}
	if ((kbd_data.ScanCode & 0xFF) == 0xF1)
	{
		int index = ((kbd_data.ScanCode & 0xFF00) >> 8);
		kbd_data.ScanCode = HidP_XlateModifierCodesSubTable[index];

		/*---

		FIXME:
		We are not able to be consistent with sticky physical keyboard modifier state. (NumLock/ ScrollLock / CapsLock)

		if(index > 7 && MakeOrBreak == 0)
		{
		KeyboardNode->ModifierState.ul = KeyboardNode->ModifierState.ul ^ (1 << index) ;
		}

		--- */

		// if E0 key , special , since we use it for DX
		if ((kbd_data.ScanCode & 0xFF) == 0xE0)
		{
			kbd_data.ScanCode >>= 8;
			kbd_data.Flags |= RI_KEY_E0;
		}
	}
	//No need second translation but it is direct E0 key in first table
	else if ((kbd_data.ScanCode & 0xFF) == 0xE0)
	{
		kbd_data.ScanCode >>= 8;
		kbd_data.Flags |= RI_KEY_E0;
	}

	if (HidKey != 0)
	{
		USB_DEBUG_INFO_LN_EX("Put [%d] key: %d DIKCode: %x ", MakeOrBreak, HidKey, kbd_data.ScanCode);
		OpenLoopBufferWrite(g_kbd_data, (const void*)&kbd_data);
	}

	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------//
NTSTATUS DispatchKeyboardNormalKey(
	_In_	PUCHAR 				RawUsbData,
	_In_	HID_DEVICE_NODE* 	KeyboardNode,
	_In_	ULONG 				NormalKeySizeInByte)
{
	ULONG OverrunErrorCheck = 0;
	ULONG i = 0, j = 0;
	CHAR* PreviousUsageList = NULL;
	CHAR* CurrentUsageList = NULL;
	CHAR* BreakUsageList = NULL;
	CHAR* MakeUsageList = NULL;
	ULONG MakeIndex = 0;
	ULONG BreakIndex = 0;

	NTSTATUS status = STATUS_UNSUCCESSFUL;


	if (!KeyboardNode || !RawUsbData)
	{
		return status;
	}

	PreviousUsageList = KeyboardNode->PreviousUsageList;
	CurrentUsageList = KeyboardNode->CurrentUsageList;
	BreakUsageList = KeyboardNode->BreakUsageList;
	MakeUsageList = KeyboardNode->MakeUsageList;

	if (!PreviousUsageList || !CurrentUsageList || !BreakUsageList || !MakeUsageList)
	{
		USB_DEBUG_INFO_LN_EX("FATAL: Empty Keyboard List Error");
		return status;
	}

	memcpy(CurrentUsageList, RawUsbData, NormalKeySizeInByte);

	GetUsageMakeAndBreakList(CurrentUsageList, PreviousUsageList, NormalKeySizeInByte, MakeUsageList, BreakUsageList, &MakeIndex, &BreakIndex);

	memcpy(&PreviousUsageList[0], &CurrentUsageList[0], NormalKeySizeInByte);

	USB_DEBUG_INFO_LN_EX("KeyboardNode->ModifierState: %x ", KeyboardNode->ModifierState);

	for (i = 0; i < BreakIndex; i++)
	{
		PutKeyIntoBuffer(KeyboardNode, g_kbd_data, BreakUsageList[i], 1);
	}

	for (i = 0; i < MakeIndex; i++)
	{
		PutKeyIntoBuffer(KeyboardNode, g_kbd_data, MakeUsageList[i], 0);
	}
	status = STATUS_SUCCESS;
	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS DispatchKeyboardData(
	_In_ HIJACK_CONTEXT* pContext
)
{
	ULONG  i = 0;
	ULONG j = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	EXTRACTDATA* data = NULL;
	ULONG OldState = 0;

	UCHAR IndexSpecialKey = 0xE0;
	ULONG SpecialKeySizeInByte = 0;
	ULONG ActiveReportLen = 0;
	ULONG NormalKeySizeInByte = 0;
	PUCHAR StartPtr = NULL;

	BOOLEAN IsFound = FALSE;

	if (!pContext)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (!pContext->node)
	{
		return STATUS_UNSUCCESSFUL;
	}

	data = pContext->node->ExtractedData[HidP_Input];
	if (!data)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	NormalKeySizeInByte = data->KBDDATA.NormalKeySize / 8;
	SpecialKeySizeInByte = data->KBDDATA.SpecialKeySize / 8;
	ActiveReportLen = SpecialKeySizeInByte + NormalKeySizeInByte;
	if (!ActiveReportLen)
	{
		USB_DEBUG_INFO_LN_EX("Zero Report Len");
		return status;
	}

	if (!pContext->node->PreviousUsageList)
	{
		status = AllocateKeyboardDataStructure(pContext->node, ActiveReportLen);
		if (!NT_SUCCESS(status))
		{
			USB_DEBUG_INFO_LN_EX("FATAL: Allocated Keyboard List Error");
			return status;
		}
	}

	StartPtr = (PUCHAR)UrbGetTransferBuffer(pContext->urb);
	if (data->KBDDATA.ReportId != 0)
	{
		if (data->KBDDATA.ReportId != *(PUCHAR)StartPtr)
		{
			status = STATUS_UNSUCCESSFUL;
			g_IsDifferentReport[1] = TRUE;
			return status;
		}
	}

	StartPtr += data->KBDDATA.SpecialKeyByteOffset;
	DispatchKeyboardModifierState(StartPtr, pContext->node);

	StartPtr = (PCHAR)UrbGetTransferBuffer(pContext->urb);
	StartPtr += data->KBDDATA.NormalKeyByteOffset;

	if (*StartPtr == 1)
	{
		USB_DEBUG_INFO_LN_EX("Overrun Packet");
		return status;
	}

	DispatchKeyboardNormalKey(StartPtr, pContext->node, NormalKeySizeInByte);

	USB_DEBUG_INFO_LN_EX("Kenrel USERKBDDATA size: %d", sizeof(USERKBDDATA));
	USB_DEBUG_INFO_LN_EX("Keyboard Request .... %x %x %x\r\n", data->KBDDATA.NormalKeyByteOffset, data->KBDDATA.SpecialKeyByteOffset, data->KBDDATA.NormalKeySize);

	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------//
void ReportMacroDetection(REPORTMACROINFO* info)
{
	if (!info)
	{
		return;
	}
	ExFreePool(info);
	info = NULL;

	return;
}

//----------------------------------------------------------------------------------------//
NTSTATUS DispatchMouseData(
	_In_	HIJACK_CONTEXT* pContext
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG totalSize = 0;
	EXTRACTDATA* data = NULL;

	data = pContext->node->ExtractedData[HidP_Input];
	if (!data)
	{
		USB_DEBUG_INFO_LN_EX("data NULL");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (data->MOUDATA.ReportIdDesc[0].ReportId)
	{
		if (data->MOUDATA.ReportIdDesc[0].ReportId != *(PCHAR)UrbGetTransferBuffer(pContext->urb) &&
			data->MOUDATA.ReportIdDesc[1].ReportId != *(PCHAR)UrbGetTransferBuffer(pContext->urb) &&
			data->MOUDATA.ReportIdDesc[2].ReportId != *(PCHAR)UrbGetTransferBuffer(pContext->urb) &&
			data->MOUDATA.ReportIdDesc[3].ReportId != *(PCHAR)UrbGetTransferBuffer(pContext->urb) &&
			!g_IsDifferentReport[0])
		{
			USB_DEBUG_INFO_LN_EX("ReportId Error");
			status = STATUS_UNSUCCESSFUL;
			return status;
		}
	}

	totalSize = data->MOUDATA.X_Descriptor.OffsetSize + data->MOUDATA.Y_Descriptor.OffsetSize + data->MOUDATA.Z_Descriptor.OffsetSize + data->MOUDATA.BtnDescriptor.BtnOffsetSize[0];
	if (!totalSize)
	{
		USB_DEBUG_INFO_LN_EX("Cannot Get Moudata \r\n");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}


	if (g_mou_data)
	{
		USERMOUDATA mou_data = { 0 };
		LONG		BtnSet1, BtnSet2 = 0;
		LONG X, RemainderX = 0;
		LONG Y, RemainderY = 0;
		UCHAR DataReportID = *(UCHAR*)UrbGetTransferBuffer(pContext->urb);
		int i = 0;
		int j = 0;

		if (data->MOUDATA.ReportIdDesc[0].ReportId)
		{
			for (i = 0; i < COUNT_OF_REPORTID; i++)
			{
				// Case 1: Our target is same Report ID
				// Case 2: Our target is not same Report ID 
				if (data->MOUDATA.ReportIdDesc[i].ReportId == DataReportID)
				{
					switch (data->MOUDATA.ReportIdDesc[i].Usage)
					{
					case HID_NOT_RANGE_USAGE_X:
						mou_data.x =
							ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.X_Descriptor.ByteOffset, data->MOUDATA.X_Descriptor.BitOffset, data->MOUDATA.X_Descriptor.OffsetSize);
						break;
					case HID_NOT_RANGE_USAGE_Y:
						mou_data.y =
							ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.Y_Descriptor.ByteOffset, data->MOUDATA.Y_Descriptor.BitOffset, data->MOUDATA.Y_Descriptor.OffsetSize);
						break;
					case HID_NOT_RANGE_USAGE_WHELL:
						mou_data.z =
							ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.Z_Descriptor.ByteOffset, data->MOUDATA.Z_Descriptor.BitOffset, data->MOUDATA.Z_Descriptor.OffsetSize);
						break;
					case HID_BUTTON:
						BtnSet1 = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.BtnDescriptor.ByteOffsetBtn[0], data->MOUDATA.BtnDescriptor.BitOffsetBtn[0], data->MOUDATA.BtnDescriptor.BtnOffsetSize[0]);
						break;
					default:
						break;
					}
				}
				USB_DEBUG_INFO_LN_EX("i: %d ReportId: %d Usage: %x", i, DataReportID, data->MOUDATA.ReportIdDesc[i].Usage);
			}
		}
		else
		{
			//Case 3: No Report id
			mou_data.x = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.X_Descriptor.ByteOffset, data->MOUDATA.X_Descriptor.BitOffset, data->MOUDATA.X_Descriptor.OffsetSize);
			mou_data.y = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.Y_Descriptor.ByteOffset, data->MOUDATA.Y_Descriptor.BitOffset, data->MOUDATA.Y_Descriptor.OffsetSize);
			mou_data.z = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.Z_Descriptor.ByteOffset, data->MOUDATA.Z_Descriptor.BitOffset, data->MOUDATA.Z_Descriptor.OffsetSize);
			BtnSet1 = ReadHidCoordinate((PCHAR)UrbGetTransferBuffer(pContext->urb), data->MOUDATA.BtnDescriptor.ByteOffsetBtn[0], data->MOUDATA.BtnDescriptor.BitOffsetBtn[0], data->MOUDATA.BtnDescriptor.BtnOffsetSize[0]);
		}

		mou_data.Click = BtnSet1;

//		EvaluateHardwareMacro(pContext, &mou_data);

		mou_data.IsAbsolute = (data->MOUDATA.IsAbsolute) ? (mou_data.IsAbsolute | USB_MOUDATA_ABS_FLAGS) : (mou_data.IsAbsolute & ~USB_MOUDATA_ABS_FLAGS);

		mou_data.Reserved1 = (ULONG)USB_MOU_DATA_HASH_MAKER(mou_data);

		OpenLoopBufferWrite(g_mou_data, (const void*)&mou_data);


#ifdef DBG
		{
			char str[REPORT_MAX_COUNT * 4 + 1] = { 0 };
			ULONG size = (UrbGetTransferLength(pContext->urb) > REPORT_MAX_COUNT) ? REPORT_MAX_COUNT : UrbGetTransferLength(pContext->urb);
			//binToString((PCHAR)UrbGetTransferBuffer(pContext->urb), size, str, REPORT_MAX_COUNT * 4 + 1);

			USB_DEBUG_INFO_LN_EX("ReportId: %d X: %d Y: %d Z: %d Click: %d Abs: %d Size: %d str: %s Device_obj: %I64x hash= %x",
				data->MOUDATA.ReportIdDesc[0].ReportId,
				mou_data.x,
				mou_data.y,
				mou_data.z,
				mou_data.Click,
				mou_data.IsAbsolute,
				data->MOUDATA.X_Descriptor.OffsetSize,
				str,
				pContext->DeviceObject,
				mou_data.Reserved1);
		}


#endif
		RtlZeroMemory(&mou_data, sizeof(USERMOUDATA));
	}

	status = STATUS_SUCCESS;
	return status;
}

//----------------------------------------------------------------------------------------//
NTSTATUS HandleKeyboardData(
	_In_ HIJACK_CONTEXT*    pContext,
	_In_ HIDP_REPORT_TYPE reportType
)
{
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	ULONG						colIndex = 0;
	USHORT						totalSize = 0;
	BOOLEAN						IsMouse = FALSE;
	BOOLEAN						IsKbd = FALSE;
	ULONG						index = 0;
	PCHAR 						StartPtr = NULL;
	int	 						i = 0;

	USERKBDDATA kbd_data = { 0 };
	if (!pContext)
	{
		USB_DEBUG_INFO_LN_EX("NULL pContext");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (!pContext->urb || !pContext->DeviceObject || !IsSafeNode(pContext))
	{
		USB_DEBUG_INFO_LN_EX("Return without handle");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (!pContext->node->ExtractedData[reportType])
	{
		if (!pContext->node->Collection)
		{
			status = GetAndVerifyKeyboardUsageInCollection(
				UrbGetTransferBuffer(pContext->urb),
				&pContext->node->parsedReport,
				&pContext->node->Collection
			);

			if (!NT_SUCCESS(status) || !pContext->node->Collection)
			{
				USB_DEBUG_INFO_LN_EX("NULL VerifyMouseUsageInCollection");
				status = STATUS_UNSUCCESSFUL;
				return status;
			}
		}

		status = AllocateExtractData(&pContext->node->ExtractedData[reportType]);
		if (!NT_SUCCESS(status) || (!pContext->node->ExtractedData[reportType]))
		{
		
			status = STATUS_UNSUCCESSFUL;
			return status;
		}

		status = ExtractKeyboardData(pContext->node->Collection, reportType, pContext->node->ExtractedData[reportType]);
		if (!NT_SUCCESS(status) || (pContext->node->ExtractedData[reportType]->KBDDATA.NormalKeyByteOffset == pContext->node->ExtractedData[reportType]->KBDDATA.SpecialKeyByteOffset))
		{
			status = STATUS_UNSUCCESSFUL;
			return status;
		}
	}
	if (g_kbd_data)
	{
		status = DispatchKeyboardData(pContext);
	}

	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS HandleMouseData(
	_In_ HIJACK_CONTEXT*    pContext,
	_In_ HIDP_REPORT_TYPE reportType
)
{
	EXTRACTDATA*				data = NULL;
	NTSTATUS					status = STATUS_UNSUCCESSFUL;
	ULONG						colIndex = 0;
	USHORT						totalSize = 0;
	BOOLEAN						IsMouse = FALSE;
	BOOLEAN						IsKbd = FALSE;
	ULONG						index = 0;

	if (!pContext)
	{
		USB_DEBUG_INFO_LN_EX("NULL pContext");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}
	if (!pContext->urb || !pContext->DeviceObject || !IsSafeNode(pContext))
	{
		USB_DEBUG_INFO_LN_EX("Return without handle");
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	if (!pContext->node->ExtractedData[reportType])
	{
		USB_COMMON_DEBUG_INFO("NULL ExtractedData");
		if (!pContext->node->Collection)
		{
			USB_COMMON_DEBUG_INFO("NULL Collection");
			status = GetAndVerifyMouseUsageInCollection(
				UrbGetTransferBuffer(pContext->urb),
				&pContext->node->parsedReport,
				&pContext->node->Collection
			);

			if (!NT_SUCCESS(status) || !pContext->node->Collection)
			{
				USB_DEBUG_INFO_LN_EX("NULL GetVerifyMouseUsageInCollection");
				status = STATUS_UNSUCCESSFUL;
				return status;
			}
		}

		status = AllocateExtractData(&pContext->node->ExtractedData[reportType]);
		if (!NT_SUCCESS(status) || (!pContext->node->ExtractedData[reportType]))
		{
			USB_COMMON_DEBUG_INFO("NULL AllocateExtractData");
			status = STATUS_UNSUCCESSFUL;
			return status;
		}

		status = ExtractMouseData(pContext->node->Collection, reportType, pContext->node->ExtractedData[reportType]);
		if (!NT_SUCCESS(status))
		{
			USB_COMMON_DEBUG_INFO("NULL ExtractMouseData");
			status = STATUS_UNSUCCESSFUL;
			return status;
		}
		USB_COMMON_DEBUG_INFO("Alloc Success");
	}

	if (g_mou_data)
	{
		status = DispatchMouseData(pContext);
	}
	return status;
}
//---------------------------------------------------------------------//
NTSTATUS VerifyClientPdoIrp(
	_In_ PDEVICE_OBJECT ClientPdo,
	_In_ PDEVICE_OBJECT VerifiedFdo)
{
	PDO_EXTENSION* pdoExt = NULL;
	FDO_EXTENSION* fdoExt = NULL;

	if (!ClientPdo || !VerifiedFdo)
	{
		return  STATUS_UNSUCCESSFUL;
	}
	/*
	-------------------------
	|	FDO (_HID0000001)	|				<< DeviceObject
	-------------------------
	-------|-------
	|			   |
	----------------	--------------
	| Client PDO1  |	| Client PDO2 |			<< Context->node->device_obj
	----------------	--------------

	*/

	pdoExt = GetClientPdoExtension(ClientPdo->DeviceExtension);
	if (!pdoExt)
	{
		return  STATUS_UNSUCCESSFUL;
	}

	fdoExt = GetFdoExtByClientPdoExt(pdoExt);
	if (!fdoExt)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (fdoExt->fdo != VerifiedFdo)
	{
		USB_DEBUG_INFO_LN_EX("IRPDevice HID: %I64x  %I64x --- ", fdoExt->fdo, VerifiedFdo);
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}
//---------------------------------------------------------------------//
NTSTATUS UsbIrpCompletionHandler(
	_In_     PDEVICE_OBJECT  	   DeviceObject,
	_In_     PIRP            	   Irp,
	_In_	 PVOID           	   Context,
	_In_     PENDINGIRPLIST* 	   pPendingList
)
{
	HIJACK_CONTEXT*		   pContext = (HIJACK_CONTEXT*)Context;
	PVOID					context = NULL;
	IO_COMPLETION_ROUTINE* callback = NULL;
	if (!pContext)
	{
		return STATUS_SUCCESS;
	}

	context = pContext->pending_irp.oldContext;
	callback = pContext->pending_irp.oldRoutine;
	if (!callback)
	{
		return STATUS_UNSUCCESSFUL;
	}

	// Completion func is called when driver unloading.
	if (g_bUnloaded)
	{
		PENDINGIRP* entry = GetRealPendingIrpByIrp(pPendingList, Irp);
		if (entry)
		{
			context = entry->oldContext;
			callback = entry->oldRoutine;
			USB_DEBUG_INFO_LN_EX("Safety Call old Routine: %I64x Context: %I64x", callback, context);
		}
	}

	//Rarely call here when driver unloading, Safely call because driver_unload 
	//will handle all element in the list. we dun need to free and unlink it,
	//otherwise, system crash.
	if (g_bUnloaded)
	{
		if (!callback)
		{
			USB_DEBUG_INFO_LN_EX("callback Empty...");
			return STATUS_SUCCESS;
		}
		return callback(DeviceObject, Irp, context);
	}

	//If Driver is not unloading , delete it 
	if (!NT_SUCCESS(RemovePendingIrp(pPendingList, &pContext->pending_irp)))
	{
		USB_DEBUG_INFO_LN_EX("FATAL: Delete element FAILED");
	}

	if (!NT_SUCCESS(VerifyClientPdoIrp(pContext->node->device_object, DeviceObject)))
	{
		return callback(DeviceObject, Irp, context);
	}

	if (UrbIsInputTransfer(pContext->urb))
	{
		if (pContext->node->mini_extension->InterfaceDesc->Class == 3 &&			//HidClass Device
			pContext->node->mini_extension->InterfaceDesc->Protocol == 2)//Mouse
		{
			USB_DEBUG_INFO_LN_EX("IRPDevice HID: %I64x  ", DeviceObject);
			if (!NT_SUCCESS(HandleMouseData(pContext, HidP_Input)))
			{
				USB_DEBUG_INFO_LN_EX("It is not mouse data ");
			}
		}

		if (pContext->node->mini_extension->InterfaceDesc->Class == 3 &&			//HidClass Device
			pContext->node->mini_extension->InterfaceDesc->Protocol == 1)//Keyboard
		{
			if (!NT_SUCCESS(HandleKeyboardData(pContext, HidP_Input)))
			{
				USB_DEBUG_INFO_LN_EX("It is not Keyboard Data ");
			}
		}
	}

	if (UrbIsOutputTransfer(pContext->urb))
	{
		USB_COMMON_DEBUG_INFO("Output Data ");
	}

	if (pContext)
	{
		ExFreePool(pContext);
		pContext = NULL;
	}

	USB_COMMON_DEBUG_INFO("Completion ");

	return callback(DeviceObject, Irp, context);
}
//-------------------------------------------------------------------------------------------------//
NTSTATUS  UsbCompletionCallback1(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
)
{
	PVOID NtBase = UtilPcToFileHeader(KdDebuggerEnabled);
	PVOID CallerBase = UtilPcToFileHeader(_ReturnAddress());

	if (NtBase != CallerBase && !g_bIsReportedCompFuncAttack)
	{
		g_bIsReportedCompFuncAttack = TRUE;
	}
	return UsbIrpCompletionHandler(DeviceObject, Irp, Context, g_HookConfig[0].pendingList);
}
//-------------------------------------------------------------------------------------------------//
NTSTATUS  UsbCompletionCallback2(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
)
{
	return UsbIrpCompletionHandler(DeviceObject, Irp, Context, g_HookConfig[1].pendingList);
}
//-----------------------------------------------------------------------------------------------//
NTSTATUS  UsbCompletionCallback3(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
)
{
	return UsbIrpCompletionHandler(DeviceObject, Irp, Context, g_HookConfig[2].pendingList);
}
//-------------------------------------------------------------------------------------------------//
NTSTATUS  UsbCompletionCallback4(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
)
{
	return UsbIrpCompletionHandler(DeviceObject, Irp, Context, g_HookConfig[3].pendingList);
}
//-------------------------------------------------------------------------------------------------//
NTSTATUS  UsbCompletionCallback5(
	_In_     PDEVICE_OBJECT DeviceObject,
	_In_     PIRP           Irp,
	_In_	 PVOID          Context
)
{
	return UsbIrpCompletionHandler(DeviceObject, Irp, Context, g_HookConfig[4].pendingList);
}


//-------------------------------------------------------------------------------------------------//
NTSTATUS UsbhubIrpHandler(
	_Inout_ struct _DEVICE_OBJECT*	pDeviceObject,
	_Inout_ struct _IRP*			pIrp,
	_In_     PENDINGIRPLIST* 	   	pPendingList,
	_In_ 	 PIO_COMPLETION_ROUTINE pCompletionRoutine,
	_In_	 DRIVER_DISPATCH*		pOldFunction
)
{
	PIO_STACK_LOCATION irpStack = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	do
	{
		HIJACK_CONTEXT*		 hijack = NULL;

		PURB					urb = NULL;
		PENDINGIRP*		  new_entry = NULL;
		PHID_DEVICE_NODE	   node = NULL;

		if (!g_HidClientPdoList)
		{
			break;
		}

		if (!pPendingList)
		{
			break;
		}

		irpStack = IoGetCurrentIrpStackLocation(pIrp);
		if (irpStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
		{
			USB_DEBUG_INFO_LN_EX("not IOCTL_INTERNAL_USB_SUBMIT_URB ");
			break;
		}

		urb = (PURB)irpStack->Parameters.Others.Argument1;
		if (!urb)
		{
			USB_DEBUG_INFO_LN_EX("empty urb ");
			break;
		}

		if (UrbGetFunction(urb) != URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
		{
			USB_DEBUG_INFO_LN_EX("not URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER %x ", UrbGetFunction(urb));
			break;
		}

		//If Urb pipe handle is used by HID mouse / kbd device. 
		if (IsHidDevicePipe(g_HidClientPdoList->head, UrbGetTransferPipeHandle(urb), &node))
		{
#ifdef DBG	
			CHAR DriverName[256] = { 0 };
			WCHAR DeviceNameW[256] = { 0 };
			CHAR  DeviceName[256] = { 0 };
#endif
			HIDCLASS_DEVICE_EXTENSION* class_extension = NULL;

			if (!node ||
				!node->device_object)
			{
				USB_DEBUG_INFO_LN_EX("Is Null Node. ");
				break;
			}

			class_extension = (HIDCLASS_DEVICE_EXTENSION*)node->device_object->DeviceExtension;
			if (!class_extension->isClientPdo)
			{
				//if FDO (HID_000000x), then next
				USB_DEBUG_INFO_LN_EX("Is Not isClientPdo. ");
				break;
			}
#ifdef DBG			
			if (KeGetCurrentIrql() < DISPATCH_LEVEL)
			{
//				wcharTocharEN(node->device_object->DriverObject->DriverName.Buffer, DriverName, node->device_object->DriverObject->DriverName.Length);
//				GetDeviceName(node->device_object, DeviceNameW);
//				wcharTocharEN(DeviceNameW, DeviceName, 256);
//				USB_DEBUG_INFO_LN_EX("DrvName:  %s DeviceName: %s", DriverName, DeviceName);
			}
#endif
			hijack = (HIJACK_CONTEXT*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HIJACK_CONTEXT), 'kcaj');
			if (!hijack)
			{
				break;
			}

			RtlZeroMemory(hijack, sizeof(HIJACK_CONTEXT));

			//Save all we need to use when unload driver / delete node , 
			//Add to linked list
			if (irpStack)
			{
				//Fake Context for Completion Routine
				hijack->DeviceObject = node->device_object;		//USBHUB device
				hijack->urb = urb;
				hijack->node = node;
				hijack->pending_irp.Irp = pIrp;
				hijack->pending_irp.oldRoutine = irpStack->CompletionRoutine;
				hijack->pending_irp.oldContext = irpStack->Context;
				hijack->pending_irp.IrpStack = irpStack;
				hijack->pending_irp.newRoutine = pCompletionRoutine;
				hijack->pending_irp.newContext = hijack;
				// List for bookkepping only.
				//1. Free From UnInit Function 
				//2. Free From IRP Completion Hook
				InsertPendingIrp(pPendingList, &hijack->pending_irp);
				irpStack->CompletionRoutine = pCompletionRoutine;

				//Completion Routine hook
				irpStack->Context = hijack;

				if (!INSIDE_SYSTEM_PTE_RANGE((ULONG64)pOldFunction))
				{
					if (!g_bIsReportedCallbackHookAttack)
					{
						g_bIsReportedCallbackHookAttack = TRUE;
						USB_DEBUG_INFO_LN_EX("IrpStack: pOldFunction: %p", pOldFunction);
					}
				}

				status = pOldFunction(pDeviceObject, pIrp);

				if (!NT_SUCCESS(IrpVerifyPendingIrpCompletionHookByIrp(pPendingList, pIrp)))
				{
					if (!g_bIsReportedCompleteHookAttack)
					{
						g_bIsReportedCompleteHookAttack = TRUE;
						USB_DEBUG_INFO_LN_EX("IrpStack: pOldFunction: %p Current Completion: %p MyCompletion: %p", pOldFunction, irpStack->CompletionRoutine, pCompletionRoutine);
					}
				}
				else
				{
					USB_DEBUG_INFO_LN_EX("Normal completion hook ");
				}

				return status;
			}
		}
	} while (0);

	status = pOldFunction(pDeviceObject, pIrp);

	return status;
}
//-------------------------------------------------------------------------------------------------//
NTSTATUS UsbHubInternalDeviceControl1(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	return UsbhubIrpHandler(
		DeviceObject,
		Irp,
		g_HookConfig[0].pendingList,
		g_HookConfig[0].CompletionRoutine,
		g_HookConfig[0].OldRoutine
	);
}

//-------------------------------------------------------------------------------------------------//
NTSTATUS UsbHubInternalDeviceControl2(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	return UsbhubIrpHandler(
		DeviceObject,
		Irp,
		g_HookConfig[1].pendingList,
		g_HookConfig[1].CompletionRoutine,
		g_HookConfig[1].OldRoutine
	);
}
//-------------------------------------------------------------------------------------------------//
NTSTATUS UsbHubInternalDeviceControl3(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	return UsbhubIrpHandler(
		DeviceObject,
		Irp,
		g_HookConfig[2].pendingList,
		g_HookConfig[2].CompletionRoutine,
		g_HookConfig[2].OldRoutine
	);
}

//-------------------------------------------------------------------------------------------------//
NTSTATUS UsbHubInternalDeviceControl4(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	return UsbhubIrpHandler(
		DeviceObject,
		Irp,
		g_HookConfig[3].pendingList,
		g_HookConfig[3].CompletionRoutine,
		g_HookConfig[3].OldRoutine
	);
}

//-------------------------------------------------------------------------------------------------//
NTSTATUS UsbHubInternalDeviceControl5(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	return UsbhubIrpHandler(
		DeviceObject,
		Irp,
		g_HookConfig[4].pendingList,
		g_HookConfig[4].CompletionRoutine,
		g_HookConfig[4].OldRoutine
	);
}


//-------------------------------------------------------------------------------------------------//
NTSTATUS HidUsbPnpIrpHandler(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
)
{
	IRPHOOKOBJ* HookObject = NULL;
	PIO_STACK_LOCATION irpStack;

	do
	{
		HIDCLASS_DEVICE_EXTENSION* ClassExt = (HIDCLASS_DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
		if (ClassExt->isClientPdo)
		{
			USB_DEBUG_INFO_LN_EX("IsClientPdo: %x", ClassExt->isClientPdo);

			irpStack = IoGetCurrentIrpStackLocation(Irp);
			if (irpStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
			{
				RemoveNodeFromHidList(DeviceObject);
				USB_DEBUG_INFO_LN_EX("REMOVE Device ");
			}

			if (irpStack->MinorFunction == IRP_MN_START_DEVICE)
			{
				USB_DEBUG_INFO_LN_EX("Add Device");
				VerifyAndInsertIntoHidList(DeviceObject, &g_HidContext);
			}
		}
	} while (FALSE);

	return g_HidPnpHandler(DeviceObject, Irp);
}

//----------------------------------------------------------------------------------------//
PENDINGIRPLIST* AllocateUsbHubsPendingList()
{
	PENDINGIRPLIST* UsbPendingIrpList = NULL;
	if (!NT_SUCCESS(AllocatePendingIrpLinkedList(&UsbPendingIrpList)))
	{
		USB_DEBUG_INFO_LN_EX("AllocatePendingIrpLinkedList Error");
	}
	return UsbPendingIrpList;
}
//----------------------------------------------------------------------------------------------------------------------------------------------
ULONG __fastcall FindInfoByProcess(
	_In_  USERPROCESSINFO* Header,
	_In_ PEPROCESS proc
)
{
	if (Header)
	{
		if (Header->proc == proc)
		{
			return CLIST_FINDCB_RET;
		}
	}
	return CLIST_FINDCB_CTN;
}
//----------------------------------------------------------------------------------------------------------------------------------------------
USERPROCESSINFO*  __fastcall FindMappedUsbInfo(
	_In_ PEPROCESS proc
)
{
	if (g_ProcessInfo)
	{
		return (USERPROCESSINFO*)QueryFromChainListByCallback(g_ProcessInfo, &FindInfoByProcess, proc);
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------//
NTSTATUS CreateProcessInfo(
	_Out_ PUSERPROCESSINFO* ProcessInfo,
	_In_ 	void* 	 source,
	_In_ 	ULONG 	 length,
	_In_ 	BOOLEAN  IsKeyoard
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	USERPROCESSINFO* processInfo = NULL;

	if (!ProcessInfo || !source)
	{
		return status;
	}
	processInfo = (USERPROCESSINFO*)ExAllocatePoolWithTag(NonPagedPool, sizeof(USERPROCESSINFO), 'pinf');
	if (!processInfo)
	{
		return status;
	}
	RtlZeroMemory(processInfo, sizeof(USERPROCESSINFO));
	processInfo->proc = PsGetCurrentProcess();
	if (!IsKeyoard)
	{
		processInfo->MouseMdl = NULL;
		processInfo->MappedMouseAddr = NULL;
		processInfo->MappedMouseAddr = MapNonpagedMemToSpace(source, length, &processInfo->MouseMdl, UserMode, 0);
	}
	else
	{
		processInfo->KeyboardMdl = NULL;
		processInfo->MappedKeyboardAddr = NULL;
		processInfo->MappedKeyboardAddr = MapNonpagedMemToSpace(source, length, &processInfo->KeyboardMdl, UserMode, 0);
	}

	if (processInfo)
	{
		*ProcessInfo = processInfo;
		status = STATUS_SUCCESS;
	}
	return status;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
NTSTATUS MappingUsbMemoryForMouse(
	_In_ void* 	 source,
	_In_ ULONG 	 length,
	_Out_ PVOID* mapped_user_address
)
{
	PEPROCESS proc = PsGetCurrentProcess();

	USERPROCESSINFO* processInfo = FindMappedUsbInfo(proc);

	if (!source ||
		!length)
	{
		USB_DEBUG_INFO_LN_EX("Mapped STATUS_INVALID_PARAMETER %x %x %x \r\n ", mapped_user_address, source, length);
		return STATUS_INVALID_PARAMETER;
	}

	if (!processInfo)
	{
		CreateProcessInfo(&processInfo, source, length, FALSE);
		if (!processInfo)
		{
			USB_DEBUG_INFO_LN_EX("Cannot Mapping Memory \r\n ");
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		if (!AddToChainListTail(g_ProcessInfo, processInfo))
		{
			ExFreePool(processInfo);
			processInfo = NULL;
			USB_DEBUG_INFO_LN_EX("Cannot Mapping Memory \r\n ");
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		USB_DEBUG_INFO_LN_EX("@@mouse mapped_user_address : %I64X \r\n ", processInfo->MappedMouseAddr);
	}

	if (!processInfo->MappedMouseAddr)
	{
		processInfo->MouseMdl = NULL;
		processInfo->MappedMouseAddr = MapNonpagedMemToSpace(source, length, &processInfo->MouseMdl, UserMode, 0);
	}

	*mapped_user_address = processInfo->MappedMouseAddr;

	return STATUS_SUCCESS;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
NTSTATUS MappingUsbMemoryForKeyboard(
	_In_  void* 	 source,
	_In_  ULONG 	 length,
	_Out_ PVOID* mapped_user_address
)
{
	PEPROCESS proc = PsGetCurrentProcess();

	USERPROCESSINFO* processInfo = FindMappedUsbInfo(proc);

	if (!source ||
		!length)
	{
		USB_DEBUG_INFO_LN_EX("Mapped STATUS_INVALID_PARAMETER %x %x %x \r\n ", mapped_user_address, source, length);
		return STATUS_INVALID_PARAMETER;
	}

	if (!processInfo)
	{
		CreateProcessInfo(&processInfo, source, length, TRUE);
		if (!processInfo)
		{
			USB_DEBUG_INFO_LN_EX("Cannot Mapping Memory \r\n ");
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		if (!AddToChainListTail(g_ProcessInfo, processInfo))
		{
			ExFreePool(processInfo);
			processInfo = NULL;
			USB_DEBUG_INFO_LN_EX("Cannot Mapping Memory \r\n ");
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		USB_DEBUG_INFO_LN_EX("@@keyboard mapped_user_address : %I64X \r\n ", processInfo->MappedKeyboardAddr);
	}

	if (!processInfo->MappedKeyboardAddr)
	{
		processInfo->KeyboardMdl = NULL;
		processInfo->MappedKeyboardAddr = MapNonpagedMemToSpace(source, length, &processInfo->KeyboardMdl, UserMode, 0);
	}

	*mapped_user_address = processInfo->MappedKeyboardAddr;

	return STATUS_SUCCESS;
}
//--------------------------------------------------------------------------------------------------------------------------------------------------
NTSTATUS MapUsbDataToUserAddressSpace(
	_In_  	USERCONFIGEX*     UserModeConfigEx,
	_In_    ULONG		      Size,
	_In_ 	BOOLEAN			  IsKeyboard
)
{

	PVOID user_mode_addr = NULL;

	if (!UserModeConfigEx)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (!g_bLoaded)
	{
		UserModeConfigEx->Version = 0;
		UserModeConfigEx->ProtocolVersion = 0;
		UserModeConfigEx->UserData = 0;
		UserModeConfigEx->UserDataLen = 0;
		return STATUS_UNSUCCESSFUL;
	}

	if (Size != sizeof(USERCONFIGEX))
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (!IsKeyboard)
	{
		if (!g_mou_data)
		{
			USB_DEBUG_INFO_LN_EX("MapUsbDataToUserAddressSpace UnSuccessfully");
			return STATUS_UNSUCCESSFUL;
		}
		if (NT_SUCCESS(MappingUsbMemoryForMouse(g_mou_data->Header, sizeof(CIRCULARBUFFER), &user_mode_addr)))
		{
			UserModeConfigEx->Version = USB_PEN_INIT_VERSION;
			UserModeConfigEx->ProtocolVersion = USB_PEN_PROTOCOL_VERSION;
			UserModeConfigEx->UserData = (ULONG64)user_mode_addr;
			UserModeConfigEx->UserDataLen = sizeof(user_mode_addr);
			return STATUS_SUCCESS;
		}
	}
	else
	{
		if (!g_kbd_data)
		{
			USB_DEBUG_INFO_LN_EX("MapUsbDataToUserAddressSpace UnSuccessfully");
			return STATUS_UNSUCCESSFUL;
		}

		if (NT_SUCCESS(MappingUsbMemoryForKeyboard(g_kbd_data->Header, sizeof(CIRCULARBUFFER), &user_mode_addr)))
		{
			UserModeConfigEx->Version = USB_PEN_INIT_VERSION;
			UserModeConfigEx->ProtocolVersion = USB_PEN_PROTOCOL_VERSION;
			UserModeConfigEx->UserData = (ULONG64)user_mode_addr;
			UserModeConfigEx->UserDataLen = sizeof(user_mode_addr);
			return STATUS_SUCCESS;
		}

	}
	USB_DEBUG_INFO_LN_EX("MapUsbDataToUserAddressSpace UnSuccessfully");

	return STATUS_UNSUCCESSFUL;
}
//-------------------------------------------------------------------------------------------------------------
ULONG   __fastcall ProcessListActionCallback(
	_In_ 	USERPROCESSINFO* Header,
	_In_ 	ULONG Act)
{
	if (Act == CLIST_ACTION_FREE)
	{
		if (!Header)
		{
			return 0;
		}

		if (Header->MouseMdl && Header->MappedMouseAddr)
		{
			MmUnmapLockedPages(Header->MappedMouseAddr, Header->MouseMdl);
			IoFreeMdl(Header->MouseMdl);
			Header->MouseMdl = NULL;
			Header->MappedMouseAddr = NULL;
		}

		if (Header->KeyboardMdl && Header->MappedKeyboardAddr)
		{
			MmUnmapLockedPages(Header->MappedKeyboardAddr, Header->KeyboardMdl);
			IoFreeMdl(Header->KeyboardMdl);
			Header->KeyboardMdl = NULL;
			Header->MappedKeyboardAddr = NULL;
		}

		Header->proc = NULL;
		ExFreePool(Header);
		Header = NULL;
	}
	return 0;
}
//-----------------------------------------------------------------------------------------//
ULONG   __fastcall DeleteProcessListCallback(
	_In_ USERPROCESSINFO* Header,
	_In_ PEPROCESS 		 proc
)
{
	if (Header)
	{
		if (Header->proc == proc)
		{
			USB_DEBUG_INFO_LN_EX("Found n delete Proc");
			return CLIST_FINDCB_DEL;
		}
	}
	return CLIST_FINDCB_CTN;
}

//----------------------------------------------------------------------------------------//
NTSTATUS OnProcessExitToUsbSystem(
	_In_ PEPROCESS eProcess
)
{
	if (g_ProcessInfo)
	{
		//call free_routine when delete a element
		QueryFromChainListByCallback(g_ProcessInfo, &DeleteProcessListCallback, eProcess);
	}
	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------//
NTSTATUS FreeProcessInfoList()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	//Free White List

	if (g_ProcessInfo)
	{
		CHAINLIST_SAFE_FREE(g_ProcessInfo);
	}

	status = STATUS_SUCCESS;
	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS UnInitialUsbHubIrpHook()
{
	ULONG i = 0;
	for (i = 0; i < g_Index; i++)
	{
		if (g_HookConfig[i].pendingList)
		{
			FreePendingList(g_HookConfig[i].pendingList);
			g_HookConfig[i].pendingList = NULL;
		}
	}
	g_Index = 0;
	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------//
ULONG __fastcall InitialUsbHubIrpHookCallback(
	_In_ USBHUBNODE* HubNode,
	_In_ PVOID Context
)
{
	IRPHOOKINFO* Info = NULL;
	PENDINGIRPLIST*  IrpPendingList = NULL;

	USB_DEBUG_INFO_LN_EX("g_Index: %d", g_Index);

	if (g_Index >= CONFIG_ARRAY_SIZE)
	{
		return CLIST_FINDCB_RET;
	}

	if (!HubNode)
	{
		return CLIST_FINDCB_CTN;
	}

	IrpPendingList = AllocateUsbHubsPendingList();

	if (!IrpPendingList)
	{
		return CLIST_FINDCB_CTN;
	}

	g_HookConfig[g_Index].pendingList = IrpPendingList;
	g_HookConfig[g_Index].DriverObject = HubNode->HubDriverObject;
	g_HookConfig[g_Index].OldRoutine = HubNode->HubDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL];

	DoIrpHook(g_HookConfig[g_Index].DriverObject, g_HookConfig[g_Index].IrpCode, g_HookConfig[g_Index].NewRoutine, Start);

	if (KeGetCurrentIrql() < DISPATCH_LEVEL)
	{
		USB_DEBUG_INFO_LN_EX("Index: %x IRPHook DriverObject: %I64x Name: %ws", g_Index, g_HookConfig[g_Index].DriverObject, HubNode->HubName);
	}
	else
	{
		USB_DEBUG_INFO_LN_EX("IRQL TOO HIGH ");
	}

	g_Index++;

	return 	CLIST_FINDCB_CTN;
}
//-----------------------------------------------------------------------------------------------//
NTSTATUS InitialUsbHubIrpHook()
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!g_UsbHubList)
	{
		USB_DEBUG_INFO_LN_EX("FATAL: Usb Hub List cannot be Found ! Quit Now");
		return status;
	}

	USB_DEBUG_INFO_LN_EX("g_UsbHubList->currentSize: %d ", g_UsbHubList->currentSize);

	QueryFromChainListByCallback(g_UsbHubList->head, &InitialUsbHubIrpHookCallback, NULL);

	if (g_Index == 0)
	{
		return status;
	}

	status = STATUS_SUCCESS;
	return status;
}
//---------------------------------------------------------------------------------------//
BOOLEAN AllocateKeyboardResource(PVOID Context)
{
	BOOLEAN ret = TRUE;
	if (!g_kbd_data)
	{
		g_kbd_data = NewOpenLoopBuffer(CB_SIZE, sizeof(USERKBDDATA), OPENLOOPBUFF_FALGS_PEASUDOHEADER);
		if (!g_kbd_data)
		{
			ret = FALSE;
		}
	}
	return ret;
}
//---------------------------------------------------------------------------------------//
BOOLEAN AllocateMouseResource(PVOID Context)
{
	BOOLEAN ret = TRUE;
	if (!g_mou_data)
	{
		g_mou_data = NewOpenLoopBuffer(CB_SIZE, sizeof(USERMOUDATA), OPENLOOPBUFF_FALGS_PEASUDOHEADER);
		if (!g_mou_data)
		{
			ret = FALSE;
		}
	}
	return ret;
}
//---------------------------------------------------------------------------------------//
NTSTATUS InitializeUserModeResource(
	_In_	ULONG RequiredDevice
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!g_ProcessInfo)
	{
		g_ProcessInfo = NewChainListHeaderEx(LISTFLAG_FTMUTEXLOCK | LISTFLAG_AUTOFREE, ProcessListActionCallback, 0);
		if (!g_ProcessInfo)
		{
			status = STATUS_UNSUCCESSFUL;
			return status;
		}
	}

	USB_DEBUG_INFO_LN_EX("g_mou_data: %I64x g_kbd_data: %I64x g_ProcessInfo : %I64x ", g_mou_data, g_kbd_data, g_ProcessInfo);
	status = STATUS_SUCCESS;
	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS InitializeHidPenetrate(
	_In_ ULONG RequiredDevice
)
{
	PDRIVER_OBJECT			  pHidDriverObj = NULL;
	NTSTATUS						 status = STATUS_UNSUCCESSFUL;
	ULONG						   ListSize = 0;

	if (g_bLoaded)
	{
		status = STATUS_SUCCESS;
		return status;
	}

	status = GetDriverObjectByName(HID_USB_DEVICE, &pHidDriverObj);
	if (!NT_SUCCESS(status) || !pHidDriverObj)
	{
		USB_DEBUG_INFO_LN_EX("GetUsbHub Error");
		return status;
	}

	status = InitializeUserModeResource(RequiredDevice);
	if (!NT_SUCCESS(status) || !g_ProcessInfo)
	{
		USB_DEBUG_INFO_LN_EX("InitializeUserModeResource Error");
		UnInitializeHidPenetrate();
		ReleaseDriverObject(pHidDriverObj);
		USB_DEBUG_INFO_LN_EX("InitHidSubSystem Error");
		return status;
	}

	g_HidContext.MouseCallback = AllocateMouseResource;
	g_HidContext.KeyboardCallback = AllocateKeyboardResource;
	g_HidContext.RequiredDevice = RequiredDevice;

	//Prepare HID PipeList AND UsbList
	status = InitHidSubSystem(&g_HidContext, &ListSize);
	if (!NT_SUCCESS(status) || !ListSize)
	{
		
		USB_DEBUG_INFO_LN_EX("InitHidSubSystem Error Size= %d", ListSize);
		UnInitializeHidPenetrate();
		ReleaseDriverObject(pHidDriverObj);
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	USB_DEBUG_INFO_LN_EX("HID SubSystem Initalization Successfully List size: %x ", ListSize);

	//Prepare IRP Hook 
	status = InitIrpHookSystem(g_IsCheckIrpHook);
	if (!NT_SUCCESS(status))
	{
		USB_DEBUG_INFO_LN_EX("InitIrpHookSystem Error");
		UnInitializeHidPenetrate();
		ReleaseDriverObject(pHidDriverObj);
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	USB_DEBUG_INFO_LN_EX("IRP Hook SubSystem Initalization Successfully");

	g_HidPnpHandler = (PDRIVER_DISPATCH)DoIrpHook(pHidDriverObj, IRP_MJ_PNP, HidUsbPnpIrpHandler, Start);
	if (!g_HidPnpHandler)
	{
		USB_DEBUG_INFO_LN_EX("DoIrpHook Error ");
		UnInitializeHidPenetrate();
		ReleaseDriverObject(pHidDriverObj);
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	USB_DEBUG_INFO_LN_EX("IRP Hook HidUsb PnP Successfully");

	status = InitialUsbHubIrpHook();
	if (!NT_SUCCESS(status))
	{
		USB_DEBUG_INFO_LN_EX("InitialUsbHubIrpHook Error");
		UnInitializeHidPenetrate();
		ReleaseDriverObject(pHidDriverObj);
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	USB_DEBUG_INFO_LN_EX("InitialUsbHubIrpHook Successfully");
	ReleaseDriverObject(pHidDriverObj);

	g_bLoaded = TRUE;

	USB_DEBUG_INFO_LN_EX("InitializeHidPenetrate Successfully");

	return status;
}
//----------------------------------------------------------------------------------------//
NTSTATUS FreeMappedLoopBuffer()
{
	PVOID TmpPointer = g_mou_data;
	PVOID TmpPointer2 = g_kbd_data;
	g_kbd_data = NULL;
	g_mou_data = NULL;

	if (TmpPointer)
	{
		OpenLoopBufferRelease(TmpPointer);
		TmpPointer = NULL;
	}


	if (TmpPointer2)
	{
		OpenLoopBufferRelease(TmpPointer2);
		TmpPointer2 = NULL;
	}

	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------//
NTSTATUS UnInitializeHidPenetrate()
{

	g_bUnloaded = TRUE;
	g_bLoaded = FALSE;

	//Recovery Irp all hook and Free the List
	UnInitIrpHookSystem();

	//Recovery Completion hook and Free all List 
	UnInitialUsbHubIrpHook();

	//Free the HID Client PDO pipe list 
	UnInitHidSubSystem();

	//Free Process Info List
	FreeProcessInfoList();

	//Free Loop Buffer
	FreeMappedLoopBuffer();
	USB_DEBUG_INFO_LN_EX("UnInitializeHidPenetrate");

	g_Index = 0;

	return STATUS_SUCCESS;
}
//----------------------------------------------------------------------------------------------------------------------------------------------
BOOLEAN IsReportHidRawDesc()
{
	return g_IsReportRawDesc;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
BOOLEAN IsReportHidMiniDrv()
{
	return g_IsEnableReportHidMiniDrv;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
NTSTATUS UsbPenerateConfigInit()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ntStatus = InitializeHidPenetrate(MOUSE_FLAGS  | KEYBOARD_FLAGS);
	return ntStatus;
}

//----------------------------------------------------------------------------------------------------------------------------------------------
BOOLEAN GetTpDataUsageStatus()
{
	return g_TpDataUsed[0];
}

