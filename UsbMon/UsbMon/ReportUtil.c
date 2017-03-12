#include <fltKernel.h>
#include "ReportUtil.h"
#include "CommonUtil.h"
#include "WinParse.h"
#include "UsbType.h"

//------------------------------------------------------------------------------------------------------------------------------//
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
//------------------------------------------------------------------------------------------------------------------------------//
VOID DumpChannel(PHIDP_COLLECTION_DESC collectionDesc, HIDP_REPORT_TYPE type, ULONG Flags)
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

	for (int k = start; k < end; k++)
	{
		USB_MON_DEBUG_INFO("+++++++++++++++++++++++ %s +++++++++++++++++++++\r\n", reportType);
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
			USB_MON_DEBUG_INFO("channel IsRange: %x \r\n", channel->IsRange);
			USB_MON_DEBUG_INFO("channel IsButton: %x   \r\n", channel->IsButton);
			USB_MON_DEBUG_INFO("channel IsAbsolute: %x \r\n", channel->IsAbsolute);
			USB_MON_DEBUG_INFO("channel IsConst: %x \r\n", channel->IsConst);
			USB_MON_DEBUG_INFO("channel IsAlias: %x \r\n", channel->IsAlias);
			USB_MON_DEBUG_INFO("channel IsDesignatorRange: %x \r\n", channel->IsDesignatorRange);
			USB_MON_DEBUG_INFO("channel IsStringRange: %x \r\n", channel->IsStringRange);


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

//------------------------------------------------------------------------------------------------------------------------------//
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
		if (channel->LinkUsagePage == 0x1)
		{
			switch (channel->LinkUsage)
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
		}
		channel++;
	}
}