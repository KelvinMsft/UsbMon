#include <fltKernel.h> 
#include "CommonUtil.h"

#pragma pack (push, 4)
typedef struct circular_buffer
{
	ULONG    		Ver;
	ULONG    		Flags;
	ULONG	 		MaxCount;		// maximum number of items in the buffer
	ULONG	 		CellSize;		// size of each item in the buffer  
	ULONG64  		LatestIndex;
	ULONG64  		WriteIndex;
	ULONG    		WriteRef;
	union {
		ULONG64 			   iHeader;
		struct circular_buffer* Header;
	};
	union {
		ULONG64		   iBuffer;
		PUCHAR   		Buffer;
	};

	UCHAR	 		Data[4];     // data buffer 
} CIRCULARBUFFER, *PCIRCULARBUFFER;
#pragma pack (pop)


#define OPENLOOPBUFF_FALGS_PEASUDOHEADER  0x80000000


CIRCULARBUFFER* NewOpenLoopBuffer(
	_In_ ULONG MaxCount,
	_In_ ULONG CellSize,
	_In_ ULONG Flags
);


//-------------------------------------------------------------------------------------------//
void OpenLoopBufferWrite(
	_In_ CIRCULARBUFFER* cb,
	_In_ const void *item
);

//------------------------------------------------//
ULONG64 OpenLoopBufferRead(
	_In_	CIRCULARBUFFER* cb,
	_In_	PVOID			RequestedBuffer,
	_In_	ULONG			RequestedCount,
	_In_	ULONG64			TargetIndex,
	_Inout_ ULONG64*		LastIndex
);