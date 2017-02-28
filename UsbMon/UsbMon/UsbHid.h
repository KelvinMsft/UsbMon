#include <fltKernel.h>
#include "UsbType.h"

NTSTATUS
SearchAllHidRelation(
	PHID_DEVICE_NODE** device_object_list,
	PULONG size
);


NTSTATUS SearchAllUsbCcgpRelation(
	PHID_DEVICE_NODE** device_object_list,
	PULONG size
);
