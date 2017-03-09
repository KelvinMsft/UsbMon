#include <fltKernel.h> 
#include "local.h"

NTSTATUS
InitHidRelation(
	PHID_DEVICE_LIST* device_object_list,
	PULONG size
);

NTSTATUS 
FreeHidRelation();
 

VOID UnitTest(
	HIDCLASS_DEVICE_EXTENSION* hid_common_extension
);