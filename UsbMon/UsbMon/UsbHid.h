#include <fltKernel.h> 
#include "local.h"

NTSTATUS
InitHidRelation(
	PHID_DEVICE_LIST* device_object_list,
	PULONG size
);

NTSTATUS 
FreeHidRelation();
 
VOID 
DumpChannel(
	PHIDP_COLLECTION_DESC collectionDesc, 
	HIDP_REPORT_TYPE type
);

VOID 
DumpReport(
	HIDP_DEVICE_DESC* report
);
