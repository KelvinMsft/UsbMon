#include "TList.h"
 

#ifndef PAGE_SIZE
#define PAGE_SIZE  4096
#endif

void EntryListLock(PChainListHeader List);
void LeaveListLock(PChainListHeader List);
void FreeListCell(PChainListHeader List,PListCell Cell);
ULONG OnRefListCell(PChainListHeader List,PListCell Cell);
ULONG OnDefListCell(PChainListHeader List,PListCell Cell);

//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
PChainListHeader NewChainListHeaderEx(ULONG Flag,TChainLisActionCallback  ChainLisActionCallback,ULONG ReferenceOffSet)
{
	PChainListHeader List;
	ULONG       mmLen;
	
	mmLen=sizeof(TChainListHeader)-sizeof(TListCellBlock)+sizeof(TListCell)*16;
	mmLen=mmLen+((PAGE_SIZE-(mmLen%PAGE_SIZE))%PAGE_SIZE);
	
	List=(PChainListHeader)ExAllocatePoolWithTag(NonPagedPool,mmLen,'MMGR');
	if(List)
	{
		PListCellBlock BlockHeader;
		ULONG					 i;
			
		memset(List,0,PAGE_SIZE);
		
		List->Version=1;
		List->Flag=Flag;
		List->Count=0;
		List->ListHeader=NULL;
		List->BlockHeader=NULL;
		List->ChainLisActionCallback=ChainLisActionCallback;
		List->LockType=Flag&MASKFLAG_LOCK;
		List->ReferenceOffSet=ReferenceOffSet;
		List->RefCount=0;
		
		InterlockedExchange(&List->RefCount,0);
		

		BlockHeader=(PListCellBlock)((PCHAR)List+sizeof(TChainListHeader));
		BlockHeader->Flag=LISTBLOCK_NOTFREE;
		BlockHeader->Next=BlockHeader;
		BlockHeader->Prev=BlockHeader;
		BlockHeader->MaxCount=((mmLen-sizeof(TChainListHeader)-sizeof(TListCellBlock))/sizeof(TListCell))+1;
		BlockHeader->Count=1;	
		
		List->BlockHeader=BlockHeader;
		
		if(List->LockType==LISTFLAG_SPINLOCK)
		{
			KeInitializeSpinLock(&List->SpinLock);
		}
		else if(List->LockType==LISTFLAG_FTMUTEXLOCK)
		{
			ExInitializeFastMutex(&List->FastMutex);
		}
		else if(List->LockType==LISTFLAG_NLMUTEXLOCK)
		{
			KeInitializeMutex(&List->Mutex,0);
		}
		
		List->ListHeader=&BlockHeader->ListCell[0];
		List->ListHeader->Next=List->ListHeader;
		List->ListHeader->Prev=List->ListHeader;
	}

	return List;
}
//------------------------------------------------------------------------------
PChainListHeader NewChainListHeader()
{
		return NewChainListHeaderEx(LISTFLAG_SPINLOCK,NULL,0);
}
//------------------------------------------------------------------------------
void  FreeChainListHeader(PChainListHeader List)
{
	if(List)
	{
			PListCellBlock  BlockHeader,bDelBlock;
			PListCell		ListHeader,ListCell,beDelCell;
			
			EntryListLock(List);
			
			ListHeader=List->ListHeader;
			ListCell=ListHeader->Next;
			
			while(ListCell!=ListHeader)
			{
	            beDelCell=ListCell;
	            ListCell=ListCell->Next;
				OnDefListCell(List,beDelCell);
	            FreeListCell(List,beDelCell);
			}
			
			BlockHeader=List->BlockHeader;
			for(;;)
			{
       			bDelBlock=BlockHeader;
      		 	BlockHeader=BlockHeader->Next;
				if(!(bDelBlock->Flag&LISTBLOCK_NOTFREE))
				{
						ExFreePool(bDelBlock);
				}
				if(BlockHeader==List->BlockHeader)
				{
					break;
				}
			}
			LeaveListLock(List);
			
			ExFreePool(List);
	}
	return;
}
//------------------------------------------------------------------------------
static void  EntryListLock(PChainListHeader List)
{

	if(List->LockType==LISTFLAG_SPINLOCK)
	{
		KeAcquireSpinLock(&List->SpinLock,&List->Irql);	
	}
	else if(List->LockType==LISTFLAG_FTMUTEXLOCK)
	{
		ExAcquireFastMutex(&List->FastMutex);
	}
	else if(List->LockType==LISTFLAG_NLMUTEXLOCK)
	{
		KeWaitForSingleObject(&List->Mutex,Executive,KernelMode,FALSE,NULL);
	}
	else if(List->LockType==LISTFLAG_LOCKCALLBCK)
	{
		List->ChainLisActionCallback(List,CLIST_ACTION_LOCKENTRY);
	}
	
	if(InterlockedIncrement(&List->RefCount)>1)
	{
		//说明访问链表没有做同步
		//暂时去掉，如果使用外边读写锁，是会有问题的
		//KeBugCheckEx(BUGCHECK_MODULEID_COMMM,CHAINLIST_LOCKERROR_REF,0,0,0);
		
	}
	
}
//------------------------------------------------------------------------------
static  void LeaveListLock(PChainListHeader List)
{
	if(InterlockedDecrement(&List->RefCount)<0)
	{
		//暂时去掉，如果使用外边读写锁，是会有问题的
		//KeBugCheckEx(BUGCHECK_MODULEID_COMMM,CHAINLIST_LOCKERROR_DEF,0,0,0);
	}
	
	if(List->LockType==LISTFLAG_SPINLOCK)
	{
		KeReleaseSpinLock(&List->SpinLock,List->Irql);
	}
	else if(List->LockType==LISTFLAG_FTMUTEXLOCK)
	{
		ExReleaseFastMutex(&List->FastMutex);
	}
	else if(List->LockType==LISTFLAG_NLMUTEXLOCK)
	{
		KeReleaseMutex(&List->Mutex,FALSE);
	}
	else if(List->LockType==LISTFLAG_LOCKCALLBCK)
	{
		List->ChainLisActionCallback(List,CLIST_ACTION_LOCKLEAVE);
	}
	
}
//------------------------------------------------------------------------------
static void AddToListComm(PListComm S,PListComm D)
{
	D->Next=S;
	D->Prev=S->Prev;
	
	S->Prev->Next=D;
	S->Prev=D;
}
//------------------------------------------------------------------------------
static void InsertToListComm(PListComm S,PListComm D)
{
    D->Next=S;
    D->Prev=S->Prev;

    S->Prev->Next=D;
    S->Prev=D;
}
//------------------------------------------------------------------------------
static void DelListComm(PListComm D)
{
	D->Prev->Next=D->Next;
	D->Next->Prev=D->Prev;
	
	D->Next=NULL;
	D->Prev=NULL;
}
//------------------------------------------------------------------------------
static PListCell AllocListCell(PChainListHeader List)
{
	PListCellBlock BlockHeader;
	PListCell			 ListCell;
	PListCell			 Ret=NULL;
	ULONG					 i;
	
	BlockHeader=List->BlockHeader;
	for(;;)
	{
		if(BlockHeader->Count<BlockHeader->MaxCount)
		{
				ListCell=BlockHeader->ListCell;
				for(i=0;i<BlockHeader->MaxCount;i++)
				{
						if(ListCell[i].Next==NULL&&ListCell[i].Prev==NULL)
						{
								break;
						}
				}
				if(i<BlockHeader->MaxCount)
				{
					BlockHeader->Count++;
					Ret=&ListCell[i];
				}
		}
		BlockHeader=BlockHeader->Next;
		if(BlockHeader==List->BlockHeader)
		{
			break;
		}
		
	}
	if(!Ret)
	{
		BlockHeader=(PListCellBlock)ExAllocatePoolWithTag(NonPagedPool,PAGE_SIZE,'MMGR');
		if(BlockHeader)
		{
				memset(BlockHeader,0,sizeof(TListCellBlock));
				BlockHeader->Flag=0;
				BlockHeader->MaxCount=((PAGE_SIZE-sizeof(TListCellBlock))/sizeof(TListCell))+1;
				BlockHeader->Count=1;
				AddToListComm((PListComm)List->BlockHeader,(PListComm)BlockHeader);
				Ret=&BlockHeader->ListCell[0];
		}
	}
	return Ret;
}
//------------------------------------------------------------------------------
static void FreeListCell(PChainListHeader List,PListCell Cell)
{
	PListCellBlock  BlockHeader;
	PListCell		Ret=NULL;
	ULONG			i;
	
	BlockHeader=List->BlockHeader;
	while(TRUE)
	{
		if(BlockHeader->Count&&Cell>=BlockHeader->ListCell&&Cell<BlockHeader->ListCell+BlockHeader->MaxCount)
		{
			if(!(List->Flag&LISTFLAG_REF))
			{
				if(List->ChainLisActionCallback)
				{
					List->ChainLisActionCallback(Cell->Pointer,CLIST_ACTION_FREE);
				}
				else if(Cell->Pointer&&(List->Flag&LISTFLAG_AUTOFREE))
		 		{
		 			ExFreePool(Cell->Pointer);
		 		}
	 		}
	 		
			Cell->Next=NULL;
			Cell->Prev=NULL;
			Cell->Pointer=NULL;
			BlockHeader->Count--;
			
			if(BlockHeader->Count==0&&(!(BlockHeader->Flag&LISTBLOCK_NOTFREE)))
			{
					DelListComm((PListComm)BlockHeader);
					ExFreePool(BlockHeader);
					BlockHeader=NULL;
			}
			break;
		}
		
		BlockHeader=BlockHeader->Next;
		if(BlockHeader==List->BlockHeader)
		{
			break;
		}
	}
	return;
}
//------------------------------------------------------------------------------
static PListCell GetListCellByIndex(PChainListHeader List,ULONG Index)
{
	ULONG i;
	PListCell Cell=List->ListHeader;
	for(i=0;;i++)
	{
			Cell=Cell->Next;
			if(Cell==List->ListHeader)
			{
				Cell=NULL;
				break;
			}
			if(i>=Index)
			{
				break;
			}
	}
	return Cell;
} 
//------------------------------------------------------------------------------
static PListCell GetListCellByData(PChainListHeader List,void* Data)
{
	ULONG i;
	PListCell Cell=List->ListHeader;
	for(i=0;;i++)
	{
		Cell=Cell->Next;
		if(Cell==List->ListHeader)
		{
			Cell=NULL;
			break;
		}
		if(Cell->Pointer==Data)
		{
			break;
		}
	}
	return Cell;
} 
//------------------------------------------------------------------------------
static ULONG IniRefListCell(PChainListHeader List,PListCell Cell)
{
	if(List->Flag&LISTFLAG_REF)
	{
		long Ret=InterlockedExchange((PLONG)((char*)Cell->Pointer+List->ReferenceOffSet),1);
		List->ChainLisActionCallback(Cell->Pointer,CLIST_ACTION_REF);
		return Ret;
	}
	return 0;
}
//------------------------------------------------------------------------------
static ULONG OnRefListCell(PChainListHeader List,PListCell Cell)
{
	if(List->Flag&LISTFLAG_REF)
	{
		long Ret=InterlockedIncrement((PLONG)((char*)Cell->Pointer+List->ReferenceOffSet));
		List->ChainLisActionCallback(Cell->Pointer,CLIST_ACTION_REF);
		return Ret;
	}
	return 0;
}
//------------------------------------------------------------------------------
static ULONG OnDefListCell(PChainListHeader List,PListCell Cell)
{
	if(List->Flag&LISTFLAG_REF)
	{
		long Ret=(long)InterlockedDecrement((PLONG)((char*)Cell->Pointer+List->ReferenceOffSet));
		List->ChainLisActionCallback(Cell->Pointer,CLIST_ACTION_DEF);
		if(Ret<0)
		{
			DbgPrint("OnDefListCell Error\n");
		}
		if(Ret==0)
		{
			List->ChainLisActionCallback(Cell->Pointer,CLIST_ACTION_FREE);
			Cell->Pointer=NULL;
		}
		return Ret;
	}
	return 1;
}
//------------------------------------------------------------------------------
static void DelAndFreeListCell(PChainListHeader List,PListCell Cell)
{
	  DelListComm((PListComm)Cell);
	  OnDefListCell(List,Cell);
	  FreeListCell(List,Cell);
}
//------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
BOOLEAN AddToChainListTail(PChainListHeader List,void* Data)
{
	PListCell Cell;
	BOOLEAN 	Ret=FALSE;
	if(!List)
	{
			return FALSE;
	}
	EntryListLock(List);
	Cell=AllocListCell(List);
	if(Cell)
	{
			Cell->Pointer=Data;
			AddToListComm((PListComm)List->ListHeader,(PListComm)Cell);
			IniRefListCell(List,Cell);
			List->Count++;
			Ret=TRUE;
	}
	LeaveListLock(List);	
	return Ret;
}
//------------------------------------------------------------------------------
BOOLEAN InsertToChainList(PChainListHeader List,ULONG Index,void* Data)
{
	PListCell Cell,Header;
	BOOLEAN Ret=FALSE;
	if(!List)
	{
			return FALSE;
	}
	EntryListLock(List);
	Header=GetListCellByIndex(List,Index);
	Cell=AllocListCell(List);
	if(Cell)
	{
		Cell->Pointer=Data;
	    if(!Header)
	    {
	            AddToListComm((PListComm)List->ListHeader,(PListComm)Cell);
	    }
	    else
	    {
	            InsertToListComm((PListComm)Header,(PListComm)Cell);
	    }
	    IniRefListCell(List,Cell);

		List->Count++;
		Ret=TRUE;
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
BOOLEAN  DelFromChainListByPointer(PChainListHeader List,void* Data)
{
	PListCell Cell,Header;
	BOOLEAN Ret=FALSE;

	if(!List)
	{
		return Ret;
	}

	EntryListLock(List);
	Header=List->ListHeader;
	for(Cell=Header->Next;Cell!=Header;Cell=Cell->Next)
	{
		 if(Cell->Pointer==Data)
		 {
		 		DelAndFreeListCell(List,Cell);

		 		List->Count--;

		 		Ret=TRUE;
		 		break;
		 }
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
BOOLEAN  DelFromChainListByIndex(PChainListHeader List,ULONG Index)
{
	PListCell Cell,Header;
        ULONG    i;
	BOOLEAN Ret=FALSE;


	if(!List)
	{
		return Ret;
	}

	EntryListLock(List);
	Header=List->ListHeader;
	Cell=Header->Next;
	for(i=0;Cell!=Header&&i<Index;i++)
	{
		Cell=Cell->Next;
	}
	if(i>=Index&&Cell!=Header)
	{
	  DelAndFreeListCell(List,Cell);
	
	  List->Count--;
	
	  Ret=TRUE;
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
BOOLEAN DelFromChainList(PChainListHeader List,PListCell delCell)
{
	PListCell Cell,Header;
	BOOLEAN Ret=FALSE;

	if(!List)
	{
		return Ret;
	}

	EntryListLock(List);
	Header=List->ListHeader;
	for(Cell=Header->Next;Cell!=Header;Cell=Cell->Next)
	{
		 if(Cell==delCell)
		 {
		 		DelAndFreeListCell(List,Cell);

		 		List->Count--;

		 		Ret=TRUE;
		 		break;
		 }
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
BOOLEAN DelNoneFromChainList(PChainListHeader List,BOOLEAN isTail)
{
	PListCell Cell,Header;
	BOOLEAN Ret=FALSE;

	if(!List)
	{
		return Ret;
	}

	EntryListLock(List);
	Header=List->ListHeader;
	if(List->Count)
	{
		if(isTail)
		{
			Cell=Header->Prev;
		}
		else 
		{
			Cell=Header->Next;
		}
		if(Cell!=Header)
		{
	 		DelAndFreeListCell(List,Cell);
	 		Cell=NULL;
	 		List->Count--;
	 		Ret=TRUE;
		 }
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
void* QueryFromChainListByIndex(PChainListHeader List,ULONG Index)
{
	PListCell Cell;
	void*			Ret=NULL;
	if(List)
	{
		EntryListLock(List);
		Cell=GetListCellByIndex(List,Index);
		if(Cell)
		{
			 Ret=Cell->Pointer;
			 OnRefListCell(List,Cell);
		}
		LeaveListLock(List);
	}
	return Ret;
}
//------------------------------------------------------------------------------
void* QueryFromChainListByULONG(PChainListHeader List,ULONG ID)
{
	PListCell Cell,Header;
	void*		  Ret=NULL;
	if(!List)
	{
			return NULL;
	}
	EntryListLock(List);
	Header=List->ListHeader;
	for(Cell=Header->Next;Cell!=Header;Cell=Cell->Next)
	{
		 if(*(ULONG*)Cell->Pointer==ID)
		 {
		 		OnRefListCell(List,Cell);
		 		Ret=Cell->Pointer;
		 		break;
		 }
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
void* QueryFromChainListByULONG64(PChainListHeader List,ULONGLONG ID)
{
	PListCell Cell,Header;
	void*		  Ret=NULL;
	if(!List)
	{
			return NULL;
	}
	EntryListLock(List);
	Header=List->ListHeader;
	for(Cell=Header->Next;Cell!=Header;Cell=Cell->Next)
	{
		 if(*(ULONGLONG*)Cell->Pointer==ID)
		 {
		 		OnRefListCell(List,Cell);
		 		Ret=Cell->Pointer;
		 		break;
		 }
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
void* QueryFromChainListByULONGPTR(PChainListHeader List,ULONG_PTR ID)
{
	PListCell Cell,Header;
	void*		  Ret=NULL;
	if(!List)
	{
			return NULL;
	}
	EntryListLock(List);
	Header=List->ListHeader;
	for(Cell=Header->Next;Cell!=Header;Cell=Cell->Next)
	{
		 if(*(ULONG_PTR*)Cell->Pointer==ID)
		 {
		 		OnRefListCell(List,Cell);
		 		Ret=Cell->Pointer;
		 		break;
		 }
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
void* QueryFromChainListByMemEx(PChainListHeader List,ULONG Offset,void* Mem,ULONG Length,ULONG Flag)
{
	PListCell Cell,Header;
	void*		  Ret=NULL;
	if(!List)
	{
			return NULL;
	}
	EntryListLock(List);
	Header=List->ListHeader;
	for(Cell=Header->Next;Cell!=Header;Cell=Cell->Next)
	{
		 if(RtlCompareMemory((char*)Cell->Pointer+Offset,Mem,Length)==Length)
		 {
		 		OnRefListCell(List,Cell);
		 		Ret=Cell->Pointer;
		 		if(Flag&CLIST_FINDCB_DEL)
		 		{
		 			DelAndFreeListCell(List,Cell);
					List->Count--;
		 		}
		 		break;
		 }
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
void* QueryFromChainListByMem(PChainListHeader List,ULONG Offset,void* Mem,ULONG Length)
{
	return QueryFromChainListByMemEx(List,Offset,Mem,Length,0);
}
//------------------------------------------------------------------------------
void* QueryFromChainListByCallback(PChainListHeader List,TQueryChainListCallback Callback,void* Context)
{
	PListCell 	  Cell,Header;
	void*		  Ret=NULL;
	if(!List)
	{
			return NULL;
	}
	EntryListLock(List);
	Header=List->ListHeader;
	for(Cell=Header->Next;Cell!=Header;Cell=Cell->Next)
	{
		ULONG   Error=Callback(Cell->Pointer,Context);
		void*	Pointer=Cell->Pointer;
		if(Error&CLIST_FINDCB_DEL)
		{
			PListCell beDel=Cell;
			Cell=Cell->Prev;
			DelAndFreeListCell(List,beDel);
			List->Count--;
		}
		if((Error&CLIST_FINDCB_BKMASK)==CLIST_FINDCB_RET)
		{
			if(!(Error&CLIST_FINDCB_DEL))
			{
				OnRefListCell(List,Cell);	
			}
			Ret=Pointer;
			break;
		}
	}
	LeaveListLock(List);
	return Ret;
}
//------------------------------------------------------------------------------
ULONG GetChainListCount(PChainListHeader List)
{
	ULONG Count=0;
	if(List)
	{
		EntryListLock(List);
		Count=List->Count;
		LeaveListLock(List);
	}
	return Count;
}
//------------------------------------------------------------------------------
BOOLEAN DefChainListPointer(PChainListHeader List,void* Data)
{
	if(List&&Data&&(List->Flag&LISTFLAG_REF))
	{
		long Ret;
		
		EntryListLock(List);
		
		Ret=InterlockedDecrement((PLONG)((char*)Data+List->ReferenceOffSet));
		List->ChainLisActionCallback(Data,CLIST_ACTION_DEF);
		if(Ret<0)
		{
			DbgPrint("OnDefListCell Error\n");
		}
		if(Ret==0)
		{
			PListCell Cell=GetListCellByData(List,Data);
			if(Cell)
			{
				DelListComm((PListComm)Cell);
				List->Count--;

				List->ChainLisActionCallback(Data,CLIST_ACTION_FREE);
				FreeListCell(List,Cell);
			}
			else 
			{
				List->ChainLisActionCallback(Data,CLIST_ACTION_FREE);
			}
		}
		
		LeaveListLock(List);
		
		return TRUE;
	}
	return FALSE;
}
//------------------------------------------------------------------------------