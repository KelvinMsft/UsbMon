#ifndef _TLIST_H_2WAZXDE456TGNM8907YH23EDC9IUJ0OKM2
#define _TLIST_H_2WAZXDE456TGNM8907YH23EDC9IUJ0OKM2

#include <ntddk.h>

#define   MASKFLAG_LOCK          0x0000000F

#define   LISTFLAG_UNLOCK        0x00000000
#define   LISTFLAG_SPINLOCK      0x00000001
#define   LISTFLAG_FTMUTEXLOCK   0x00000002
#define   LISTFLAG_NLMUTEXLOCK   0x00000003
#define   LISTFLAG_LOCKCALLBCK   0x00000004
#define   LISTFLAG_REF   		 0x00000010
#define   LISTFLAG_AUTOFREE      0x80000000



typedef struct _ListComm
{
	struct _ListComm* Next;
	struct _ListComm* Prev;
}TListComm,*PListComm;

typedef struct _ListCell
{
	struct _ListCell* 	Next;
	struct _ListCell* 	Prev;
	void*  				Pointer;
}TListCell,*PListCell;


#define LISTBLOCK_NOTFREE  0x00000001

typedef struct _ListCellBlock
{
	struct _ListCellBlock* 	Next;
	struct _ListCellBlock* 	Prev;
	ULONG					Flag;
	ULONG					MaxCount;
	ULONG					Count;
	TListCell				ListCell[1];
}TListCellBlock,*PListCellBlock;


#define   CLIST_FINDCB_BKMASK 0x000000FF
#define   CLIST_FINDCB_CTN 	  0x00000000
#define   CLIST_FINDCB_RET 	  0x00000001
#define   CLIST_FINDCB_DEL 	  0x80000000

#define   CLIST_ACTION_TYPEMASK     0x0000000F 
#define   CLIST_ACTION_REF	        0x00000001
#define   CLIST_ACTION_DEF	        0x00000002
#define   CLIST_ACTION_FREE	        0x00000003
#define   CLIST_ACTION_LOCKENTRY    0x00000010
#define   CLIST_ACTION_LOCKLEAVE	0x00000020

typedef ULONG   (__fastcall *TQueryChainListCallback)(void* Data,void* Context);
typedef ULONG   (__fastcall *TChainLisActionCallback)(void* Data,ULONG Act);


typedef struct 
{
	ULONG		             Version;
	ULONG		             Flag;
	ULONG 		             Count;
	PListCell                ListHeader;
	PListCellBlock           BlockHeader;
	
	TChainLisActionCallback  ChainLisActionCallback;
	ULONG				     ReferenceOffSet;
	ULONG     			     LockType;
	union
	{
		struct
		{
			KSPIN_LOCK   SpinLock;
			KIRQL		 Irql;
		};
		struct
		{
			 FAST_MUTEX  FastMutex;
		};
		struct 
		{
			 KMUTEX      Mutex; 
		};
	};
	ULONG   		   HashDataOffset;	
	ULONG			   HashListSize;//保留，后在加入hash的快速查找
	PListCell		   CellHashList;			   
	char*			   Buffer;
	ULONG			   RefCount;//用于交验访问数据前是否有做加锁处理。	
	ULONG              Reserved[16];
}TChainListHeader,*PChainListHeader;

#define  CHAINLIST_SAFE_FREE(header) { \
	PChainListHeader beFreeObj=(header);\
	(header) = NULL;\
	FreeChainListHeader(beFreeObj);\
	beFreeObj=NULL;\
    }



PChainListHeader     NewChainListHeaderEx(ULONG Flag,TChainLisActionCallback  ChainLisActionCallback,ULONG ReferenceOffSet);
PChainListHeader     NewChainListHeader();
void  			     FreeChainListHeader(PChainListHeader List);
BOOLEAN 		     AddToChainListTail(PChainListHeader List,void* Data);
BOOLEAN 		     InsertToChainList(PChainListHeader List,ULONG Index,void* Data);
BOOLEAN  		     DelFromChainListByPointer(PChainListHeader List,void* Data);
BOOLEAN              DelFromChainListByIndex(PChainListHeader List,ULONG Index);
BOOLEAN              DelFromChainList(PChainListHeader List,PListCell delCell);
BOOLEAN              DelNoneFromChainList(PChainListHeader List,BOOLEAN isTail);

void* 			     QueryFromChainListByIndex(PChainListHeader List,ULONG Index);
void* 			     QueryFromChainListByULONG(PChainListHeader List,ULONG ID);
void* 			     QueryFromChainListByULONG64(PChainListHeader List,ULONGLONG ID);
void* 			     QueryFromChainListByULONGPTR(PChainListHeader List,ULONG_PTR ID);
void*                QueryFromChainListByMem(PChainListHeader List,ULONG Offset,void* Mem,ULONG Length);
void*                QueryFromChainListByMemEx(PChainListHeader List,ULONG Offset,void* Mem,ULONG Length,ULONG Flag);
void* 			     QueryFromChainListByCallback(PChainListHeader List,TQueryChainListCallback Callback,void* Context);
ULONG 			     GetChainListCount(PChainListHeader List);
BOOLEAN              DefChainListPointer(PChainListHeader List,void* Data);




























#endif