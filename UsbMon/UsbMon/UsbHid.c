                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #include "UsbHid.h"
#include "UsbUtil.h" 
#include "hidparser.h"
#define HIDP_MAX_UNKNOWN_ITEMS 4
struct _CHANNEL_REPORT_HEADER
{
	USHORT Offset;  // Position in the _CHANNEL_ITEM array
	USHORT Size;    // Length in said array
	USHORT Index;
	USHORT ByteLen; // The length of the data including reportID.
					// This is the longest such report that might be received
					// for the given collection.
};

typedef struct _HIDP_CHANNEL_DESC
{
	USHORT   UsagePage;
	UCHAR    ReportID;
	UCHAR    BitOffset;    // 0 to 8 value describing bit alignment

	ULONG    BitField;   // The 8 (plus extra) bits associated with a main item

	USHORT   ReportSize;   // HID defined report size
	USHORT   ReportCount;  // HID defined report count
	USHORT   ByteOffset;   // byte position of start of field in report packet
	USHORT   BitLength;    // total bit length of this channel

	USHORT   ByteEnd;      // First byte not containing bits of this channel.
	USHORT   LinkCollection;  // A unique internal index pointer
	USHORT   LinkUsagePage;
	USHORT   LinkUsage;

	ULONG    Units;
	ULONG    UnitExp;

	ULONG  MoreChannels : 1; // Are there more channel desc associated with
							 // this array.  This happens if there is a
							 // several usages for one main item.
	ULONG  IsConst : 1; // Does this channel represent filler
	ULONG  IsButton : 1; // Is this a channel of binary usages, not value usages.
	ULONG  IsAbsolute : 1; // As apposed to relative
	ULONG  IsRange : 1;
	ULONG  IsAlias : 1; // a usage described in a delimiter
	ULONG  IsStringRange : 1;
	ULONG  IsDesignatorRange : 1;
	ULONG  Reserved : 20;
	ULONG  NumGlobalUnknowns : 4;

	struct _HIDP_UNKNOWN_TOKEN GlobalUnknowns[HIDP_MAX_UNKNOWN_ITEMS];

	union {
		struct {
			USHORT   UsageMin, UsageMax;
			USHORT   StringMin, StringMax;
			USHORT   DesignatorMin, DesignatorMax;
			USHORT   DataIndexMin, DataIndexMax;
		} Range;
		struct {
			USHORT   Usage, Reserved1;
			USHORT   StringIndex, Reserved2;
			USHORT   DesignatorIndex, Reserved3;
			USHORT   DataIndex, Reserved4;
		} NotRange;
	};

	union {
		struct {
			LONG     LogicalMin, LogicalMax;
		} button;
		struct {
			BOOLEAN  HasNull;  // Does this channel have a null report
			UCHAR    Reserved[3];
			LONG     LogicalMin, LogicalMax;
			LONG     PhysicalMin, PhysicalMax;
		} Data;
	};
} HIDP_CHANNEL_DESC, *PHIDP_CHANNEL_DESC;
typedef struct _HIDP_SYS_POWER_INFO {
	ULONG   PowerButtonMask;

} HIDP_SYS_POWER_INFO, *PHIDP_SYS_POWER_INFO;
typedef struct _HIDP_PREPARSED_DATA
{
	LONG   Signature1, Signature2;
	USHORT Usage;
	USHORT UsagePage;

	HIDP_SYS_POWER_INFO;

	// The following channel report headers point to data within
	// the Data field below using array indices.
	struct _CHANNEL_REPORT_HEADER Input;
	struct _CHANNEL_REPORT_HEADER Output;
	struct _CHANNEL_REPORT_HEADER Feature;

	// After the CANNEL_DESC array the follows a LinkCollection array nodes.
	// LinkCollectionArrayOffset is the index given to RawBytes to find
	// the first location of the _HIDP_LINK_COLLECTION_NODE structure array
	// (index zero) and LinkCollectionArrayLength is the number of array
	// elements in that array.
	USHORT LinkCollectionArrayOffset;
	USHORT LinkCollectionArrayLength;

	union {
		HIDP_CHANNEL_DESC    Data[];
		UCHAR                RawBytes[];
	};
} HIDP_PREPARSED_DATA;

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Global/Extern Variable 
//// 
HID_DEVICE_LIST* g_hid_relation = NULL;

HIDP_DEVICE_DESC* g_hid_collection = NULL;

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Marco
//// 
#define HID_USB_DEVICE L"\\Driver\\hidusb"
#define ARRAY_SIZE					  100


/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
//// 

//-------------------------------------------------------------------------------------------//
HID_DEVICE_NODE* CreateHidDeviceNode(
	_In_ PDEVICE_OBJECT device_object, 
	_In_ HID_USB_DEVICE_EXTENSION* mini_extension
)
{
	if (!device_object || !mini_extension)
	{
		return NULL;
	}

	HID_DEVICE_NODE* node = ExAllocatePool(NonPagedPool, sizeof(HID_DEVICE_NODE));
	if (!node)
	{
		return NULL;
	}

	RtlZeroMemory(node, sizeof(HID_DEVICE_NODE));
	node->device_object  = device_object;
	node->mini_extension = mini_extension;

	return node;
}

/*
********************************************************************************
*  GetReportIdentifier
********************************************************************************
*
*
*/
PHIDP_REPORT_IDS GetReportIdentifier(FDO_EXTENSION *fdoExtension, ULONG reportId)
{
	PHIDP_REPORT_IDS result = NULL;
	PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
	ULONG i;

	if (deviceDesc->ReportIDs) {
		for (i = 0; i < deviceDesc->ReportIDsLength; i++) {
			if (deviceDesc->ReportIDs[i].ReportID == reportId) {
				result = &deviceDesc->ReportIDs[i];
				break;
			}
		}
	}

	if (fdoExtension->deviceSpecificFlags & DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION) {
		/*
		*  This call from HidpGetSetFeature can fail because we allow
		*  feature access on non-feature collections.
		*/
	}
	else {
		if (!result) {
			STACK_TRACE_DEBUG_INFO("Bad harware, returning NULL report ID");
		}
	}

	return result;
}

/*
********************************************************************************
*  GetHidclassCollection
********************************************************************************
*
*/
PHIDCLASS_COLLECTION GetHidclassCollection(FDO_EXTENSION *fdoExtension, ULONG collectionId)
{
	PHIDCLASS_COLLECTION result = NULL;
	PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
	ULONG i;
	 
		for (i = 0; i < deviceDesc->CollectionDescLength; i++) {
			if (fdoExtension->classCollectionArray[i].CollectionNumber == collectionId) {
				result = &fdoExtension->classCollectionArray[i];
				break;
			}
		} 
	ASSERT(result);
	return result;
}

/*
********************************************************************************
*  GetCollectionDesc
********************************************************************************
*
*
*/
PHIDP_COLLECTION_DESC GetCollectionDesc(FDO_EXTENSION *fdoExtension, ULONG collectionId)
{
	PHIDP_COLLECTION_DESC result = NULL;
	PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
	ULONG i;

	if (deviceDesc->CollectionDesc) {
		for (i = 0; i < fdoExtension->deviceDesc.CollectionDescLength; i++) {
			if (deviceDesc->CollectionDesc[i].CollectionNumber == collectionId) {
				result = &deviceDesc->CollectionDesc[i];
				break;
			}
		}
	}

	ASSERT(result);
	return result;
}
VOID UnitTest(PVOID raw_buf, ULONG size)
{
	static HIDParser hParser;
	static HIDData   hData;

	ResetParser(&hParser);
	hParser.ReportDescSize = size;
	memcpy(hParser.ReportDesc, raw_buf, size);
 
	int i = 0;
	while (HIDParse(&hParser, &hData))
	{
		STACK_TRACE_DEBUG_INFO("UPage: %d ", hParser.UPage);  
		STACK_TRACE_DEBUG_INFO("Count: %d ", hParser.Count);

	}
}
//---------------------------------------------------------------------------------------------------------//
BOOLEAN  IsKeyboardOrMouseDevice(
	_In_  PDEVICE_OBJECT				   device_object, 
	_Out_ PHID_USB_DEVICE_EXTENSION*   hid_mini_extension
)
{
	HID_USB_DEVICE_EXTENSION* mini_extension    = NULL;
	HIDCLASS_DEVICE_EXTENSION* hid_common_extension  = NULL;
	WCHAR						DeviceName[256] = { 0 };
	int i = 0;
	GetDeviceName(device_object, DeviceName);

 
	hid_common_extension = (HIDCLASS_DEVICE_EXTENSION*)device_object->DeviceExtension;
	if (!hid_common_extension)
	{
		return FALSE;
	}
 
	 
	//STACK_TRACE_DEBUG_INFO("Extension_common: %I64X sizeof: %x \r\n", hid_common_extension, sizeof(HID_USB_DEVICE_EXTENSION));
	mini_extension = (HID_USB_DEVICE_EXTENSION*)hid_common_extension->hidExt.MiniDeviceExtension;
	if (!mini_extension)
	{
		return FALSE;
	} 	
	

 
	//DumpHidMiniDriverExtension(hid_common_extension); 

	if (!hid_common_extension->isClientPdo)
	{	
		*hid_mini_extension = NULL;
		return FALSE; 
	}
	 
	if ( mini_extension->InterfaceDesc->Class == 3 &&			//HidClass Device
		(mini_extension->InterfaceDesc->Protocol == 1 ||		//Keyboard
		 mini_extension->InterfaceDesc->Protocol == 2 ||
		 mini_extension->InterfaceDesc->Protocol == 0) )			//Mouse
	{

	//	STACK_TRACE_DEBUG_INFO("Signature: %I64x \r\n", hid_common_extension->Signature);
	//	STACK_TRACE_DEBUG_INFO("DescLen: %x \r\n", hid_common_extension->fdoExt.rawReportDescriptionLength); 
		FDO_EXTENSION       *fdoExt = NULL;
		PDO_EXTENSION       *pdoExt = NULL;

		if (hid_common_extension->isClientPdo)
		{
			STACK_TRACE_DEBUG_INFO("---------------------------------------------------------------------------------------------------- \r\n");

			STACK_TRACE_DEBUG_INFO("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws \r\n", device_object, device_object->DriverObject->DriverName.Buffer, DeviceName);

			pdoExt = &hid_common_extension->pdoExt; 
			STACK_TRACE_DEBUG_INFO("sizeof FDO_EXTENSION : %X  \r\n", sizeof(FDO_EXTENSION));

			STACK_TRACE_DEBUG_INFO("hid_common_extension: %I64x \r\n", hid_common_extension);
			STACK_TRACE_DEBUG_INFO("pdoExt: %I64x \r\n", pdoExt); 
			STACK_TRACE_DEBUG_INFO("OFFSET %x \r\n", FIELD_OFFSET(PDO_EXTENSION,deviceFdoExt));
			STACK_TRACE_DEBUG_INFO("pdoExt Offset: %I64x \r\n", &hid_common_extension->pdoExt.deviceFdoExt);
			STACK_TRACE_DEBUG_INFO("pdoExt Offset: %I64x \r\n", ((ULONG64)hid_common_extension) + 0x60);		//For Win7 deviceFdoExt offset by HIDCLASS_DEVICE_EXTENSION->PDO_EXTENSION.backptr
		
			WCHAR name[256] = { 0 };
			HIDCLASS_DEVICE_EXTENSION* addr = (HIDCLASS_DEVICE_EXTENSION*)pdoExt->deviceFdoExt;
			fdoExt = &addr->fdoExt;
			GetDeviceName(fdoExt->fdo, name);
			STACK_TRACE_DEBUG_INFO("Pdo->fdoExt: %I64x \r\n", fdoExt);
			STACK_TRACE_DEBUG_INFO("Pdo->fdo: %I64x \r\n", fdoExt->fdo);
			STACK_TRACE_DEBUG_INFO("Pdo->fdo DriverName: %ws \r\n", fdoExt->fdo->DriverObject->DriverName.Buffer);
			STACK_TRACE_DEBUG_INFO("Pdo->fdo DeviceName: %ws \r\n", name); 
 			STACK_TRACE_DEBUG_INFO("Pdo->CollectionIndex: %I64x \r\n", pdoExt->collectionIndex);
			STACK_TRACE_DEBUG_INFO("Pdo->CollectionNum: %I64x \r\n", pdoExt->collectionNum);

			HIDP_DEVICE_DESC* tmp = (HIDP_DEVICE_DESC*)((PUCHAR)fdoExt + 0x58);
		 
///			STACK_TRACE_DEBUG_INFO("deviceDesc(FIELD_OFFSET): %x \r\n", FIELD_OFFSET(FDO_EXTENSION, deviceDesc));  

///			STACK_TRACE_DEBUG_INFO("(PUCHAR)fdoExt: %I64x \r\n", (PUCHAR)fdoExt + 0x58);
///			STACK_TRACE_DEBUG_INFO("(PUCHAR)fdoExt: %I64x \r\n",  &fdoExt->deviceDesc);

			UnitTest(fdoExt->rawReportDescription, fdoExt->rawReportDescriptionLength);
#define HIDP_PREPARSED_DATA_SIGNATURE1 'PdiH'
#define HIDP_PREPARSED_DATA_SIGNATURE2 'RDK '
			 
			PHIDP_COLLECTION_DESC collectionDesc = &tmp->CollectionDesc[0];
			for (int i = 0; i < tmp->CollectionDescLength; i++)
			{	
				STACK_TRACE_DEBUG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
				STACK_TRACE_DEBUG_INFO("[Collection] Usage: %xh \r\n", collectionDesc->Usage); 
				STACK_TRACE_DEBUG_INFO("[Collection] UsagePage: %xh \r\n", collectionDesc->UsagePage);
				STACK_TRACE_DEBUG_INFO("[Collection] InputLength: %xh \r\n", collectionDesc->InputLength);
				STACK_TRACE_DEBUG_INFO("[Collection] OutputLength: %xh \r\n", collectionDesc->OutputLength);
				STACK_TRACE_DEBUG_INFO("[Collection] FeatureLength: %xh \r\n", collectionDesc->FeatureLength);
				STACK_TRACE_DEBUG_INFO("[Collection] CollectionNumber: %x \r\n", collectionDesc->CollectionNumber); 
				STACK_TRACE_DEBUG_INFO("[Collection] PreparsedData: %I64x \r\n", collectionDesc->PreparsedData); 
				if (collectionDesc->PreparsedData->Signature1 == HIDP_PREPARSED_DATA_SIGNATURE1)
				{
					STACK_TRACE_DEBUG_INFO("[Collection] Signature1 correct \r\n");
				}
				if (collectionDesc->PreparsedData->Signature2 == HIDP_PREPARSED_DATA_SIGNATURE2)
				{
					STACK_TRACE_DEBUG_INFO("[Collection] Signature2 correct \r\n");
				}
				STACK_TRACE_DEBUG_INFO("[Collection] Input Offset: %x  Index: %x \r\n", collectionDesc->PreparsedData->Input.Offset, collectionDesc->PreparsedData->Input.Index);
				STACK_TRACE_DEBUG_INFO("[Collection] Output Offset: %x Index: %x \r\n", collectionDesc->PreparsedData->Output.Offset, collectionDesc->PreparsedData->Output.Index);
				STACK_TRACE_DEBUG_INFO("[Collection] Feature Offset: %x Index: %x \r\n", collectionDesc->PreparsedData->Feature.Offset, collectionDesc->PreparsedData->Feature.Index);
				  
				HIDP_CHANNEL_DESC*            channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Input.Offset];
				for (int k = collectionDesc->PreparsedData->Input.Offset; k < collectionDesc->PreparsedData->Input.Index ; k++) 
				{
					STACK_TRACE_DEBUG_INFO("$$$$$$$$$$$$$$$$$$$Input$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\r\n");
					STACK_TRACE_DEBUG_INFO("channel ReportID: %d \r\n", channel->ReportID);
					STACK_TRACE_DEBUG_INFO("channel ReportSize: %d \r\n", channel->ReportSize);
					STACK_TRACE_DEBUG_INFO("channel ReportCount: %d \r\n", channel->ReportCount);					
					STACK_TRACE_DEBUG_INFO("channel IsButton: %x \r\n", channel->IsButton);
					STACK_TRACE_DEBUG_INFO("channel IsAbsolute: %x \r\n", channel->IsAbsolute);
					STACK_TRACE_DEBUG_INFO("channel IsRange: %x \r\n", channel->IsRange);
					STACK_TRACE_DEBUG_INFO("channel IsConst: %x \r\n", channel->IsConst);
					STACK_TRACE_DEBUG_INFO("channel IsAlias: %x \r\n", channel->IsAlias);
					STACK_TRACE_DEBUG_INFO("channel IsDesignatorRange: %x \r\n", channel->IsDesignatorRange);
					STACK_TRACE_DEBUG_INFO("channel IsStringRange: %x \r\n", channel->IsStringRange); 
					STACK_TRACE_DEBUG_INFO("channel ByteOffset: %x \r\n", channel->ByteOffset);
					STACK_TRACE_DEBUG_INFO("channel ByteEnd: %x \r\n", channel->ByteEnd); 
					STACK_TRACE_DEBUG_INFO("channel BitOffset: %x \r\n", channel->BitOffset); 
 					STACK_TRACE_DEBUG_INFO("channel UsagePage: %x \r\n", channel->UsagePage); 
					STACK_TRACE_DEBUG_INFO("channel LinkCollection: %x \r\n", channel->LinkCollection); 
					STACK_TRACE_DEBUG_INFO("channel LinkUsage: %x \r\n", channel->LinkUsage);
					STACK_TRACE_DEBUG_INFO("channel LinkUsagePage: %x \r\n", channel->LinkUsagePage);  
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.DataIndexMax);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.DataIndexMin);	//Logical M
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.DesignatorMax);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.DesignatorMin);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.StringMax);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.StringMin);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.UsageMin);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.UsageMax);
					 
					STACK_TRACE_DEBUG_INFO("channel Data: %x \r\n", channel->Data.LogicalMax);
					STACK_TRACE_DEBUG_INFO("channel Data: %x \r\n", channel->Data.LogicalMin);
					STACK_TRACE_DEBUG_INFO("channel Data: %x \r\n", channel->Data.PhysicalMax);
					STACK_TRACE_DEBUG_INFO("channel Data: %x \r\n", channel->Data.PhysicalMin);

					STACK_TRACE_DEBUG_INFO("$$$$$$$$$$$$$$$$$$$Input$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\r\n");
					channel++;
				}

			    channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Output.Offset];
				for (int k = collectionDesc->PreparsedData->Output.Offset; k < collectionDesc->PreparsedData->Output.Index; k++)
				{
					STACK_TRACE_DEBUG_INFO("$$$$$$$$$$$$$$$$$$$$$Output$$$$$$$$$$$$$$$$$$$$$$$$$$$\r\n");
					STACK_TRACE_DEBUG_INFO("channel ReportID: %d \r\n", channel->ReportID);
					STACK_TRACE_DEBUG_INFO("channel ReportSize: %d \r\n", channel->ReportSize);
					STACK_TRACE_DEBUG_INFO("channel ReportCount: %d \r\n", channel->ReportCount);
					STACK_TRACE_DEBUG_INFO("channel IsButton: %x \r\n", channel->IsButton);
					STACK_TRACE_DEBUG_INFO("channel IsAbsolute: %x \r\n", channel->IsAbsolute);
					STACK_TRACE_DEBUG_INFO("channel IsRange: %x \r\n", channel->IsRange);
					STACK_TRACE_DEBUG_INFO("channel IsConst: %x \r\n", channel->IsConst);
					STACK_TRACE_DEBUG_INFO("channel IsAlias: %x \r\n", channel->IsAlias);
					STACK_TRACE_DEBUG_INFO("channel IsDesignatorRange: %x \r\n", channel->IsDesignatorRange);
					STACK_TRACE_DEBUG_INFO("channel IsStringRange: %x \r\n", channel->IsStringRange);  
					STACK_TRACE_DEBUG_INFO("channel ByteOffset: %x \r\n", channel->ByteOffset);
					STACK_TRACE_DEBUG_INFO("channel BitOffset: %x \r\n", channel->BitOffset);
					STACK_TRACE_DEBUG_INFO("channel UsagePage: %x \r\n", channel->UsagePage);			
					STACK_TRACE_DEBUG_INFO("channel LinkCollection: %x \r\n", channel->LinkCollection); 
					STACK_TRACE_DEBUG_INFO("channel LinkUsage: %x \r\n", channel->LinkUsage);
					STACK_TRACE_DEBUG_INFO("channel LinkUsagePage: %x \r\n", channel->LinkUsagePage);

					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.DataIndexMax);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.DataIndexMin);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.DesignatorMax);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.DesignatorMin);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.StringMax);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.StringMin);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.UsageMin);
					STACK_TRACE_DEBUG_INFO("channel Range: %x \r\n", channel->Range.UsageMax);

					STACK_TRACE_DEBUG_INFO("channel Data: %x \r\n", channel->Data.LogicalMax);
					STACK_TRACE_DEBUG_INFO("channel Data: %x \r\n", channel->Data.LogicalMin);
					STACK_TRACE_DEBUG_INFO("channel Data: %x \r\n", channel->Data.PhysicalMax);
					STACK_TRACE_DEBUG_INFO("channel Data: %x \r\n", channel->Data.PhysicalMin);
					STACK_TRACE_DEBUG_INFO("$$$$$$$$$$$$$$$$$$$$$Output$$$$$$$$$$$$$$$$$$$$$$$$$$$\r\n");
					channel++;
				}
			 
			 	STACK_TRACE_DEBUG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n"); 
				collectionDesc++;
			}

			PHIDP_REPORT_IDS      reportDesc = &tmp->ReportIDs[0];
			for (int i = 0; i < tmp->ReportIDsLength ; i++)
			{ 
				 STACK_TRACE_DEBUG_INFO("*******************************************************\r\n");
				 STACK_TRACE_DEBUG_INFO("[Report] ReportID: %xh \r\n", reportDesc->ReportID);
				 STACK_TRACE_DEBUG_INFO("[Report] InputLength: %xh \r\n", reportDesc->InputLength);
				 STACK_TRACE_DEBUG_INFO("[Report] OutputLength: %xh \r\n", reportDesc->OutputLength);
				 STACK_TRACE_DEBUG_INFO("[Report] FeatureLength: %xh \r\n", reportDesc->FeatureLength);
				 STACK_TRACE_DEBUG_INFO("[Report] CollectionNumber: %x \r\n", reportDesc->CollectionNumber);
				 STACK_TRACE_DEBUG_INFO("*******************************************************\r\n"); 
				reportDesc++;
			}

			STACK_TRACE_DEBUG_INFO("---------------------------------------------------------------------------------------------------- \r\n");

		}
		*hid_mini_extension = mini_extension;
		return TRUE;
	}
 
	return FALSE;
}
//---------------------------------------------------------------------------------------------------------//
NTSTATUS FreeHidRelation()
{
	//Free White List
	if (g_hid_relation)
	{
		if (g_hid_relation->head)
		{
			CHAINLIST_SAFE_FREE(g_hid_relation->head);
		}
		ExFreePool(g_hid_relation);
		g_hid_relation = NULL;
	}
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS ReleaseHidRelation()
{
	NTSTATUS status = STATUS_SUCCESS;
	if (!g_hid_relation)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}
	InterlockedIncrement64(&g_hid_relation->RefCount);
	return status;
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS AcquireHidRelation(
	_Out_ PHID_DEVICE_LIST* device_object_list,
	_Out_ PULONG size
)
{
	*device_object_list = g_hid_relation;
	*size = g_hid_relation->currentSize;
	InterlockedIncrement64(&g_hid_relation->RefCount);
}
//----------------------------------------------------------------------------------------------------------//
NTSTATUS InitHidRelation(
	_Out_ PHID_DEVICE_LIST* device_object_list,
	_Out_ PULONG size
)
{
	PDEVICE_OBJECT	 device_object = NULL;
	PDRIVER_OBJECT	    pDriverObj = NULL;
	ULONG			  current_size = 0;
	
	
	if (!g_hid_relation)
	{
		g_hid_relation = (HID_DEVICE_LIST*)ExAllocatePoolWithTag(NonPagedPool, sizeof(HID_DEVICE_LIST), 'ldih');
	}

	if (!g_hid_relation)
	{
		return STATUS_UNSUCCESSFUL;
	} 

	RtlZeroMemory(g_hid_relation, sizeof(HID_DEVICE_LIST));

	g_hid_relation->head = NewChainListHeaderEx(LISTFLAG_SPINLOCK | LISTFLAG_AUTOFREE, NULL, 0);

	if (!g_hid_relation->head)
	{
		return STATUS_UNSUCCESSFUL;
	}

	GetDriverObjectByName(HID_USB_DEVICE, &pDriverObj);
	if (!pDriverObj)
	{
		FreeHidRelation();
		return STATUS_UNSUCCESSFUL;
	}

	STACK_TRACE_DEBUG_INFO("DriverObj: %I64X \r\n", pDriverObj);

	device_object = pDriverObj->DeviceObject;
	while (device_object)
	{
		HID_USB_DEVICE_EXTENSION* mini_extension = NULL; 
		if (IsKeyboardOrMouseDevice(device_object, &mini_extension))
		{
			if (mini_extension && g_hid_relation)
			{
				HID_DEVICE_NODE* node = CreateHidDeviceNode(device_object, mini_extension);
				AddToChainListTail(g_hid_relation->head, node);
				current_size++;
				STACK_TRACE_DEBUG_INFO("Inserted one element: %I64x InferfaceDesc: %I64X device_object: %I64x \r\n", node->device_object, node->mini_extension, device_object);
				STACK_TRACE_DEBUG_INFO("Added one element :%x \r\n", current_size);
			}
		}
 		device_object = device_object->NextDevice;
	}

	if (current_size > 0 && g_hid_relation)
	{
		*device_object_list = g_hid_relation;
		*size = current_size;
		g_hid_relation->RefCount = 1;
		return STATUS_SUCCESS;
	}  

	FreeHidRelation();
	return STATUS_UNSUCCESSFUL;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           