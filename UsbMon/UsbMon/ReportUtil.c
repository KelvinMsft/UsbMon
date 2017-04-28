
#include "ReportUtil.h" 
#include "WinParse.h"  

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
	ULONG i = 0;
	PHIDP_REPORT_IDS      reportDesc = NULL;
	PHIDP_COLLECTION_DESC collectionDesc = &report->CollectionDesc[0];
	for (i = 0; i < report->CollectionDescLength; i++)
	{
		ASSERT(collectionDesc->PreparsedData->Signature1 == HIDP_PREPARSED_DATA_SIGNATURE1);
		ASSERT(collectionDesc->PreparsedData->Signature2 == HIDP_PREPARSED_DATA_SIGNATURE2);

		USB_DEBUG_INFO_LN_EX("+++++++++++++++++++++++++++++++++++++++++++++++++++++++");
		USB_DEBUG_INFO_LN_EX("[Collection] Usage: %xh", collectionDesc->Usage);
		USB_DEBUG_INFO_LN_EX("[Collection] UsagePage: %xh ", collectionDesc->UsagePage);
		USB_DEBUG_INFO_LN_EX("[Collection] InputLength: %xh ", collectionDesc->InputLength);
		USB_DEBUG_INFO_LN_EX("[Collection] OutputLength: %xh ", collectionDesc->OutputLength);
		USB_DEBUG_INFO_LN_EX("[Collection] FeatureLength: %xh ", collectionDesc->FeatureLength);
		USB_DEBUG_INFO_LN_EX("[Collection] CollectionNumber: %x ", collectionDesc->CollectionNumber);
		USB_DEBUG_INFO_LN_EX("[Collection] PreparsedData: %I64x ", collectionDesc->PreparsedData);

		USB_DEBUG_INFO_LN_EX("[Collection] Input Offset: %x  Index: %x ", collectionDesc->PreparsedData->Input.Offset, collectionDesc->PreparsedData->Input.Index);
		USB_DEBUG_INFO_LN_EX("[Collection] Output Offset: %x Index: %x ", collectionDesc->PreparsedData->Output.Offset, collectionDesc->PreparsedData->Output.Index);
		USB_DEBUG_INFO_LN_EX("[Collection] Feature Offset: %x Index: %x ", collectionDesc->PreparsedData->Feature.Offset, collectionDesc->PreparsedData->Feature.Index);


		DumpChannel(collectionDesc, HidP_Input, CHANNEL_DUMP_ALL);
		DumpChannel(collectionDesc, HidP_Output, CHANNEL_DUMP_ALL);
		DumpChannel(collectionDesc, HidP_Feature, CHANNEL_DUMP_ALL);

		USB_DEBUG_INFO_LN_EX("+++++++++++++++++++++++++++++++++++++++++++++++++++++++");
		collectionDesc++;
	}

	reportDesc = &report->ReportIDs[0];
	for (i = 0; i < report->ReportIDsLength; i++)
	{
		USB_DEBUG_INFO_LN_EX("*******************************************************");
		USB_DEBUG_INFO_LN_EX("[Report] ReportID: %xh ", reportDesc->ReportID);
		USB_DEBUG_INFO_LN_EX("[Report] InputLength: %xh ", reportDesc->InputLength);
		USB_DEBUG_INFO_LN_EX("[Report] OutputLength: %xh ", reportDesc->OutputLength);
		USB_DEBUG_INFO_LN_EX("[Report] FeatureLength: %xh ", reportDesc->FeatureLength);
		USB_DEBUG_INFO_LN_EX("[Report] CollectionNumber: %x ", reportDesc->CollectionNumber);
		USB_DEBUG_INFO_LN_EX("*******************************************************");
		reportDesc++;
	}
}

//------------------------------------------------------------------------------------------------------------------------------//
VOID DumpChannel(
	_In_ PHIDP_COLLECTION_DESC collectionDesc,
	_In_ HIDP_REPORT_TYPE type,
	_In_ ULONG Flags)
{
	ULONG k = 0;
	HIDP_CHANNEL_DESC* channel = NULL;
	ULONG start = 0;
	ULONG end = 0;
	CHAR* reportType = NULL;

	if (!collectionDesc)
	{
		return;
	}

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

	for (k = start; k < end; k++)
	{
		USB_DEBUG_INFO_LN_EX("+++++++++++++++++++++++ %s +++++++++++++++++++++", reportType);
		if (Flags & CHANNEL_DUMP_REPORT_REALTED)
		{
			USB_DEBUG_INFO_LN_EX(" UsagePage: %x		OFFSET_FIELD: %x ", channel->UsagePage, FIELD_OFFSET(HIDP_CHANNEL_DESC, UsagePage));
			USB_DEBUG_INFO_LN_EX(" ReportID: %d		OFFSET_FIELD: %x ", channel->ReportID, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportID));
			USB_DEBUG_INFO_LN_EX(" ReportSize: %d		OFFSET_FIELD: %x ", channel->ReportSize, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportSize));
			USB_DEBUG_INFO_LN_EX(" ReportCount: %d		OFFSET_FIELD: %x ", channel->ReportCount, FIELD_OFFSET(HIDP_CHANNEL_DESC, ReportCount));
		}
		if (Flags & CHANNEL_DUMP_REPORT_BYTE_OFFSET_REALTED)
		{
			USB_DEBUG_INFO_LN_EX(" BitLength: %d		OFFSET_FIELD: %x ", channel->BitLength, FIELD_OFFSET(HIDP_CHANNEL_DESC, BitLength));
			USB_DEBUG_INFO_LN_EX(" ByteEnd: %d			OFFSET_FIELD: %x ", channel->ByteEnd, FIELD_OFFSET(HIDP_CHANNEL_DESC, ByteEnd));
			USB_DEBUG_INFO_LN_EX(" BitOffset: %d		OFFSET_FIELD: %x ", channel->BitOffset, FIELD_OFFSET(HIDP_CHANNEL_DESC, BitOffset));
			USB_DEBUG_INFO_LN_EX(" ByteOffset: %d		OFFSET_FIELD: %x ", channel->ByteOffset, FIELD_OFFSET(HIDP_CHANNEL_DESC, ByteOffset));
		}
		if (Flags & CHANNEL_DUMP_LINK_COL_RELATED)
		{
			USB_DEBUG_INFO_LN_EX(" LinkCollection: %x  OFFSET_FIELD: %x ", channel->LinkCollection, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkCollection));
			USB_DEBUG_INFO_LN_EX(" LinkUsage: %x		OFFSET_FIELD: %x ", channel->LinkUsage, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkUsage));
			USB_DEBUG_INFO_LN_EX(" LinkUsagePage: %x	OFFSET_FIELD: %x ", channel->LinkUsagePage, FIELD_OFFSET(HIDP_CHANNEL_DESC, LinkUsagePage));
		}

		if (Flags & CHANNEL_DUMP_ATTRIBUTE_RELATED)
		{
			USB_DEBUG_INFO_LN_EX(" IsRange: %x ", channel->IsRange);
			USB_DEBUG_INFO_LN_EX(" IsButton: %x   ", channel->IsButton);
			USB_DEBUG_INFO_LN_EX(" IsAbsolute: %x ", channel->IsAbsolute);
			USB_DEBUG_INFO_LN_EX(" IsConst: %x ", channel->IsConst);
			USB_DEBUG_INFO_LN_EX(" IsAlias: %x ", channel->IsAlias);
			USB_DEBUG_INFO_LN_EX(" IsDesignatorRange: %x ", channel->IsDesignatorRange);
			USB_DEBUG_INFO_LN_EX(" IsStringRange: %x ", channel->IsStringRange);


			if (!channel->IsButton)
			{
				USB_DEBUG_INFO_LN_EX(" Data.LogicalMin: %d	 OFFSET_FIELD: %X ", channel->Data.HasNull, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.HasNull));
				USB_DEBUG_INFO_LN_EX(" Data.LogicalMin: %d	 OFFSET_FIELD: %X ", channel->Data.LogicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.LogicalMin));
				USB_DEBUG_INFO_LN_EX(" Data.LogicalMax: %d	 OFFSET_FIELD: %X ", channel->Data.LogicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.LogicalMax));
				USB_DEBUG_INFO_LN_EX(" Data.PhysicalMax: %d OFFSET_FIELD: %X  ", channel->Data.PhysicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.PhysicalMin));
				USB_DEBUG_INFO_LN_EX(" Data.PhysicalMax: %d OFFSET_FIELD: %X  ", channel->Data.PhysicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Data.PhysicalMax));
			}
			if (channel->IsButton)
			{
				USB_DEBUG_INFO_LN_EX(" button.LogicalMin %d OFFSET_FIELD: %X  ", channel->button.LogicalMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, button.LogicalMin));
				USB_DEBUG_INFO_LN_EX(" button.LogicalMax %d OFFSET_FIELD: %X  ", channel->button.LogicalMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, button.LogicalMax));
			}

			if (channel->MoreChannels) {
				USB_DEBUG_INFO_LN_EX("MoreChannels ");
			}
			if (channel->IsConst) {
				USB_DEBUG_INFO_LN_EX("Const ");
			}
			if (channel->IsButton) {
				USB_DEBUG_INFO_LN_EX("Button ");
			}
			else {
				USB_DEBUG_INFO_LN_EX("Value ");
			}
			if (channel->IsAbsolute) {
				USB_DEBUG_INFO_LN_EX("Absolute ");
			}
			if (channel->IsAlias) {
				USB_DEBUG_INFO_LN_EX("ALIAS! ");
			}
		}

		//USB_DEBUG_INFO_LN();

		if (Flags & CHANNEL_DUMP_RANGE_RELATED)
		{
			if (channel->IsRange)
			{
				USB_DEBUG_INFO_LN_EX(" Range.UsageMin:  %d		   OFFSET_FIELD: %X  ", channel->Range.UsageMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.UsageMin));
				USB_DEBUG_INFO_LN_EX(" Range.UsageMax:  %d		   OFFSET_FIELD: %X  ", channel->Range.UsageMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.UsageMax));
				USB_DEBUG_INFO_LN_EX(" Range.DataIndexMax:  %d	   OFFSET_FIELD: %X  ", channel->Range.DataIndexMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DataIndexMax));
				USB_DEBUG_INFO_LN_EX(" Range.DataIndexMin: %d	   OFFSET_FIELD: %X  ", channel->Range.DataIndexMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DataIndexMin));
			}
			else
			{
				USB_DEBUG_INFO_LN_EX(" NotRange.Usage:  %x		     OFFSET_FIELD: %X  ", channel->NotRange.Usage, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.Usage));
				USB_DEBUG_INFO_LN_EX(" NotRange.DataIndex:  %x		 OFFSET_FIELD: %X  ", channel->NotRange.DataIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.DataIndex));
			}

			if (channel->IsDesignatorRange)
			{
				USB_DEBUG_INFO_LN_EX(" Range.DesignatorMax: %d OFFSET_FIELD: %X  ", channel->Range.DesignatorMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DesignatorMax));
				USB_DEBUG_INFO_LN_EX(" Range.DesignatorMin: %d OFFSET_FIELD: %X  ", channel->Range.DesignatorMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.DesignatorMin));
			}
			else
			{
				USB_DEBUG_INFO_LN_EX(" Range.DesignatorMax: %d OFFSET_FIELD: %X  ", channel->NotRange.DesignatorIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.DesignatorIndex));
			}

			if (channel->IsStringRange)
			{
				USB_DEBUG_INFO_LN_EX(" Range.StringMax: %d	   OFFSET_FIELD: %X  ", channel->Range.StringMax, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.StringMax));
				USB_DEBUG_INFO_LN_EX(" Range.StringMin: %d	   OFFSET_FIELD: %X  ", channel->Range.StringMin, FIELD_OFFSET(HIDP_CHANNEL_DESC, Range.StringMin));
			}
			else
			{
				USB_DEBUG_INFO_LN_EX(" NotRange.StringIndex:  %d	 OFFSET_FIELD: %X  ", channel->NotRange.StringIndex, FIELD_OFFSET(HIDP_CHANNEL_DESC, NotRange.StringIndex));
			}
		}

		if (channel->NumGlobalUnknowns)
		{
			ULONG z = 0;
			for (z = 0; z < channel->NumGlobalUnknowns; z++)
			{
				USB_DEBUG_INFO_LN_EX(" UnknownsToken:  %d	 OFFSET_FIELD: %X  ", channel->GlobalUnknowns[z].Token, FIELD_OFFSET(HIDP_CHANNEL_DESC, GlobalUnknowns));
			}
		}

		//USB_DEBUG_INFO_LN();

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
		strncpy(selectedChannel->reportType, "Input Report", sizeof("Input Report"));
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
		strncpy(selectedChannel->reportType, "Feature Report", sizeof("Feature Report"));
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
	ULONG k = 0;
	if (!collectionDesc || !ExtractedData)
	{
		status = STATUS_UNSUCCESSFUL;
		return status;
	}

	SelectChannel(type, collectionDesc, &selected_channel);

	USB_DEBUG_INFO_LN_EX("Start: %x End: %x ReportType: %s", selected_channel.start, selected_channel.end, selected_channel.reportType);

	channel = selected_channel.channel;
	for (k = selected_channel.start; k < selected_channel.end; k++)
	{
		switch (channel->UsagePage)
		{
			//Output use
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
	ULONG k = 0;
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

	USB_DEBUG_INFO_LN_EX("Start: %x End: %x ReportType: %s", selected_channel.start, selected_channel.end, selected_channel.reportType);

	channel = selected_channel.channel;
	for (k = selected_channel.start; k < selected_channel.end; k++)
	{
		if (channel->IsButton)
		{
			if (channel->IsRange)
			{
				ExtractedData->MOUDATA.ByteOffsetBtn = channel->ByteOffset - 1;
				ExtractedData->MOUDATA.BitOffsetBtn = channel->BitOffset;
				ExtractedData->MOUDATA.BtnOffsetSize = channel->BitLength;
				if (channel->ReportID)
				{
					ExtractedData->MOUDATA.ByteOffsetBtn++;
				}
			}
		}
		else // not a bug means it is data.
		{
			// Not a range data 
			if (!channel->IsRange)
			{
				//Meaning X , Y, Z
				switch (channel->NotRange.Usage)
				{
				case HID_NOT_RANGE_USAGE_X:		 //coordinate - X  
					ExtractedData->MOUDATA.ByteOffsetX = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.BitOffsetX = channel->BitOffset; //(channel->ByteOffset - 1) * 8 +channel->BitOffset;
					ExtractedData->MOUDATA.XOffsetSize = channel->BitLength; //channel->ByteEnd - channel->ByteOffset; 
					ExtractedData->MOUDATA.IsAbsolute = channel->IsAbsolute;
					ExtractedData->MOUDATA.ReportId = channel->ReportID;
					if (channel->ReportID)
					{
						ExtractedData->MOUDATA.ByteOffsetX++;
					}
					break;
				case HID_NOT_RANGE_USAGE_Y:		 //coordinate - Y
					ExtractedData->MOUDATA.ByteOffsetY = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.BitOffsetY = channel->BitOffset; //	ExtractedData->MOUDATA.BitOffsetY  = (channel->ByteOffset - 1) * 8 +channel->BitOffset; 
					ExtractedData->MOUDATA.YOffsetSize = channel->BitLength; //channel->ByteEnd - channel->ByteOffset;  
					ExtractedData->MOUDATA.IsAbsolute = channel->IsAbsolute;
					if (channel->ReportID)
					{
						ExtractedData->MOUDATA.ByteOffsetY++;
					}
					break;
				case HID_NOT_RANGE_USAGE_WHELL:  //coordinate - Z
					ExtractedData->MOUDATA.ByteOffsetZ = channel->ByteOffset - 1;
					ExtractedData->MOUDATA.BitOffsetZ = channel->BitOffset; //	ExtractedData->MOUDATA.BitOffsetY  = (channel->ByteOffset - 1) * 8 +channel->BitOffset;  
					ExtractedData->MOUDATA.ZOffsetSize = channel->BitLength; //channel->ByteEnd - channel->ByteOffset; 
					if (channel->ReportID)
					{
						ExtractedData->MOUDATA.ByteOffsetZ++;
					}
					break;
				default:
					//USB_MON_COMMON_DBG_BREAK();	//FATAL ! Ignore it
					break;
				}


			}
		}

		channel++;
	}

	return status;
}