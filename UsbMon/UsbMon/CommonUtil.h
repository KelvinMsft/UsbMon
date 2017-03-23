#ifndef __UTIL_HEADER__
#define __UTIL_HEADER__ 

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//// Type
//// 
typedef ULONG	 LOOKUP_STATUS;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//// Marco
//// 
#ifndef _In_
#define _In_
#endif // !
#ifndef _Out_
#define _Out_
#endif // !
#ifndef _Out_opt_
#define _Out_opt_
#endif // !
#ifndef _Inout_
#define _Inout_
#endif // !
#ifndef _Inout_opt_
#define _Inout_opt_
#endif // !
#ifndef _In_opt_
#define _In_opt_
#endif // !



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//// Marco Utility
//// 
/// Sets a break point that works only when a debugger is present
#if !defined(HYPERPLATFORM_COMMON_DBG_BREAK)
#define USB_MON_COMMON_DBG_BREAK() \
  if (KD_DEBUGGER_NOT_PRESENT) {         \
  } else {                               \
    __debugbreak();                      \
  }                                      \
  (void*)(0)
#endif




#define USB_NATIVE_DEBUG_INFO(format, ...)	DbgPrintEx(0,0,format,__VA_ARGS__) 

#define USB_COMMON_DEBUG_INFO(format, ...)	USB_NATIVE_DEBUG_INFO("[%s] => [%d] : "format, __FILE__ ,  __LINE__,  __VA_ARGS__)
#define USB_DEBUG_INFO_LN()					USB_NATIVE_DEBUG_INFO("\r\n")
#define USB_DEBUG_INFO_LN_EX(format, ...)	USB_COMMON_DEBUG_INFO(format"\r\n", __VA_ARGS__)

#define DELAY_ONE_MICROSECOND 	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)

#define DUMP_DEVICE_NAME(DeviceObject) \
		WCHAR DeviceName[256] = {0};		\
		GetDeviceName(DeviceObject, DeviceName);	\
		USB_DEBUG_INFO_LN_EX("DriverName: %ws DeviceName: %ws Flags: %x", DeviceObject->DriverObject->DriverName.Buffer, DeviceName, UrbGetTransferFlags(pContext->urb));	\

#define UNREFERENCED_PARAMETER(P)          (P)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 

NTSTATUS KernelSleep(
	_In_ LONG msec
);

NTSTATUS GetDriverObjectByName(
	_In_  WCHAR* name, 
	_Out_ PDRIVER_OBJECT* pDriverObj
);

NTSTATUS GetDeviceName(
	_In_  PDEVICE_OBJECT device_object, 
	_Out_ WCHAR* DeviceNameBuf
);
  

#endif