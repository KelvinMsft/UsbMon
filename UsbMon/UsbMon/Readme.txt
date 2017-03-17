///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
//

 Usb Hid Device Createion Process:
----------------------------------------------------
Driver Stack:

----->	  Filter 
			|
			|
			|
			|
-----> MouClass / KbdClass.sys	(KbdServiceCallback)
			|
			|
			|
			|
-----> MouHid / Kbdhid.sys			
			|
			|
			|
			|
			|	¡¡£ß£ß£ß£ß£ß£ß£ß HID Usb Device Stack			
			|	 /				----------------	 £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß 
			|	£ü				+  Client PDO  +								               £Ü
			|	£ü				+--------------+	 £ß£ß£ß£ß£ß								    |
			|	£ü				+	  FDO	   +			¡¡£Ü			¡¡¡¡			            |
			|	£ü				----------------			   |								| Query Pnp Relations by FDO:
-----> UsbHid.sys(miniclass driver)							   |							   (7)		- Create Client Pdo	 
			|       /\										   | AddDevice:						|			(0000000xx)	¡¡				¡¡
			|	 ¡¡¡¡|										  (2)	- Create FDO            	|							¡¡
			| ¡¡£ø   |									       |	 (HID_xxxxxxx)				|							¡¡
			|	|¡¡¡¡|     Call Mini-Class AddDevice		       |								|								                                    <<<<<<<<<<<
			|	|¡¡¡¡|£ß£ß£ß£ß£ß£ß£ß£ß(3)£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß / £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£¯ £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß   £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß HidClass.sys(Class driver)							¡¡
			|	|											¡¡  /\			¡¡¡¡¡¡/\					¡¡¡¡/\					   ¡¡\  
			|   |					¡¡¡¡							£ü				  |						|		                 |
			|   |					¡¡¡¡							£ü Actual		  | IRP Query Pnp		| IRP StartDevice Pnp	 |	¡¡
			|   |					¡¡¡¡							£ü AddDevice		 (6) Reations			(4) Reations             |	 
			|   |					¡¡¡¡						    (1)Path           |  					|                        |
			|   |					¡¡¡¡							£ü                |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡   (5) IoInvalidateDeviceRelations
			|   |					¡¡¡¡							£ü                |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |
			|   |					¡¡¡¡							£ü                |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |
			|	|					¡¡¡¡							£ü                |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |											
-----> UsbHub   |					¡¡¡¡							£ü                |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |										
   (Bus Driver) |	 UsbHid Expected AddDevice Call Path		£ü                |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |									 
				£Ü£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß £Ü£ß£ß£ß£ß£ß£ß£ß£ß £Ü£ß£ß£ß£ß£ß£ß£ß£ß£ß    £Ü£ß£ß£ß£ß£ß£ß£ß          \/ £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß   	 PnP		  
																																									   Manager	  



/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Windows 7 - 10 x64 structure:

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
-------------------------------------------------------------------------------------------------------*/