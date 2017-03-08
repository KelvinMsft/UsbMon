/*!@file
*@brief HID parser type definitions
*
* Header GPL
* @todo Properly tag all files with GPL (as appropriate)
*/

#ifndef TYPE_H
#define TYPE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <fltKernel.h>
#include <sys/types.h>

	/*
	* Types
	*/
#if !AIX
	typedef unsigned char  uchar;
#endif

#if HPUX || __APPLE__
	typedef unsigned long  ULONG;
#endif

	typedef short          wchar;

	/*
	* Constants
	*/
#define PATH_SIZE               10 /*!< maximum depth for Path */
#define USAGE_TAB_SIZE          50 /*!< Size of usage stack */

	/*! Including FEATURE, INPUT and OUTPUT */
#define MAX_REPORT             300

	/*! Size max of Report Descriptor */
#define REPORT_DSC_SIZE       6144

	/*
	* Items
	* -------------------------------------------------------------------------- */
#define SIZE_0                0x00
#define SIZE_1                0x01
#define SIZE_2                0x02
#define SIZE_4                0x03
#define SIZE_MASK             0x03

#define TYPE_MAIN             0x00
#define TYPE_GLOBAL           0x04
#define TYPE_LOCAL            0x08
#define TYPE_MASK             0x0C

	/* Main items */
#define ITEM_COLLECTION       0xA0
#define ITEM_END_COLLECTION   0xC0
#define ITEM_FEATURE          0xB0
#define ITEM_INPUT            0x80
#define ITEM_OUTPUT           0x90

	/* Global items */
#define ITEM_UPAGE            0x04
#define ITEM_LOG_MIN          0x14
#define ITEM_LOG_MAX          0x24
#define ITEM_PHY_MIN          0x34
#define ITEM_PHY_MAX          0x44
#define ITEM_UNIT_EXP         0x54
#define ITEM_UNIT             0x64
#define ITEM_REP_SIZE         0x74
#define ITEM_REP_ID           0x84
#define ITEM_REP_COUNT        0x94

	/* Local items */
#define ITEM_USAGE            0x08
#define ITEM_STRING           0x78

	/* Long item */
#define ITEM_LONG	      0xFC

#define ITEM_MASK             0xFC

	/* Attribute Flags */
#define ATTR_DATA_CST         0x01
#define ATTR_NVOL_VOL         0x80

	/*!
	* Describe a HID Path point
	*/
	typedef struct
	{
		USHORT UPage;
		USHORT Usage;
	} HIDNode;

	/*!
	* Describe a HID Path
	*/
	typedef struct
	{
		uchar   Size;             /*!< HID Path size */
		HIDNode Node[PATH_SIZE];  /*!< HID Path */
	} HIDPath;

	/*!
	* Describe a HID Data with its location in report
	*/
	typedef struct
	{
		long    Value;            /*!< HID Object Value                             */
		HIDPath Path;             /*!< HID Path                                     */

		uchar   ReportID;         /*!< Report ID, (from incoming report) ???        */
		uchar   Offset;           /*!< Offset of data in report                     */
		uchar   Size;             /*!< Size of data in bit                          */

		uchar   Type;             /*!< Type : FEATURE / INPUT / OUTPUT              */
		uchar   Attribute;        /*!< Report field attribute                       */

		ULONG   Unit;             /*!< HID Unit                                     */
		char    UnitExp;          /*!< Unit exponent                                */

		long    LogMin;           /*!< Logical Min                                  */
		long    LogMax;           /*!< Logical Max                                  */
		long    PhyMin;           /*!< Physical Min                                 */
		long    PhyMax;           /*!< Physical Max                                 */
	} HIDData;

	/* -------------------------------------------------------------------------- */
	typedef struct
	{
		uchar   ReportDesc[REPORT_DSC_SIZE];  /*!< Store Report Descriptor          */
		USHORT  ReportDescSize;               /*!< Size of Report Descriptor        */
		USHORT  Pos;                          /*!< Store current pos in descriptor  */
		uchar   Item;                         /*!< Store current Item               */
		long    Value;                        /*!< Store current Value              */

		HIDData Data;                         /*!< Store current environment        */

		uchar   OffsetTab[MAX_REPORT][3];     /*!< Store ID, type & offset of report*/
		uchar   ReportCount;                  /*!< Store Report Count               */
		uchar   Count;                        /*!< Store local report count         */

		USHORT  UPage;                        /*!< Global UPage                     */
		HIDNode UsageTab[USAGE_TAB_SIZE];     /*!< Usage stack                      */
		uchar   UsageSize;                    /*!< Design number of usage used      */

		uchar   nObject;                      /*!< Count objects in Report Descriptor */
		uchar   nReport;                      /*!< Count reports in Report Descriptor */
	} HIDParser;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif