/*
 * dyn_block_management.c
 *
 *  Created on: Sep 21, 2022
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

uint32 address = 0  ;
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// PRINT MEM BLOCK LISTS:
//===========================

void print_mem_block_lists()
{
	cprintf("\n=========================================\n");
	struct MemBlock* blk ;
	struct MemBlock* lastBlk = NULL ;
	cprintf("\nFreeMemBlocksList:\n");
	uint8 sorted = 1 ;
	LIST_FOREACH(blk, &FreeMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nFreeMemBlocksList is NOT SORTED!!\n") ;

	lastBlk = NULL ;
	cprintf("\nAllocMemBlocksList:\n");
	sorted = 1 ;
	LIST_FOREACH(blk, &AllocMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nAllocMemBlocksList is NOT SORTED!!\n") ;
	cprintf("\n=========================================\n");

}

//********************************************************************************//
//********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//===============================
// [1] INITIALIZE AVAILABLE LIST:
//===============================
void initialize_MemBlocksList(uint32 numOfBlocks)
{
	LIST_INIT(&AvailableMemBlocksList);
	for(uint32 i =0 ; i < numOfBlocks ; i++){
//			MemBlockNodes[i].size =0 ;
//			MemBlockNodes[i].sva = 0 ;
			LIST_INSERT_HEAD(&AvailableMemBlocksList , &(MemBlockNodes[i])) ;
	}
}

//===============================
// [2] FIND BLOCK:
//===============================
struct MemBlock *find_block(struct MemBlock_List *blockList, uint32 va)
{

	struct MemBlock *element;
	LIST_FOREACH(element, blockList)
	{
		if(element->sva == va ){
			return element;
		}
	}
	return NULL ;
}

//=========================================
// [3] INSERT BLOCK IN ALLOC LIST [SORTED]:
//=========================================
void insert_sorted_allocList(struct MemBlock *blockToInsert)
{
	struct MemBlock *first_element =LIST_FIRST(&AllocMemBlocksList) ;
	struct MemBlock *last_element =LIST_LAST(&AllocMemBlocksList) ;
	if(LIST_EMPTY(&AllocMemBlocksList)==1 ){
		LIST_INSERT_HEAD(&AllocMemBlocksList, blockToInsert) ;
	}else if (first_element->sva > blockToInsert->sva){
		LIST_INSERT_HEAD(&AllocMemBlocksList, blockToInsert) ;
	}else if (last_element->sva < blockToInsert->sva){
		LIST_INSERT_TAIL(&AllocMemBlocksList, blockToInsert) ;
	}else {
		struct MemBlock *element;
		LIST_FOREACH(element, &(AllocMemBlocksList))
		{
			if(element->sva > blockToInsert->sva ){
				LIST_INSERT_BEFORE(&AllocMemBlocksList,element,blockToInsert);
				break;
			}
		}

	}
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
struct MemBlock *alloc_block_FF(uint32 size)
{
	 struct MemBlock *ReturnBlock;
	 LIST_FOREACH (ReturnBlock, &(FreeMemBlocksList))
	 {
		  if(size == ReturnBlock->size){
		 LIST_REMOVE(&(FreeMemBlocksList), ReturnBlock);

		 return ReturnBlock;

		 }
		 else if (size < ReturnBlock->size )
		 {
			 struct MemBlock *search = LIST_FIRST(&AvailableMemBlocksList);
			 LIST_REMOVE(&AvailableMemBlocksList,search);

			  search->size=size;
			  search->sva= ReturnBlock->sva;
			  ReturnBlock->size  = ReturnBlock->size - size;
			  ReturnBlock -> sva = size + ReturnBlock->sva;
//			  cprintf("ReturnBlock -> sva   %x \n" , ReturnBlock -> sva) ;
			  return search;
		  }
	 }

   return NULL;

}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
struct MemBlock *alloc_block_BF(uint32 size)
{
	struct MemBlock *trace ;
		uint32 sva = 0 , minSize = 0 ;
		int flag= 0 ;
		LIST_FOREACH(trace,&(FreeMemBlocksList)){
			if(trace->size==size){
				LIST_REMOVE(&FreeMemBlocksList,trace);
				return trace;
			} else if(trace->size>size&&(minSize >trace->size||flag==0)){
				sva = trace->sva ;
				minSize = trace->size ;
				flag = 1 ;
			}
		}
		if(flag == 1 ){
			struct MemBlock *find  ;
			LIST_FOREACH(find,&FreeMemBlocksList){
				if(find->sva==sva){
					struct MemBlock *found =LIST_FIRST(&AvailableMemBlocksList) ;
					LIST_REMOVE(&AvailableMemBlocksList ,found ) ;
					found ->size = size ;
					found ->sva = find->sva ;
					find->sva = sva + size;
					find->size = minSize - size ;
					return found;
				}
			}
		}
		return NULL;

}


//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
struct MemBlock *alloc_block_NF(uint32 size)
{
	struct MemBlock *trace ;

	if(address == 0 ) {
		address = LIST_FIRST(&FreeMemBlocksList)->sva ;

	}
	LIST_FOREACH(trace,&(FreeMemBlocksList)){
		if(trace->sva < address)
			continue ;
		else{
			 if(trace->size>size){
				address = trace->sva + size ;
				struct MemBlock  *element =LIST_FIRST(&AvailableMemBlocksList) ;
				LIST_REMOVE(&AvailableMemBlocksList ,element ) ;
				element ->size = size ;
				element ->sva = trace->sva ;
				trace->sva = trace->sva + size;
				trace->size = trace->size - size ;

				return element ;
			}else if(trace->size==size){
				address = trace->sva +size;
				LIST_REMOVE(&FreeMemBlocksList,trace);
				return trace;
			}
		}
	}
	LIST_FOREACH(trace,&(FreeMemBlocksList)){

		if(trace->size==size){
			address = trace->sva + size ;
			LIST_REMOVE(&FreeMemBlocksList,trace);
			return trace;
		} else if(trace->size>size){
			address = trace->sva +size ;
			struct MemBlock  *element =LIST_FIRST(&AvailableMemBlocksList) ;
			LIST_REMOVE(&AvailableMemBlocksList ,element ) ;
			element ->size = size ;
			element ->sva = trace->sva ;
			trace->sva = trace->sva + size;
			trace->size = trace->size - size ;
			return element ;
		}
		if(trace->sva > address)
			break ;
	}
//	cprintf("\n====================10=====================\n");


	return NULL;

}

//===================================================
// [8] INSERT BLOCK (SORTED WITH MERGE) IN FREE LIST:
//===================================================

void insert_sorted_with_merge_freeList(struct MemBlock *blockToInsert)
{
//	cprintf("BEFORE INSERT with MERGE: insert [%x, %x)\n=====================\n", blockToInsert->sva, blockToInsert->sva + blockToInsert->size);
//	print_mem_block_lists() ;

	uint32 newSize = 0;
	int flage = 0 ;
	int flage1 = 0 ;
	struct MemBlock *firstElement = NULL ;
	if(LIST_EMPTY(&FreeMemBlocksList)==1 ){
		LIST_INSERT_HEAD(&FreeMemBlocksList, blockToInsert) ;
	}else if (LIST_FIRST(&FreeMemBlocksList)->sva > blockToInsert->sva ){
		firstElement =LIST_FIRST(&FreeMemBlocksList) ;
		if(firstElement->sva == blockToInsert->sva+ blockToInsert->size ){
			firstElement->sva = blockToInsert->sva;
			firstElement->size = firstElement->size+ blockToInsert->size;
			blockToInsert->size = 0 ;
			blockToInsert->sva = 0 ;
			LIST_INSERT_HEAD(&AvailableMemBlocksList , blockToInsert) ;
		}else{
			LIST_INSERT_HEAD(&FreeMemBlocksList,blockToInsert);
		}
	}else {
		struct MemBlock *element;
		struct MemBlock *prevElement;
		if(LIST_SIZE(&FreeMemBlocksList)> 1 ){
			LIST_FOREACH(element, &(FreeMemBlocksList))
			{
				prevElement= LIST_PREV(element);
				if(element->sva > blockToInsert->sva ){

					if(element->sva == blockToInsert->sva  +blockToInsert->size){
						element->sva = blockToInsert->sva  ;
						element->size = blockToInsert->size + element->size ;
						blockToInsert->size = 0 ;
						blockToInsert->sva = 0 ;
						LIST_INSERT_HEAD(&AvailableMemBlocksList , blockToInsert ) ;
						if(prevElement->sva + prevElement-> size  == element->sva ){
							prevElement-> size = prevElement-> size + element->size;
							LIST_REMOVE(&FreeMemBlocksList,element) ;
							element->size = 0 ;
							element->sva = 0 ;
							element->prev_next_info.le_next = NULL ;
							element->prev_next_info.le_prev = NULL ;
							LIST_INSERT_HEAD(&AvailableMemBlocksList , element) ;
						}
						flage =1 ;
						break;
					}else if(prevElement->sva +prevElement-> size == blockToInsert->sva){
						prevElement-> size = prevElement-> size + blockToInsert->size;
						blockToInsert->size = 0 ;
						blockToInsert->sva = 0 ;
						LIST_INSERT_HEAD(&AvailableMemBlocksList , blockToInsert) ;
						flage1 = 1 ;
					}
					if(flage1 == 0 ){
					LIST_INSERT_BEFORE(&FreeMemBlocksList,element,blockToInsert);

					}
					flage =1 ;
					break;
				}
			}

		}
		if(flage == 0 ){
			struct MemBlock *prevElement =LIST_LAST(&FreeMemBlocksList) ;
			if(prevElement->sva +prevElement->size== blockToInsert->sva ){
				prevElement->size = prevElement->size+ blockToInsert->size;
				blockToInsert->size = 0 ;
				blockToInsert->sva = 0 ;
				LIST_INSERT_HEAD(&AvailableMemBlocksList , blockToInsert) ;
			}else{
				LIST_INSERT_TAIL(&FreeMemBlocksList,blockToInsert);
			}
		}
	}

//	cprintf("\nAFTER INSERT with MERGE:\n=====================\n");
//	print_mem_block_lists();

}

