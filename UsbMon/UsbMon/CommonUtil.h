#ifndef __UTIL_HEADER__
#define __UTIL_HEADER__ 
   

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//// Marco
//// 
#ifndef _In_
#define _In_
#endif // !
#ifndef _Out_
#define _In_
#endif // !
#ifndef _Out_opt_
#define _Out_opt_
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
#define STACK_TRACE_COMMON_DBG_BREAK() \
  if (KD_DEBUGGER_NOT_PRESENT) {         \
  } else {                               \
    __debugbreak();                      \
  }                                      \
  (void*)(0)
#endif

#define STACK_TRACE_DEBUG_INFO(format, ...) DbgPrintEx(0,0,format,__VA_ARGS__)

#define DELAY_ONE_MICROSECOND 	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 

NTSTATUS KeSleep(
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