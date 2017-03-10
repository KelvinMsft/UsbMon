                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    #include "UsbHid.h"
#include "UsbUtil.h" 
#include "WinParse.h"
#define HIDP_MAX_UNKNOWN_ITEMS 4
/*
struct _CHANNEL_REPORT_HEADER
{
	USHORT Offset;  // Position in the _CHANNEL_ITEM array
	USHORT Size;    // Length in said array
	USHORT Index;
	USHORT ByteLen; // The length of the data including reportID.
					// This is the longest such report that might be received
					// for the given collection.
};
/*
typedef struct _HIDP_CHANNEL_DESC
{
	USHORT   UsagePage;		  //+0x0
	UCHAR    ReportID;		  //+0x2
	UCHAR    BitOffset;		  // 0 to 8 value describing bit alignment	+0x3
	USHORT   ReportSize;	  // HID defined report size	+0x4
	USHORT   ReportCount;	  // HID defined report count	+0x6
	USHORT   ByteOffset;	  // byte position of start of field in report packet +0x8
	USHORT   BitLength;		  // total bit length of this channel	+0xA
	ULONG    BitField;		  // The 8 (plus extra) bits associated with a main item +0xC
	USHORT   ByteEnd;		  // First byte not containing bits of this channel. +0x10
	USHORT   LinkCollection;  // A unique internal index pointer	+0x12
	USHORT   LinkUsagePage;   // +0x14
	USHORT   LinkUsage;		  // +0x16
	union
	{
		struct
		{
			ULONG  MoreChannels : 1; // Are there more channel desc associated with
									 // this array.  This happens if there is a
									 // several usages for one main item.
			ULONG  IsConst : 1;		// Does this channel represent filler
			ULONG  IsButton : 1;	// Is this a channel of binary usages, not value usages.
			ULONG  IsAbsolute : 1;  // As apposed to relative
			ULONG  IsRange : 1;
			ULONG  IsAlias : 1;		// a usage described in a delimiter
			ULONG  IsStringRange : 1;
			ULONG  IsDesignatorRange : 1;
			ULONG  Reserved : 20;
			ULONG  NumGlobalUnknowns : 4;
		};
		ULONG  all;
	};
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

	ULONG Units;
	ULONG UnitExp;
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
*/
 
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
#define HIDP_PREPARSED_DATA_SIGNATURE1 'PdiH'
#define HIDP_PREPARSED_DATA_SIGNATURE2 'RDK '
 
//usage page-id
#define HID_GENERIC_DESKTOP_PAGE 0x1

//There is a sub-usage id
#define HID_POINTER_USAGE	       0x1
#define HID_MOU_USAGE		       0x2
#define HID_RESERVED_USAGE	       0x3
#define HID_JOYSTICK_USAGE	       0x4
#define HID_GAMEPAD_USAGE	       0x5  
#define HID_KBD_USAGE		       0x6
#define HID_KBYPAD_USAGE	       0x7
#define HID_LED_USAGE		       0x8
#define HID_MULTI_AXIS_USAGE	   0x9 

//0xA-0x2f reserved

#define HID_NOT_RANGE_USAGE_X	  0x30
#define HID_NOT_RANGE_USAGE_Y	  0x31
#define HID_NOT_RANGE_USAGE_Z	  0x32 

#define HID_NOT_RANGE_USAGE_RX	  0x33
#define HID_NOT_RANGE_USAGE_RY	  0x34
#define HID_NOT_RANGE_USAGE_RZ	  0x35 

#define HID_USAGE_SLIDER		  0x36 
#define HID_USAGE_DIAL			  0x37 
#define HID_NOT_RANGE_USAGE_WHELL 0x38

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Prototype
//// 

/////////////////////////////////////////////////////////////////////////////////////////////// 
//// Implementation
////  


//----------------------------------------------------------------------------------------------------------//
HIDP_DEVICE_DESC* GetReport(HIDCLASS_DEVICE_EXTENSION* hid_common_extension)
{
	PDO_EXTENSION* pdoExt = NULL;
	FDO_EXTENSION* fdoExt = NULL; 

	if (!hid_common_extension)
	{
		return NULL;
	}
 
	pdoExt = &hid_common_extension->pdoExt;

	USB_MON_DEBUG_INFO("sizeof FDO_EXTENSION : %X  \r\n", sizeof(FDO_EXTENSION));
	USB_MON_DEBUG_INFO("hid_common_extension: %I64x \r\n", hid_common_extension);
	USB_MON_DEBUG_INFO("pdoExt: %I64x \r\n", pdoExt);
	USB_MON_DEBUG_INFO("OFFSET %x \r\n", FIELD_OFFSET(PDO_EXTENSION, deviceFdoExt));
	USB_MON_DEBUG_INFO("pdoExt Offset: %I64x \r\n", &hid_common_extension->pdoExt.deviceFdoExt);
	USB_MON_DEBUG_INFO("pdoExt Offset: %I64x \r\n", ((ULONG64)hid_common_extension) + 0x60);		//For Win7 deviceFdoExt offset by HIDCLASS_DEVICE_EXTENSION->PDO_EXTENSION.backptr
	
	WCHAR name[256] = { 0 };
	HIDCLASS_DEVICE_EXTENSION* addr = (HIDCLASS_DEVICE_EXTENSION*)pdoExt->deviceFdoExt;
	fdoExt = &addr->fdoExt;
	GetDeviceName(fdoExt->fdo, name);
	USB_MON_DEBUG_INFO("Pdo->fdoExt: %I64x \r\n", fdoExt);
	USB_MON_DEBUG_INFO("Pdo->fdo: %I64x \r\n", fdoExt->fdo);
	USB_MON_DEBUG_INFO("Pdo->fdo DriverName: %ws \r\n", fdoExt->fdo->DriverObject->DriverName.Buffer);
	USB_MON_DEBUG_INFO("Pdo->fdo DeviceName: %ws \r\n", name);
	USB_MON_DEBUG_INFO("Pdo->CollectionIndex: %I64x \r\n", pdoExt->collectionIndex);
	USB_MON_DEBUG_INFO("Pdo->CollectionNum: %I64x \r\n", pdoExt->collectionNum);
	// = (HIDP_DEVICE_DESC*)((PUCHAR)fdoExt + 0x58);
	HIDP_DEVICE_DESC* tmp = ExAllocatePoolWithTag(NonPagedPool, sizeof(HIDP_DEVICE_DESC), 'pdih'); // = (HIDP_DEVICE_DESC*)((PUCHAR)fdoExt + 0x58);
	if (!tmp)
	{
		return NULL;
	}
	RtlZeroMemory(tmp, sizeof(HIDP_DEVICE_DESC));

	USB_MON_DEBUG_INFO("[rawReportDescription] %I64x rawReportDescriptionLength: %xh \r\n", fdoExt->rawReportDescription, fdoExt->rawReportDescriptionLength);

	MyGetCollectionDescription(fdoExt->rawReportDescription, fdoExt->rawReportDescriptionLength, NonPagedPool, tmp);	 
	DumpReport(tmp);
	return tmp;
}
//----------------------------------------------------------------------------------------------------------//
VOID DumpReport(HIDP_DEVICE_DESC* report)
{
	PHIDP_COLLECTION_DESC collectionDesc = &report->CollectionDesc[0];
	for (int i = 0; i < report->CollectionDescLength; i++)
	{
		ASSERT(collectionDesc->PreparsedData->Signature1 == HIDP_PREPARSED_DATA_SIGNATURE1);
		ASSERT(collectionDesc->PreparsedData->Signature2 == HIDP_PREPARSED_DATA_SIGNATURE2);

		USB_MON_DEBUG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
		USB_MON_DEBUG_INFO("[Collection] Usage: %xh \r\n", collectionDesc->Usage);
		USB_MON_DEBUG_INFO("[Collection] UsagePage: %xh \r\n", collectionDesc->UsagePage);
		USB_MON_DEBUG_INFO("[Collection] InputLength: %xh \r\n", collectionDesc->InputLength);
		USB_MON_DEBUG_INFO("[Collection] OutputLength: %xh \r\n", collectionDesc->OutputLength);
		USB_MON_DEBUG_INFO("[Collection] FeatureLength: %xh \r\n", collectionDesc->FeatureLength);
		USB_MON_DEBUG_INFO("[Collection] CollectionNumber: %x \r\n", collectionDesc->CollectionNumber);
		USB_MON_DEBUG_INFO("[Collection] PreparsedData: %I64x \r\n", collectionDesc->PreparsedData);

		USB_MON_DEBUG_INFO("[Collection] Input Offset: %x  Index: %x \r\n", collectionDesc->PreparsedData->Input.Offset, collectionDesc->PreparsedData->Input.Index);
		USB_MON_DEBUG_INFO("[Collection] Output Offset: %x Index: %x \r\n", collectionDesc->PreparsedData->Output.Offset, collectionDesc->PreparsedData->Output.Index);
		USB_MON_DEBUG_INFO("[Collection] Feature Offset: %x Index: %x \r\n", collectionDesc->PreparsedData->Feature.Offset, collectionDesc->PreparsedData->Feature.Index);


		DumpChannel(collectionDesc, HidP_Input, CHANNEL_DUMP_ALL);
		DumpChannel(collectionDesc, HidP_Output, CHANNEL_DUMP_ALL);
		DumpChannel(collectionDesc, HidP_Feature, CHANNEL_DUMP_ALL);

		USB_MON_DEBUG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
		collectionDesc++;
	}

	PHIDP_REPORT_IDS      reportDesc = &report->ReportIDs[0];
	for (int i = 0; i < report->ReportIDsLength; i++)
	{
		USB_MON_DEBUG_INFO("*******************************************************\r\n");
		USB_MON_DEBUG_INFO("[Report] ReportID: %xh \r\n", reportDesc->ReportID);
		USB_MON_DEBUG_INFO("[Report] InputLength: %xh \r\n", reportDesc->InputLength);
		USB_MON_DEBUG_INFO("[Report] OutputLength: %xh \r\n", reportDesc->OutputLength);
		USB_MON_DEBUG_INFO("[Report] FeatureLength: %xh \r\n", reportDesc->FeatureLength);
		USB_MON_DEBUG_INFO("[Report] CollectionNumber: %x \r\n", reportDesc->CollectionNumber);
		USB_MON_DEBUG_INFO("*******************************************************\r\n");
		reportDesc++;
	}
}
NTSTATUS ExtractDataFromChannel(PHIDP_COLLECTION_DESC collectionDesc, HIDP_REPORT_TYPE type, EXTRACTDATA* ExtractedData)
{
	HIDP_CHANNEL_DESC*            channel = NULL;
	ULONG start = 0;
	ULONG end = 0;
	CHAR* reportType = NULL;
	switch (type)
	{
	case HidP_Input:
		channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Input.Offset];
		start = collectionDesc->PreparsedData->Input.Offset;
		end = collectionDesc->PreparsedData->Input.Index;
		reportType = "Input Report";
		break;
	case HidP_Output:
		channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Output.Offset];
		start = collectionDesc->PreparsedData->Output.Offset;
		end = collectionDesc->PreparsedData->Output.Index;
		reportType = "Output Report";
		break;
	case HidP_Feature:
		channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Feature.Offset];
		start = collectionDesc->PreparsedData->Feature.Offset;
		end = collectionDesc->PreparsedData->Feature.Index;
		reportType = "Feature Report";
		break;
	default:
		break;
	}
	USB_MON_DEBUG_INFO("Start: %x End: %x ReportType: %s \r\n", start, end, reportType);
	for (int k = start; k < end; k++)
	{
		//Root Collection
		if(channel->LinkUsagePage == 0x1)
		switch(channel->LinkUsage)
		{
		case 0x2:
		 	if (channel->IsButton)
			{
				ExtractedData->MOUDATA.OffsetButton = channel->ByteOffset - 1;
				ExtractedData->MOUDATA.BtnOffsetSize = channel->ByteEnd - channel->ByteOffset;
			}

			if (!channel->IsRange)
			{
				//Meaning X , Y, Z
				switch (channel->NotRange.Usage)
				{
				case HID_NOT_RANGE_USAGE_X:	//coordinate - X
					ExtractedData->MOUDATA.OffsetX = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.XOffsetSize = channel->ByteEnd - channel->ByteOffset;
					break;
				case HID_NOT_RANGE_USAGE_Y:	//coordinate - Y
					ExtractedData->MOUDATA.OffsetY = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.YOffsetSize = channel->ByteEnd - channel->ByteOffset;
					break;
				case HID_NOT_RANGE_USAGE_WHELL:  //coordinate - Z
					ExtractedData->MOUDATA.OffsetZ = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.ZOffsetSize = channel->ByteEnd - channel->ByteOffset;
					break;
				default:
					USB_MON_COMMON_DBG_BREAK();	//FATLA ERROR!!
					break;
				}
			} 
			break;
		
		case 0x6:
			break;
		default:
			break;
		}
		ExtractedData->MOUDATA.IsAbsolute = channel->IsAbsolute; 
		channel++;
	}
}
//----------------------------------------------------------------------------------------------------------//
VOID DumpChannel(PHIDP_COLLECTION_DESC collectionDesc, HIDP_REPORT_TYPE type, ULONG Flags )
{
	if (!collectionDesc)
	{
		return;
	}
	HIDP_CHANNEL_DESC*            channel = NULL;
	ULONG start = 0;
	ULONG end = 0;
	CHAR* reportType = NULL;
	switch (type)
	{
	case HidP_Input:
		channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Input.Offset];
		start = collectionDesc->PreparsedData->Input.Offset;
		end = collectionDesc->PreparsedData->Input.Index;
		reportType = "Input Report";
		break;
	case HidP_Output:
		channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Output.Offset]; 
		start = collectionDesc->PreparsedData->Output.Offset;
		end = collectionDesc->PreparsedData->Output.Index;
		reportType = "Output Report";

		break;
	case HidP_Feature:
		channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Feature.Offset]; 
		start = collectionDesc->PreparsedData->Feature.Offset;
		end = collectionDesc->PreparsedData->Feature.Index;
		reportType = "Feature Report"; 
		break;
	default:
			break;
	}

	for (int k = start  ; k < end ; k++)
	{
		USB_MON_DEBUG_INFO("+++++++++++++++++++++++ %s +++++++++++++++++++++\r\n" , reportType );
		if (Flags & CHANNEL_DUMP_REPORT_REALTED)
		{
			USB_MON_DEBUG_INFO("channel UsagePage: %x		OFFSET_FIELD: %x \r\n", channel->UsagePage, FIELD_OFFSET(HIDP_CHANNEL_DESC, UsagePage));
			USB_MON_DEBUG_INFO("channel ReportID: %d		OFFSET_FIELD: %x \r\n", channel->ReportID, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportID));
			USB_MON_DEBUG_INFO("channel ReportSize: %d		OFFSET_FIELD: %x \r\n", channel->ReportSize, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportSize));
			USB_MON_DEBUG_INFO("channel ReportCount: %d		OFFSET_FIELD: %x \r\n", channel->ReportCount, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportCount)); 
		}
		if (Flags & CHANNEL_DUMP_REPORT_BYTE_OFFSET_REALTED)
		{
			USB_MON_DEBUG_INFO("channel BitLength: %d		OFFSET_FIELD: %x \r\n", channel->BitLength, FIELD_OFFSET(HIDP_CHANNEL_DESC, BitLength));
			USB_MON_DEBUG_INFO("channel ByteEnd: %d			OFFSET_FIELD: %x \r\n", channel->ByteEnd, FIELD_OFFSET(HIDP_CHANNEL_DESC, ByteEnd));
			USB_MON_DEBUG_INFO("channel BitOffset: %d		OFFSET_FIELD: %x \r\n", channel->BitOffset, FIELD_OFFSET(HIDP_CHANNEL_DESC, BitOffset));
			USB_MON_DEBUG_INFO("channel ByteOffset: %d		OFFSET_FIELD: %x \r\n", channel->ByteOffset, FIELD_OFFSET(HIDP_CHANNEL_DESC, ByteOffset));
		}
		if (Flags & CHANNEL_DUMP_LINK_COL_RELATED)
		{
			USB_MON_DEBUG_INFO("channel LinkCollection: %x  OFFSET_FIELD: %x \r\n", channel->LinkCollection, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkCollection));
			USB_MON_DEBUG_INFO("channel LinkUsage: %x		OFFSET_FIELD: %x \r\n", channel->LinkUsage, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkUsage));
			USB_MON_DEBUG_INFO("channel LinkUsagePage: %x	OFFSET_FIELD: %x \r\n", channel->LinkUsagePage, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkUsagePage));
		}

 		if (Flags & CHANNEL_DUMP_ATTRIBUTE_RELATED)
		{
			USB_MON_DEBUG_INFO("channel IsRange: %x \r\n",			 channel->IsRange); 
			USB_MON_DEBUG_INFO("channel IsButton: %x   \r\n",		 channel->IsButton);
			USB_MON_DEBUG_INFO("channel IsAbsolute: %x \r\n",		 channel->IsAbsolute);
			USB_MON_DEBUG_INFO("channel IsConst: %x \r\n",			 channel->IsConst);
			USB_MON_DEBUG_INFO("channel IsAlias: %x \r\n",			 channel->IsAlias);
			USB_MON_DEBUG_INFO("channel IsDesignatorRange: %x \r\n", channel->IsDesignatorRange);
			USB_MON_DEBUG_INFO("channel IsStringRange: %x \r\n",	 channel->IsStringRange);
 

			if (!channel->IsButton)
			{
				USB_MON_DEBUG_INFO("channel Data.LogicalMin: %d	 OFFSET_FIELD: %X \r\n", channel->Data.HasNull, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.HasNull));
				USB_MON_DEBUG_INFO("channel Data.LogicalMin: %d	 OFFSET_FIELD: %X \r\n", channel->Data.LogicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.LogicalMin));
				USB_MON_DEBUG_INFO("channel Data.LogicalMax: %d	 OFFSET_FIELD: %X \r\n", channel->Data.LogicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.LogicalMax));
				USB_MON_DEBUG_INFO("channel Data.PhysicalMax: %d OFFSET_FIELD: %X  \r\n", channel->Data.PhysicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.PhysicalMin));
				USB_MON_DEBUG_INFO("channel Data.PhysicalMax: %d OFFSET_FIELD: %X  \r\n", channel->Data.PhysicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.PhysicalMax));
			}
			if (channel->IsButton)
			{
				USB_MON_DEBUG_INFO("channel button.LogicalMin %d OFFSET_FIELD: %X  \r\n", channel->button.LogicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, button.LogicalMin));
				USB_MON_DEBUG_INFO("channel button.LogicalMax %d OFFSET_FIELD: %X  \r\n", channel->button.LogicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, button.LogicalMax));
			}

			if (channel->MoreChannels) {
				USB_MON_DEBUG_INFO("MoreChannels ");
			}
			if (channel->IsConst) {
				USB_MON_DEBUG_INFO("Const ");
			}
			if (channel->IsButton) {
				USB_MON_DEBUG_INFO("Button ");
			}
			else {
				USB_MON_DEBUG_INFO("Value ");
			}
			if (channel->IsAbsolute) {
				USB_MON_DEBUG_INFO("Absolute ");
			}
			if (channel->IsAlias) {
				USB_MON_DEBUG_INFO("ALIAS! ");
			}
		}
		if (Flags & CHANNEL_DUMP_RANGE_RELATED)
		{
			if (channel->IsRange)
			{
				USB_MON_DEBUG_INFO("channel Range.UsageMin:  %d		   OFFSET_FIELD: %X  \r\n", channel->Range.UsageMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.UsageMin));
				USB_MON_DEBUG_INFO("channel Range.UsageMax:  %d		   OFFSET_FIELD: %X  \r\n", channel->Range.UsageMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.UsageMax));
				USB_MON_DEBUG_INFO("channel Range.DataIndexMax:  %d	   OFFSET_FIELD: %X  \r\n", channel->Range.DataIndexMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DataIndexMax));
				USB_MON_DEBUG_INFO("channel Range.DataIndexMin: %d	   OFFSET_FIELD: %X  \r\n", channel->Range.DataIndexMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DataIndexMin));

			}
			else
			{
				USB_MON_DEBUG_INFO("channel NotRange.Usage:  %x		     OFFSET_FIELD: %X  \r\n", channel->NotRange.Usage, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.Usage));
				USB_MON_DEBUG_INFO("channel NotRange.DataIndex:  %x		 OFFSET_FIELD: %X  \r\n", channel->NotRange.DataIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.DataIndex));
			}

			if (channel->IsDesignatorRange)
			{
				USB_MON_DEBUG_INFO("channel Range.DesignatorMax: %d OFFSET_FIELD: %X  \r\n", channel->Range.DesignatorMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DesignatorMax));
				USB_MON_DEBUG_INFO("channel Range.DesignatorMin: %d OFFSET_FIELD: %X  \r\n", channel->Range.DesignatorMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DesignatorMin));
			}
			else
			{
				USB_MON_DEBUG_INFO("channel Range.DesignatorMax: %d OFFSET_FIELD: %X  \r\n", channel->NotRange.DesignatorIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.DesignatorIndex));
			}

			if (channel->IsStringRange)
			{
				USB_MON_DEBUG_INFO("channel Range.StringMax: %d	   OFFSET_FIELD: %X  \r\n", channel->Range.StringMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.StringMax));
				USB_MON_DEBUG_INFO("channel Range.StringMin: %d	   OFFSET_FIELD: %X  \r\n", channel->Range.StringMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.StringMin));
			}
			else
			{
				USB_MON_DEBUG_INFO("channel NotRange.StringIndex:  %d	 OFFSET_FIELD: %X  \r\n", channel->NotRange.StringIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.StringIndex));
			}
		}
	 
		if (channel->NumGlobalUnknowns)
		{
			for (int z = 0; z < channel->NumGlobalUnknowns; z++)
			{
				USB_MON_DEBUG_INFO("channel UnknownsToken:  %d	 OFFSET_FIELD: %X  \r\n", channel->GlobalUnknowns[z].Token, FIELD_OFFSET(HIDP_CHANNEL_DESC, GlobalUnknowns));
			}
		}
		 
		USB_MON_DEBUG_INFO("\r\n"); 
		channel++; 
	}
}
//-------------------------------------------------------------------------------------------//
HID_DEVICE_NODE* CreateHidDeviceNode(
	_In_ PDEVICE_OBJECT device_object,
	_In_ HID_USB_DEVICE_EXTENSION* mini_extension,
	_In_ HIDP_DEVICE_DESC* parsedReport
)
{
	if (!device_object || !mini_extension || !parsedReport)
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
	node->parsedReport = parsedReport;
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
			USB_MON_DEBUG_INFO("Bad harware, returning NULL report ID");
		}
	}

	return result;
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
 
	 
	//USB_MON_DEBUG_INFO("Extension_common: %I64X sizeof: %x \r\n", hid_common_extension, sizeof(HID_USB_DEVICE_EXTENSION));
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
		 mini_extension->InterfaceDesc->Protocol == 2 ||		//Mouse
		 mini_extension->InterfaceDesc->Protocol == 0) )			
	{ 
		FDO_EXTENSION       *fdoExt = NULL;
		PDO_EXTENSION       *pdoExt = NULL;

		if (hid_common_extension->isClientPdo)
		{
			USB_MON_DEBUG_INFO("---------------------------------------------------------------------------------------------------- \r\n");

			USB_MON_DEBUG_INFO("hid_common_extension: %I64x \r\n", hid_common_extension);
			USB_MON_DEBUG_INFO("DeviceObj: %I64X  DriverName: %ws DeviceName: %ws \r\n", device_object, device_object->DriverObject->DriverName.Buffer, DeviceName);
			USB_MON_DEBUG_INFO("collectionIndex: %x collectionNum: %x \r\n", hid_common_extension->pdoExt.collectionIndex, hid_common_extension->pdoExt.collectionNum);

			USB_MON_DEBUG_INFO("---------------------------------------------------------------------------------------------------- \r\n"); 
			*hid_mini_extension = mini_extension;
			
		}
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

	USB_MON_DEBUG_INFO("DriverObj: %I64X \r\n", pDriverObj);

	device_object = pDriverObj->DeviceObject;
	while (device_object)
	{
		HID_USB_DEVICE_EXTENSION* mini_extension = NULL; 
		if (IsKeyboardOrMouseDevice(device_object, &mini_extension))
		{
			HIDP_DEVICE_DESC* report = GetReport(device_object->DeviceExtension);
		
			if (mini_extension && g_hid_relation)
			{
				HID_DEVICE_NODE* node = CreateHidDeviceNode(device_object, mini_extension, report);
				if (node) {
					AddToChainListTail(g_hid_relation->head, node);
					current_size++;
					USB_MON_DEBUG_INFO("Inserted one element: %I64x InferfaceDesc: %I64X device_object: %I64x \r\n", node->device_object, node->mini_extension, device_object);
					USB_MON_DEBUG_INFO("Added one element :%x \r\n", current_size);
				}
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
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           