#include <fltKernel.h>
#include "ReportUtil.h"
#include "CommonUtil.h"
#include "WinParse.h"
#include "UsbType.h"

typedef struct _SELECTED_CHANNEL
{
	HIDP_CHANNEL_DESC*            channel;
	ULONG							start;
	ULONG							  end;
	CHAR				  reportType[256];
}SELETEDCHANNEL, *PSELECTEDCHANNEL;

//------------------------------------------------------------------------------------------------------------------------------//
VOID DumpReport(
	_In_ HIDP_DEVICE_DESC* report)
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
VOID DumpChannel(
	_In_ PHIDP_COLLECTION_DESC collectionDesc, 
	_In_ HIDP_REPORT_TYPE type, 
	_In_ ULONG Flags)
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
			USB_MON_DEBUG_INFO(" UsagePage: %x		OFFSET_FIELD: %x \r\n", channel->UsagePage, FIELD_OFFSET(HIDP_CHANNEL_DESC, UsagePage));
			USB_MON_DEBUG_INFO(" ReportID: %d		OFFSET_FIELD: %x \r\n", channel->ReportID, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportID));
			USB_MON_DEBUG_INFO(" ReportSize: %d		OFFSET_FIELD: %x \r\n", channel->ReportSize, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportSize));
			USB_MON_DEBUG_INFO(" ReportCount: %d		OFFSET_FIELD: %x \r\n", channel->ReportCount, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportCount));
		}
		if (Flags & CHANNEL_DUMP_REPORT_BYTE_OFFSET_REALTED)
		{
			USB_MON_DEBUG_INFO(" BitLength: %d		OFFSET_FIELD: %x \r\n", channel->BitLength, FIELD_OFFSET(HIDP_CHANNEL_DESC, BitLength));
			USB_MON_DEBUG_INFO(" ByteEnd: %d			OFFSET_FIELD: %x \r\n", channel->ByteEnd, FIELD_OFFSET(HIDP_CHANNEL_DESC, ByteEnd));
			USB_MON_DEBUG_INFO(" BitOffset: %d		OFFSET_FIELD: %x \r\n", channel->BitOffset, FIELD_OFFSET(HIDP_CHANNEL_DESC, BitOffset));
			USB_MON_DEBUG_INFO(" ByteOffset: %d		OFFSET_FIELD: %x \r\n", channel->ByteOffset, FIELD_OFFSET(HIDP_CHANNEL_DESC, ByteOffset));
		}
		if (Flags & CHANNEL_DUMP_LINK_COL_RELATED)
		{
			USB_MON_DEBUG_INFO(" LinkCollection: %x  OFFSET_FIELD: %x \r\n", channel->LinkCollection, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkCollection));
			USB_MON_DEBUG_INFO(" LinkUsage: %x		OFFSET_FIELD: %x \r\n", channel->LinkUsage, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkUsage));
			USB_MON_DEBUG_INFO(" LinkUsagePage: %x	OFFSET_FIELD: %x \r\n", channel->LinkUsagePage, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkUsagePage));
		}

		if (Flags & CHANNEL_DUMP_ATTRIBUTE_RELATED)
		{
			USB_MON_DEBUG_INFO(" IsRange: %x \r\n", channel->IsRange);
			USB_MON_DEBUG_INFO(" IsButton: %x   \r\n", channel->IsButton);
			USB_MON_DEBUG_INFO(" IsAbsolute: %x \r\n", channel->IsAbsolute);
			USB_MON_DEBUG_INFO(" IsConst: %x \r\n", channel->IsConst);
			USB_MON_DEBUG_INFO(" IsAlias: %x \r\n", channel->IsAlias);
			USB_MON_DEBUG_INFO(" IsDesignatorRange: %x \r\n", channel->IsDesignatorRange);
			USB_MON_DEBUG_INFO(" IsStringRange: %x \r\n", channel->IsStringRange);


			if (!channel->IsButton)
			{
				USB_MON_DEBUG_INFO(" Data.LogicalMin: %d	 OFFSET_FIELD: %X \r\n", channel->Data.HasNull, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.HasNull));
				USB_MON_DEBUG_INFO(" Data.LogicalMin: %d	 OFFSET_FIELD: %X \r\n", channel->Data.LogicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.LogicalMin));
				USB_MON_DEBUG_INFO(" Data.LogicalMax: %d	 OFFSET_FIELD: %X \r\n", channel->Data.LogicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.LogicalMax));
				USB_MON_DEBUG_INFO(" Data.PhysicalMax: %d OFFSET_FIELD: %X  \r\n", channel->Data.PhysicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.PhysicalMin));
				USB_MON_DEBUG_INFO(" Data.PhysicalMax: %d OFFSET_FIELD: %X  \r\n", channel->Data.PhysicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.PhysicalMax));
			}
			if (channel->IsButton)
			{
				USB_MON_DEBUG_INFO(" button.LogicalMin %d OFFSET_FIELD: %X  \r\n", channel->button.LogicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, button.LogicalMin));
				USB_MON_DEBUG_INFO(" button.LogicalMax %d OFFSET_FIELD: %X  \r\n", channel->button.LogicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, button.LogicalMax));
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

		USB_MON_DEBUG_INFO("\r\n");

		if (Flags & CHANNEL_DUMP_RANGE_RELATED)
		{
			if (channel->IsRange)
			{
				USB_MON_DEBUG_INFO(" Range.UsageMin:  %d		   OFFSET_FIELD: %X  \r\n", channel->Range.UsageMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.UsageMin));
				USB_MON_DEBUG_INFO(" Range.UsageMax:  %d		   OFFSET_FIELD: %X  \r\n", channel->Range.UsageMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.UsageMax));
				USB_MON_DEBUG_INFO(" Range.DataIndexMax:  %d	   OFFSET_FIELD: %X  \r\n", channel->Range.DataIndexMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DataIndexMax));
				USB_MON_DEBUG_INFO(" Range.DataIndexMin: %d	   OFFSET_FIELD: %X  \r\n", channel->Range.DataIndexMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DataIndexMin));
			}
			else
			{
				USB_MON_DEBUG_INFO(" NotRange.Usage:  %x		     OFFSET_FIELD: %X  \r\n", channel->NotRange.Usage, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.Usage));
				USB_MON_DEBUG_INFO(" NotRange.DataIndex:  %x		 OFFSET_FIELD: %X  \r\n", channel->NotRange.DataIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.DataIndex));
			}

			if (channel->IsDesignatorRange)
			{
				USB_MON_DEBUG_INFO(" Range.DesignatorMax: %d OFFSET_FIELD: %X  \r\n", channel->Range.DesignatorMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DesignatorMax));
				USB_MON_DEBUG_INFO(" Range.DesignatorMin: %d OFFSET_FIELD: %X  \r\n", channel->Range.DesignatorMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DesignatorMin));
			}
			else
			{
				USB_MON_DEBUG_INFO(" Range.DesignatorMax: %d OFFSET_FIELD: %X  \r\n", channel->NotRange.DesignatorIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.DesignatorIndex));
			}

			if (channel->IsStringRange)
			{
				USB_MON_DEBUG_INFO(" Range.StringMax: %d	   OFFSET_FIELD: %X  \r\n", channel->Range.StringMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.StringMax));
				USB_MON_DEBUG_INFO(" Range.StringMin: %d	   OFFSET_FIELD: %X  \r\n", channel->Range.StringMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.StringMin));
			}
			else
			{
				USB_MON_DEBUG_INFO(" NotRange.StringIndex:  %d	 OFFSET_FIELD: %X  \r\n", channel->NotRange.StringIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.StringIndex));
			}
		}

		if (channel->NumGlobalUnknowns)
		{
			for (int z = 0; z < channel->NumGlobalUnknowns; z++)
			{
				USB_MON_DEBUG_INFO(" UnknownsToken:  %d	 OFFSET_FIELD: %X  \r\n", channel->GlobalUnknowns[z].Token, FIELD_OFFSET(HIDP_CHANNEL_DESC, GlobalUnknowns));
			}
		}

		USB_MON_DEBUG_INFO("\r\n");
		channel++;
	}
}
//------------------------------------------------------------------------------------------------------------------------------//
NTSTATUS SelectChannel(
	_In_ HIDP_REPORT_TYPE				type,	
	_In_ PHIDP_COLLECTION_DESC	collectionDesc,
	_Out_ SELETEDCHANNEL*       selectedChannel
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	if (!collectionDesc || !selectedChannel)
	{
		return status;
	}
	switch (type)
	{
	case HidP_Input:
		selectedChannel->channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Input.Offset];
		selectedChannel->start = collectionDesc->PreparsedData->Input.Offset;
		selectedChannel->end = collectionDesc->PreparsedData->Input.Index;
		strncpy(selectedChannel->reportType , "Input Report", sizeof("Input Report"));
		break;
	case HidP_Output:
		selectedChannel->channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Output.Offset];
		selectedChannel->start = collectionDesc->PreparsedData->Output.Offset;
		selectedChannel->end = collectionDesc->PreparsedData->Output.Index;
		strncpy(selectedChannel->reportType, "Output Report", sizeof("Output Report"));
		break;
	case HidP_Feature:
		selectedChannel->channel = &collectionDesc->PreparsedData->Data[collectionDesc->PreparsedData->Feature.Offset];
		selectedChannel->start = collectionDesc->PreparsedData->Feature.Offset;
		selectedChannel->end = collectionDesc->PreparsedData->Feature.Index;
		strncpy(selectedChannel->reportType , "Feature Report", sizeof("Feature Report"));
		break;
	default:
		break;
	}
	status = STATUS_SUCCESS;
	return status;
}

//------------------------------------------------------------------------------------------------------------------------------//
NTSTATUS ExtractKeyboardData(
	_In_	 PHIDP_COLLECTION_DESC collectionDesc,
	_In_	 HIDP_REPORT_TYPE type,
	_Inout_  EXTRACTDATA* ExtractedData)
{
	HIDP_CHANNEL_DESC*            channel = NULL;
	SELETEDCHANNEL			     selected_channel = { 0 };
	NTSTATUS status = STATUS_SUCCESS;

	if (!collectionDesc || !ExtractedData)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	SelectChannel(type, collectionDesc, &selected_channel);
	
	USB_MON_DEBUG_INFO("Start: %x End: %x ReportType: %s \r\n", selected_channel.start, selected_channel.end, selected_channel.reportType);
	
	channel = selected_channel.channel; 
	for (int k = selected_channel.start; k < selected_channel.end; k++)
	{ 
		switch (channel->UsagePage)
		{
			case HID_LEDS:
			{
				ExtractedData->KBDDATA.LedKeyOffset = channel->ByteOffset - 1;
				ExtractedData->KBDDATA.LedKeySize = channel->ByteEnd - channel->ByteOffset;
			}
				break;
			case HID_KEYBOARD_OR_KEYPAD:
			{
				if (channel->Range.UsageMax == 0xFF)
				{
					ExtractedData->KBDDATA.NormalKeyOffset = channel->ByteOffset - 1;
					ExtractedData->KBDDATA.NormalKeySize = channel->ByteEnd - channel->ByteOffset;
				}
				else
				{
					ExtractedData->KBDDATA.SpecialKeyOffset = channel->ByteOffset - 1;
					ExtractedData->KBDDATA.SpecialKeySize = channel->ByteEnd - channel->ByteOffset;
				}
			} 
				break;
			default:
				USB_MON_COMMON_DBG_BREAK();
				break;
		}
		ExtractedData->MOUDATA.IsAbsolute = channel->IsAbsolute;
		channel++;
	} 
	return status;
}

//------------------------------------------------------------------------------------------------------------------------------//
NTSTATUS ExtractMouseData(
	_In_	 PHIDP_COLLECTION_DESC collectionDesc, 
	_In_	 HIDP_REPORT_TYPE type, 
	_Inout_  EXTRACTDATA* ExtractedData)
{
	HIDP_CHANNEL_DESC*            channel = NULL;
	SELETEDCHANNEL			     selected_channel = { 0 };
	NTSTATUS status = STATUS_SUCCESS;

	if (!collectionDesc || !ExtractedData)
	{
		status = STATUS_UNSUCCESSFUL;
		return status; 
	}

	if (!NT_SUCCESS(SelectChannel(type, collectionDesc, &selected_channel)))
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	USB_MON_DEBUG_INFO("Start: %x End: %x ReportType: %s \r\n", selected_channel.start, selected_channel.end, selected_channel.reportType); 
	channel = selected_channel.channel;
	for (int k = selected_channel.start; k < selected_channel.end; k++)
	{
		if (channel->IsButton)
		{
			if (channel->IsRange)
			{
				ExtractedData->MOUDATA.OffsetButton = channel->ByteOffset - 1;
				ExtractedData->MOUDATA.BtnOffsetSize = channel->ByteEnd - channel->ByteOffset;
			}
		} 
		else 
		{
			if (!channel->IsRange)
			{
				//Meaning X , Y, Z
				switch (channel->NotRange.Usage)
				{
				case HID_NOT_RANGE_USAGE_X:		 //coordinate - X
					ExtractedData->MOUDATA.OffsetX = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.XOffsetSize = channel->ByteEnd - channel->ByteOffset;
					break;
				case HID_NOT_RANGE_USAGE_Y:		 //coordinate - Y
					ExtractedData->MOUDATA.OffsetY = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.YOffsetSize = channel->ByteEnd - channel->ByteOffset;
					break;
				case HID_NOT_RANGE_USAGE_WHELL:  //coordinate - Z
					ExtractedData->MOUDATA.OffsetZ = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.ZOffsetSize = channel->ByteEnd - channel->ByteOffset;
					break;
				default:
					USB_MON_COMMON_DBG_BREAK();	//FATAL ! Ignore it
					break;
				}
			}
		}
		ExtractedData->MOUDATA.IsAbsolute = channel->IsAbsolute;
		channel++;
	}

	return status;
}