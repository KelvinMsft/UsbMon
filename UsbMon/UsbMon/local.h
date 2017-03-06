#include <fltKernel.h>
#include "UsbType.h"
#include "Hidpddi.h" 

typedef VOID(t_BluescreenFunction)(PVOID Context, PCHAR Buffer);
// Blue Screen IOCTL struct
typedef struct _BlueScreen {
	PVOID Context;                          // Context to pass to processing routine
	t_BluescreenFunction *BluescreenFunction;// Processing routine
	ULONG *IsBluescreenTime;                // Non zero -> blue screen happening
} BLUESCREEN, *PBLUESCREEN;


typedef struct _HID_DESCRIPTOR            *PHID_DESCRIPTOR;
typedef struct _HIDCLASS_COLLECTION       *PHIDCLASS_COLLECTION;
typedef struct _HIDCLASS_DEVICE_EXTENSION *PHIDCLASS_DEVICE_EXTENSION;
typedef struct _HIDCLASS_DRIVER_EXTENSION *PHIDCLASS_DRIVER_EXTENSION;
typedef struct _HIDCLASS_FILE_EXTENSION   *PHIDCLASS_FILE_EXTENSION;
typedef struct _HIDCLASS_PINGPONG         *PHIDCLASS_PINGPONG;
typedef struct _HIDCLASS_REPORT           *PHIDCLASS_REPORT;
typedef struct _FDO_EXTENSION             *PFDO_EXTENSION;
typedef struct _PDO_EXTENSION             *PPDO_EXTENSION;


#if DBG
#define LockFileExtension( f, i )                               \
        {                                                           \
            KeAcquireSpinLock( &(f)->ListSpinLock, (i) );           \
            (f)->ListSpinLockTaken = TRUE;                          \
        }

#define UnlockFileExtension(f, i)                               \
        {                                                           \
            (f)->ListSpinLockTaken = FALSE;                         \
            KeReleaseSpinLock( &(f)->ListSpinLock, (i) );           \
        }

VOID DbgLogIntStart();
VOID DbgLogIntEnd();
#define DBGLOG_INTSTART() DbgLogIntStart()
#define DBGLOG_INTEND() DbgLogIntEnd()

VOID DbgTestGetDeviceString(PFDO_EXTENSION fdoExt);
VOID DbgTestGetIndexedString(PFDO_EXTENSION fdoExt);

#else
#define LockFileExtension(f, i) KeAcquireSpinLock(&(f)->ListSpinLock, (i));
#define UnlockFileExtension(f, i) KeReleaseSpinLock(&(f)->ListSpinLock, (i));

#define DBGLOG_INTSTART()
#define DBGLOG_INTEND()
#endif

#define ALLOCATEPOOL(poolType, size) ExAllocatePoolWithTag((poolType), (size), 'CdiH')

/*
*  String constants for use in compatible-id multi-string.
*/
//                                             0123456789 123456789 1234
#define HIDCLASS_COMPATIBLE_ID_STANDARD_NAME L"HID_DEVICE\0"
#define HIDCLASS_COMPATIBLE_ID_GENERIC_NAME  L"HID_DEVICE_UP:XXXX_U:XXXX\0"
#define HIDCLASS_COMPATIBLE_ID_PAGE_OFFSET  14
#define HIDCLASS_COMPATIBLE_ID_USAGE_OFFSET 21
#define HIDCLASS_COMPATIBLE_ID_STANDARD_LENGTH 11
#define HIDCLASS_COMPATIBLE_ID_GENERIC_LENGTH 26
//                                        0123456789 123456789 123456
#define HIDCLASS_SYSTEM_KEYBOARD        L"HID_DEVICE_SYSTEM_KEYBOARD\0"
#define HIDCLASS_SYSTEM_MOUSE           L"HID_DEVICE_SYSTEM_MOUSE\0"
#define HIDCLASS_SYSTEM_GAMING_DEVICE   L"HID_DEVICE_SYSTEM_GAME\0"
#define HIDCLASS_SYSTEM_CONTROL         L"HID_DEVICE_SYSTEM_CONTROL\0"
#define HIDCLASS_SYSTEM_CONSUMER_DEVICE L"HID_DEVICE_SYSTEM_CONSUMER\0"

#define NO_STATUS 0x80000000    // this will never be a STATUS_xxx constant in NTSTATUS.H

//
// Valid values for HIDCLASS_DEVICE_EXTENSION.state
//
enum deviceState {
	DEVICE_STATE_INITIALIZED,
	DEVICE_STATE_STARTING,
	DEVICE_STATE_START_SUCCESS,
	DEVICE_STATE_START_FAILURE,
	DEVICE_STATE_STOPPED,
	DEVICE_STATE_REMOVING,
	DEVICE_STATE_REMOVED,
	DEVICE_STATE_SUSPENDED
};

enum collectionState {
	COLLECTION_STATE_UNINITIALIZED,
	COLLECTION_STATE_INITIALIZED,
	COLLECTION_STATE_RUNNING,
	COLLECTION_STATE_STOPPED,
	COLLECTION_STATE_REMOVING
};


//
// _HIDCLASS_DRIVER_EXTENSION contains per-minidriver extension information
// for the class driver.  It is created upon a HidRegisterMinidriver() call.
//

typedef struct _HIDCLASS_DRIVER_EXTENSION {

	//
	// Pointer to the minidriver's driver object.
	//

	PDRIVER_OBJECT      MinidriverObject;

	//
	// RegistryPath is a pointer to the minidriver's RegistryPath that it
	// received as a DriverEntry() parameter.
	//

	PUNICODE_STRING     RegistryPath;

	//
	// DeviceExtensionSize is the size of the minidriver's per-device
	// extension.
	//

	ULONG               DeviceExtensionSize;

	//
	// Dispatch routines for the minidriver.  These are the only dispatch
	// routines that the minidriver should ever care about, no others will
	// be forwarded.
	//

	PDRIVER_DISPATCH    MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

	/*
	*  These are the minidriver's original entrypoints,
	*  to which we chain.
	*/
	PDRIVER_ADD_DEVICE  AddDevice;
	PDRIVER_UNLOAD      DriverUnload;

	//
	// Number of pointers to this structure that we've handed out
	//

	LONG                ReferenceCount;

	//
	// Linkage onto our global list of driver extensions
	//

	LIST_ENTRY          ListEntry;


	/*
	*  Either all or none of the devices driven by a given minidriver are polled.
	*/
	BOOLEAN             DevicesArePolled;


#if DBG

	ULONG               Signature;

#endif

} HIDCLASS_DRIVER_EXTENSION;

#if DBG
#define HID_DRIVER_EXTENSION_SIG 'EdiH'
#endif



#define MIN_POLL_INTERVAL_MSEC      1
#define MAX_POLL_INTERVAL_MSEC      10000
#define DEFAULT_POLL_INTERVAL_MSEC  5


/*
*  Device-specific flags
*/
//  Nanao depends on a Win98G bug that allows GetFeature on input collection
#define DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION  (1 << 0)


//
// HIDCLASS_COLLECTION is where we keep our per-collection information.
//

typedef struct _HIDCLASS_COLLECTION {


	ULONG                       CollectionNumber;
	ULONG                       CollectionIndex;

	//
	// DeviceExtension is the pointer to the device extension of which
	// this collection is a member.
	//

	PHIDCLASS_DEVICE_EXTENSION  DeviceExtension;

	//
	// NumOpens is a count of open handles against this collection.
	//

	ULONG                       NumOpens;

	// Number of pending reads for all clients on this collection.
	ULONG                       numPendingReads;

	//
	// FileExtensionList is the head of a list of file extensions, i.e.
	// open instances against this collection.
	//

	LIST_ENTRY                  FileExtensionList;
	KSPIN_LOCK                  FileExtensionListSpinLock;

	/*
	*  For polled devices, we only read from the device
	*  once every poll interval.  We queue read IRPs
	*  here until the poll timer expiration.
	*
	*  Note:  for a polled device, we keep a separate background
	*         loop for each collection.  This way, queued-up read IRPs
	*         remain associated with the right collection.
	*         Also, this will keep the number of reads we do on each
	*         timer period roughly equal to the number of collections.
	*/
	ULONG                       PollInterval_msec;
	KTIMER                      polledDeviceTimer;
	KDPC                        polledDeviceTimerDPC;
	LIST_ENTRY                  polledDeviceReadQueue;
	KSPIN_LOCK                  polledDeviceReadQueueSpinLock;

	/*
	*  We save old reports on polled devices for
	*  "opportunistic" readers who want to get a result right away.
	*  The polledDataIsStale flag indicates that the saved report
	*  is at least one poll interval old (so we should not use it).
	*/
	PUCHAR                      savedPolledReportBuf;
	ULONG                       savedPolledReportLen;
	BOOLEAN                     polledDataIsStale;

	UNICODE_STRING              SymbolicLinkName;
	UNICODE_STRING              SymbolicLinkName_SystemControl;

	/*
	*  HID collection information descriptor for this collection.
	*/
	HID_COLLECTION_INFORMATION  hidCollectionInfo;
	PHIDP_PREPARSED_DATA        phidDescriptor;

	/*
	*  This buffer is used to "cook" a raw report when it's been received.
	*  This is only used for non-polled (interrupt) devices.
	*/
	PUCHAR                      cookedInterruptReportBuf;

	/*
	*  This is an IRP that we queue and complete
	*  when a read report contains a power event.
	*
	*  The powerEventIrp field retains an IRP
	*  so it needs a spinlock to synchronize cancellation.
	*/
	PIRP                        powerEventIrp;
	KSPIN_LOCK                  powerEventSpinLock;


#if DBG
	ULONG                   Signature;
#endif

} HIDCLASS_COLLECTION;

#if DBG
#define HIDCLASS_COLLECTION_SIG 'EccH'
#endif

//
// For HID devices that have at least one interrupt-style collection, we
// try to keep a set of "ping-pong" report-read IRPs pending in the minidriver
// in the event we get a report.
//
// HIDCLASS_PINGPONG contains a pointer to an IRP as well as an event
// and status block.  Each device has a pointer to an array of these structures,
// the array size depending on the number of such IRPs we want to keep in
// motion.
//
// Right now the default number is 2.
//

#define MIN_PINGPONG_IRPS   2

typedef struct _HIDCLASS_PINGPONG {

#define PINGPONG_SIG (ULONG)'gnoP'
	ULONG           sig;

	PIRP    irp;
	PUCHAR  reportBuffer;
	LONG    weAreCancelling;

	KEVENT sentEvent;       // When a read has been sent.
	KEVENT pumpDoneEvent;   // When the read loop is finally exitting.

	PFDO_EXTENSION   myFdoExt;

	/*
	*  Timeout context for back-off algorithm applied to broken devices.
	*/
	KTIMER          backoffTimer;
	KDPC            backoffTimerDPC;
	LARGE_INTEGER   backoffTimerPeriod; // in negative 100-nsec units

} HIDCLASS_PINGPONG;

#if DBG
#define HIDCLASS_REPORT_BUFFER_GUARD    'draG'
#endif


/*
*  Stores information about a Functional Device Object (FDO) which HIDCLASS attaches
*  to the top of the Physical Device Object (PDO) that it get from the minidriver below.
*/
typedef struct _FDO_EXTENSION {

	//
	// Back pointer to the functional device object
	//
	PDEVICE_OBJECT          fdo;

	//
	// HidDriverExtension is a pointer to our driver extension for the
	// minidriver that gave us the PDO.
	//

	PHIDCLASS_DRIVER_EXTENSION driverExt;

	//
	// Hid descriptor that we get from the device.
	//

	HID_DESCRIPTOR          hidDescriptor;  // 9 bytes

											//
											// The attributes of this hid device.
											//

	HID_DEVICE_ATTRIBUTES   hidDeviceAttributes;  // 0x20 bytes

												  //
												  // Pointer to and length of the raw report descriptor.
												  //

	PUCHAR                  rawReportDescription;
	ULONG                   rawReportDescriptionLength;

	//
	// This device has one or more collections.  We store the count and
	// pointer to an array of our HIDCLASS_COLLECTION structures (one per
	// collection) here.
	//

	PHIDCLASS_COLLECTION    classCollectionArray;  // BUGBUG put each of these into pdoExt.

												   /*
												   *  This is initialized for us by HIDPARSE's HidP_GetCollectionDescription().
												   *  It includes an array of HIDP_COLLECTION_DESC structs corresponding
												   *  the classCollectionArray declared above.
												   */
	HIDP_DEVICE_DESC        deviceDesc;     // 0x30 bytes
	BOOLEAN                 devDescInitialized;

	//
	// The maximum input size amongst ALL report types.
	//
	ULONG                   maxReportSize;

	//
	// For devices that have at least one interrupt collection, we keep
	// a couple of ping-pong IRPs and associated structures.
	// The ping-pong IRPs ferry data up from the USB hub.
	//
	ULONG                   numPingPongs;
	PHIDCLASS_PINGPONG      pingPongs;

	//
	// OpenCount represents the number of file objects aimed at this device
	//
	ULONG                   openCount;


	/*
	*  This is the number of IRPs still outstanding in the minidriver.
	*/

	ULONG                   outstandingRequests;

	enum deviceState        state;

	UNICODE_STRING          name;

	/*
	*  deviceRelations contains an array of client PDO pointers.
	*
	*  As the HID bus driver, HIDCLASS produces this data structure to report
	*  collection-PDOs to the system.
	*/
	PDEVICE_RELATIONS       deviceRelations;

	/*
	*  This is an array of device extensions for the collection-PDOs of this
	*  device-FDO.
	*/
	PHIDCLASS_DEVICE_EXTENSION   *collectionPdoExtensions;


	/*
	*  This includes a
	*  table mapping system power states to device power states.
	*/
	DEVICE_CAPABILITIES deviceCapabilities;

	/*
	*  Track current system power state
	*/
	SYSTEM_POWER_STATE  systemPowerState;

	/*
	*  Wait Wake Irp sent to parent PDO
	*/
	PIRP        waitWakeIrp;
	KSPIN_LOCK  waitWakeSpinLock;
	BOOLEAN isWaitWakePending;


	BOOLEAN isOutputOnlyDevice;

	/*
	*  This is a list of WaitWake IRPs sent to the collection-PDOs
	*  on this device, which we just save and complete when the
	*  base device's WaitWake IRP completes.
	*/
	LIST_ENTRY  collectionWaitWakeIrpQueue;
	KSPIN_LOCK  collectionWaitWakeIrpQueueSpinLock;

	struct _FDO_EXTENSION       *nextFdoExt;

	/*
	*  Device-specific flags (DEVICE_FLAG_xxx).
	*/
	ULONG deviceSpecificFlags;

	/*
	*  This is our storage space for the systemState IRP that we need to hold
	*  on to and complete in DevicePowerRequestCompletion.
	*/
	PIRP currentSystemStateIrp;

#if DBG
	WCHAR dbgDriverKeyName[64];
#endif


} FDO_EXTENSION;


/*
*  Stores information about a Physical Device Object (PDO) which HIDCLASS creates
*  for each HID device-collection.
*/
typedef struct _PDO_EXTENSION {

	enum collectionState    state;

	ULONG                   collectionNum;
	ULONG                   collectionIndex;
	PDEVICE_OBJECT          pdo;                // represents a collection on the HID "bus"
	PUNICODE_STRING         name;

	/*
	*  This is a back-pointer to the original FDO's extension.
	*/
	PHIDCLASS_DEVICE_EXTENSION  deviceFdoExt;


	/*
	*  Access protection information.
	*  We count the number of opens for read and write on the collection.
	*  We also count the number of opens which RESTRICT future
	*  read/write opens on the collection.
	*
	*  Note that desired access is independent of restriction.
	*  A client may, for example, do an open-for-read-only but
	*  (by not setting the FILE_SHARE_WRITE bit)
	*  restrict other clients from doing an open-for-write.
	*/
	ULONG                       openCount;
	ULONG                       opensForRead;
	ULONG                       opensForWrite;
	ULONG                       restrictionsForRead;
	ULONG                       restrictionsForWrite;
	ULONG                       restrictionsForAnyOpen;

} PDO_EXTENSION;


/*
*  This contains info about either a device FDO or a device-collection PDO.
*  Some of the same functions process both, so we need one structure.
*/
typedef struct _HIDCLASS_DEVICE_EXTENSION {

	/*
	*  This is the public part of a HID FDO device extension, and
	*  must be the first entry in this structure.
	*/
	HID_DEVICE_EXTENSION    hidExt;     // size== 0x0C.

										/*
										*  Determines whether this is a device extension for a device-FDO or a
										*  device-collection-PDO; this resolves the following union.
										*/
	BOOLEAN                 isClientPdo;

	/*
	*  Include this signature for both debug and retail --
	*  kenray's debug extensions look for this.
	*/
#define             HID_DEVICE_EXTENSION_SIG 'EddH'
	ULONG               Signature;
	 
	union {
		FDO_EXTENSION       fdoExt;
		PDO_EXTENSION       pdoExt;
	};


} HIDCLASS_DEVICE_EXTENSION; 

//
// HIDCLASS_FILE_EXTENSION is private data we keep per file object.
//

typedef struct _HIDCLASS_FILE_EXTENSION {

	//
	// CollectionNumber is the ordinal of the collection in the device
	//

	ULONG                       CollectionNumber;


	PFDO_EXTENSION              fdoExt;

	//
	// PendingIrpList is a list of READ IRPs currently waiting to be satisfied.
	//

	LIST_ENTRY                  PendingIrpList;

	//
	// ReportList is a list of reports waiting to be read on this handle.
	//

	LIST_ENTRY                  ReportList;

	//
	// FileList provides a way to link all of a collection's
	// file extensions together.
	//

	LIST_ENTRY                  FileList;

	//
	// Both PendingIrpList and ReportList are protected by the same spinlock,
	// ListSpinLock.
	//
	KSPIN_LOCK                  ListSpinLock;

	//
	// MaximumInputReportAge is only applicable for polled collections.
	// It represents the maximum acceptable input report age for this handle.
	// There is a value in the HIDCLASS_COLLECTION,
	// CurrentMaximumInputReportAge, that represents the current minimum value
	// of all of the file extensions open against the collection.
	//

	LARGE_INTEGER               MaximumInputReportAge;

	//
	// CurrentInputReportQueueSize is the current size of the report input
	// queue.
	//

	ULONG                       CurrentInputReportQueueSize;

	/*
	*  This is the maximum number of reports that will be queued for the file extension.
	*  This starts at a default value and can be adjusted (within a fixed range) by an IOCTL.
	*/
	ULONG                       MaximumInputReportQueueSize;
#define MIN_INPUT_REPORT_QUEUE_SIZE MIN_PINGPONG_IRPS
#define MAX_INPUT_REPORT_QUEUE_SIZE (MIN_INPUT_REPORT_QUEUE_SIZE*100)
#define DEFAULT_INPUT_REPORT_QUEUE_SIZE (MIN_INPUT_REPORT_QUEUE_SIZE*4)

	//
	// Back pointer to the file object that this extension is for
	//

	PFILE_OBJECT                FileObject;


	/*
	*  File-attributes passed in irpSp->Parameters.Create.FileAttributes
	*  when this open was made.
	*/
	USHORT                      FileAttributes;
	ACCESS_MASK                 accessMask;
	USHORT                      shareMask;

	//
	// Closing is set when this file object is closing and will be removed
	// shortly.  Don't queue any more reports or IRPs to this object
	// when this flag is set.
	//

	BOOLEAN                     Closing;

	//
	// Security has been checked.
	//

	BOOLEAN                     SecurityCheck;

	//
	// DWORD allignment
	//
	BOOLEAN                     Reserved[2];

	/*
	*  This flag indicates that this client does irregular, opportunistic
	*  reads on the device, which is a polled device.
	*  Instead of waiting for the background timer-driven read loop,
	*  this client should have his reads completed immediately.
	*/
	BOOLEAN                     isOpportunisticPolledDeviceReader;
	ULONG                       nowCompletingIrpForOpportunisticReader;


	/*
	*  haveReadPrivilege TRUE indicates that the client has full
	*  permissions on the device, including read.
	*/
	BOOLEAN                     haveReadPrivilege;

	//
	// Memphis Blue Screen info
	//
	BLUESCREEN                  BlueScreenData;


	/*
	*  If a read fails, some clients reissue the read on the same thread.
	*  If this happens repeatedly, we can run out of stack space.
	*  So we keep track of the depth
	*/
#define INSIDE_READCOMPLETE_MAX 4
	ULONG						insideReadCompleteCount;

#if DBG
	BOOLEAN                     ListSpinLockTaken;
	ULONG                       Signature;
#endif

} HIDCLASS_FILE_EXTENSION;

#if DBG
#define HIDCLASS_FILE_EXTENSION_SIG 'efcH'
#endif


typedef struct {

#define ASYNC_COMPLETE_CONTEXT_SIG 'cnsA'
	ULONG sig;

	WORK_QUEUE_ITEM workItem;
	PIRP irp;
	PDEVICE_OBJECT devObj;
} ASYNC_COMPLETE_CONTEXT;


//
// HIDCLASS_REPORT is the structure we use to track a report returned from
// the minidriver.
//

typedef struct _HIDCLASS_REPORT {

	//
	// ListEntry queues this report onto a file extension.
	//

	LIST_ENTRY  ListEntry;

	ULONG reportLength;
	//
	// UnparsedReport is a data area for the unparsed report data as returned
	// from the minidriver.  The lengths of all input reports for a given
	// class are the same, so we don't need to store the length in each
	// report.
	//

	UCHAR       UnparsedReport[];

} HIDCLASS_REPORT;

 

