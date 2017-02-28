#ifndef __UTIL_HEADER__
#define __UTIL_HEADER__ 
   
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

NTSTATUS KeSleep(
	LONG msec
);

NTSTATUS GetDriverObjectByName(
	WCHAR* name, 
	PDRIVER_OBJECT* pDriverObj
);

NTSTATUS GetDeviceName(
	PDEVICE_OBJECT device_object, 
	WCHAR* DeviceNameBuf
);
  

#endif