#include "OpenLoopBuffer.h"
 

#pragma intrinsic(_ReturnAddress) 
CIRCULARBUFFER* NewOpenLoopBuffer(
	_In_ ULONG MaxCount, 
	_In_ ULONG CellSize,
	_In_ ULONG Flags)
{
	char* Buffer = NULL;
	CIRCULARBUFFER* cb;
	CIRCULARBUFFER* header=NULL;
	ULONG memsize= MaxCount * CellSize + sizeof(CIRCULARBUFFER);
	 
	memsize = BYTES_TO_PAGES(memsize) * 0x1000;
	Buffer  = (char*)ExAllocatePoolWithTag(NonPagedPool, memsize, 'cbne');
	
	if(!Buffer)
	{
		return NULL;
	}
	
	RtlZeroMemory(Buffer, memsize);

	cb = (CIRCULARBUFFER*)Buffer;

	cb->Flags=Flags;
	cb->MaxCount = MaxCount;
	cb->CellSize = CellSize;
	cb->LatestIndex = 0 ;
	cb->WriteIndex=0;
	cb->WriteRef=0;
	cb->Buffer=cb->Data;
	
	if(Flags&OPENLOOPBUFF_FALGS_PEASUDOHEADER)
	{
		header=(CIRCULARBUFFER*)ExAllocatePoolWithTag(NonPagedPool,sizeof(CIRCULARBUFFER) , 'cbne');
		if(!header)
		{
			ExFreePool(cb);
			cb= NULL;
			return cb;
		}
		
		memcpy(header,cb,sizeof(CIRCULARBUFFER));
		
		cb->Buffer=NULL;
		header->Header=cb;
		header->Flags=Flags;
		
		return header;
	}

	return cb;
}
 
//--------------------------------------------------------------------------------------------//
void  OpenLoopBufferWrite(
	_In_ CIRCULARBUFFER* cb, 
	_In_ const void *item)
{
	ULONG Index = 0; 

	if(!cb || !cb->Header)
	{
		return ;	
	}
	
	InterlockedIncrement(&cb->WriteRef);

	Index = cb->WriteIndex % cb->MaxCount;
	
	memcpy(cb->Buffer + Index * cb->CellSize, item, cb->CellSize);
	
	cb->WriteIndex = InterlockedAdd64((LONG64 volatile*)&cb->WriteIndex, 1);
	
	if (InterlockedDecrement(&cb->WriteRef) == 0)
	{		
		InterlockedExchange64((LONG64 volatile*)&cb->LatestIndex, cb->WriteIndex); 
		if(cb->Flags&OPENLOOPBUFF_FALGS_PEASUDOHEADER)
		{
			cb->Header->MaxCount=cb->MaxCount;
			cb->Header->CellSize=cb->CellSize;
			cb->Header->WriteIndex=cb->WriteIndex;
			cb->Header->LatestIndex=cb->LatestIndex;
			
			USB_DEBUG_INFO_LN_EX("_ReturnAddress: %p KernelModeIndex: %d WriteIndex: %I64u LatestIndex: %I64u ", _ReturnAddress(), Index, cb->WriteIndex, cb->LatestIndex );
		}	

	}
}

//------------------------------------------------//
ULONG64  OpenLoopBufferRead(
	_In_	CIRCULARBUFFER* cb, 
	_In_	PVOID			RequestedBuffer,  
	_In_	ULONG			RequestedCount,
	_In_	ULONG64			TargetIndex, 
	_Inout_ ULONG64*		LastIndex
)
{
	ULONG64 i = 0;
	ULONG64 base = 0;
	ULONG64 Offset = 0;	
	ULONG64 WritecacheLen = 0;
	
	if(!cb || !cb->Header)
	{
		return 0;	
	}
	
	if (TargetIndex < cb->LatestIndex)
	{
		base = InterlockedAdd64((LONG64 volatile*)&cb->LatestIndex, 0) - 1;
		Offset = base - TargetIndex;
		WritecacheLen = InterlockedAdd64((LONG64 volatile*)&cb->WriteIndex, 0) - InterlockedAdd64((LONG64 volatile*)&cb->LatestIndex, 0);

		if (WritecacheLen > cb->MaxCount / 2)
		{
			WritecacheLen = cb->MaxCount / 2;
		}
		else if(WritecacheLen<8)
		{
			WritecacheLen = 8;
		}

		if (Offset > cb->MaxCount - 1- WritecacheLen)
		{
			Offset = cb->MaxCount - 1- WritecacheLen;
		}
		if (Offset > base)
		{
			Offset = base;
		}

		for ( i = 0; i <= Offset && i < RequestedCount ; i++)
		{
			ULONG Index = (ULONG)(( base - Offset + i ) % cb->MaxCount); 
			  
			memcpy((PUCHAR)RequestedBuffer+i*cb->CellSize, cb->Buffer+Index*cb->CellSize, cb->CellSize);

			/*printf(" base: %d Offset: %d j: %d SerialCode: %d g_InfoDataIndex: %d\r\n",
				base, Offset, Index, TargetIndex, cb->LatestIndex); 
				*/
		}
		*LastIndex = base - Offset + i ;
		if (*LastIndex == 0)
		{
			USB_DEBUG_INFO_LN_EX(" base: %d Offset: %d i: %d TargetIndex: %d LatestIndex: %d\r\n",
				base, Offset,  i , TargetIndex, cb->LatestIndex);
		}
	}

	return i;
}

//------------------------------------------------//
VOID   OpenLoopBufferRelease(
	_In_	CIRCULARBUFFER* cb
)
{
	PVOID CBufferHeader = NULL;
	if(!cb)
	{
		return ; 
	}
	
	CBufferHeader = cb->Header;
	ExFreePool(cb);
	cb = NULL;
	
	if(!CBufferHeader)
	{
		return ;
	}
	
	ExFreePool(CBufferHeader);
	CBufferHeader = NULL;  
}