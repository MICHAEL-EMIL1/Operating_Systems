#include "kheap.h"
#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include <kern/cpu/sched.h>
#include "memory_manager.h"

#define Mega  (1024*1024)
#define kilo (1024)
#define DYNAMIC_ALLOCATOR_DS ROUNDUP(NUM_OF_KHEAP_PAGES * sizeof(struct MemBlock), PAGE_SIZE)
#define INITIAL_KHEAP_ALLOCATIONS (DYNAMIC_ALLOCATOR_DS + KERNEL_SHARES_ARR_INIT_SIZE + KERNEL_SEMAPHORES_ARR_INIT_SIZE + ROUNDUP(num_of_ready_queues * sizeof(uint8), PAGE_SIZE) + ROUNDUP(num_of_ready_queues * sizeof(struct Env_Queue), PAGE_SIZE))

//void  *phys_to_vrit = create_page_table(ptr_page_directory,984) ;
uint32 *phys_to_vrit ;
uint32 *allPA;
uint32 allPAs[PPN(KERNEL_HEAP_MAX)] ;

//==================================================================//
//==================================================================//
//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)//
//==================================================================//
//==================================================================//

void initialize_dyn_block_system()
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] initialize_dyn_block_system
	// your code is here, remove the panic and write your code
	LIST_INIT(&AllocMemBlocksList);
	LIST_INIT(&FreeMemBlocksList);

//	kpanic_into_prompt("initialize_dyn_block_system() is not implemented yet...!!");

	#if STATIC_MEMBLOCK_ALLOC
	//DO NOTHING
	#else
		MAX_MEM_BLOCK_CNT=NUM_OF_KHEAP_PAGES;
		MemBlockNodes = (struct MemBlock*)KERNEL_HEAP_START;
		allocate_chunk(ptr_page_directory,
				ROUNDDOWN(KERNEL_HEAP_START,PAGE_SIZE),
				NUM_OF_KHEAP_PAGES * sizeof(struct MemBlock),
				PERM_WRITEABLE);

	#endif

	initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);

	struct MemBlock *Blockrem=LIST_FIRST(&AvailableMemBlocksList);

	Blockrem->size=ROUNDUP(KERNEL_HEAP_MAX,PAGE_SIZE)-KERNEL_HEAP_START-ROUNDUP(NUM_OF_KHEAP_PAGES * sizeof(struct MemBlock), PAGE_SIZE);
	Blockrem->sva = KERNEL_HEAP_START+ROUNDUP(NUM_OF_KHEAP_PAGES * sizeof(struct MemBlock), PAGE_SIZE);

	LIST_REMOVE(&AvailableMemBlocksList ,Blockrem ) ;

	LIST_INSERT_TAIL(&FreeMemBlocksList, Blockrem);
	for(uint32 i=ROUNDDOWN(KERNEL_HEAP_START,PAGE_SIZE);i<ROUNDUP(KERNEL_HEAP_START+NUM_OF_KHEAP_PAGES * sizeof(struct MemBlock),PAGE_SIZE);i+=PAGE_SIZE){
		uint32 physical_address= virtual_to_physical(ptr_page_directory,i) ;
	//		kpanic_into_prompt("initialize_dyn_block_system() is not implemented yet...!!");
		allPAs[PPN(physical_address)] = i ;
	}


}

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kmalloc
	// your code is here, remove the panic and write your code
//	kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	//change this "return" according to your answer
	int ret=-1;

	struct MemBlock *allocblock;
	if(isKHeapPlacementStrategyFIRSTFIT()){
	allocblock=alloc_block_FF(ROUNDUP(size,PAGE_SIZE));
	}
	if(isKHeapPlacementStrategyNEXTFIT()){
	allocblock=alloc_block_NF(ROUNDUP(size,PAGE_SIZE));
	}
	if(isKHeapPlacementStrategyBESTFIT()){
	allocblock=alloc_block_BF(ROUNDUP(size,PAGE_SIZE));
	}
	if(allocblock!=NULL){
	uint32 end=ROUNDUP(allocblock->sva+allocblock->size,PAGE_SIZE);
	ret=allocate_chunk(ptr_page_directory,allocblock->sva,ROUNDUP(allocblock->size,PAGE_SIZE),PERM_WRITEABLE);
	}
	if(ret==0){
		insert_sorted_allocList(allocblock) ;
		uint32 physical_address= virtual_to_physical(ptr_page_directory,allocblock->sva) ;
		for(uint32 i=ROUNDDOWN(allocblock->sva,PAGE_SIZE);i<ROUNDUP(allocblock->sva+allocblock->size ,PAGE_SIZE);i+=PAGE_SIZE){
			uint32 physical_address= virtual_to_physical(ptr_page_directory,i) ;
			allPAs[PPN(physical_address)] = i ;
		}
//		kpanic_into_prompt("initialize_dyn_block_system() is not implemented yet...!!");

		return (void *)allocblock->sva;

	}else return NULL;



}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");
	uint32 *ptr_page_table = NULL ;
	struct MemBlock *free=NULL;
	free=find_block(&AllocMemBlocksList,(uint32)virtual_address);
	if(free != NULL){
		for(uint32 i = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE) ; i <(ROUNDUP(((uint32)virtual_address )+free->size, PAGE_SIZE) ); i+=PAGE_SIZE ){
			struct FrameInfo *frame_info= get_frame_info(ptr_page_directory , i , &ptr_page_table ) ;
			if(frame_info != NULL){
				uint32 physical_address= virtual_to_physical(ptr_page_directory,i) ;
				allPAs[PPN(physical_address)] = 0 ;
//				cprintf("i  %x\n " , i );
//				cprintf("physical_address  %x\n " , physical_address );
//				cprintf("allPA[PPN(physical_address)]   %x \n" , allPAs[PPN(physical_address)] );

				unmap_frame(ptr_page_directory,i);

			}
		}
//		unsigned int physical_address= kheap_physical_address(ROUNDDOWN((uint32)virtual_address, PAGE_SIZE) ) ;
		LIST_REMOVE(&AllocMemBlocksList,free);
		insert_sorted_with_merge_freeList(free);

	}
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	// Write your code here, remove the panic and write your code
//	panic("kheap_virtual_address() is not implemented yet...!!");
	return (unsigned int)allPAs[PPN(physical_address)] ;
	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");
//	uint32* ptr_page_table = NULL  ;
//	struct FrameInfo* frames_info =  get_frame_info(ptr_page_directory, virtual_address, &ptr_page_table);
//	uint32 physical_address = to_physical_address(frames_info);
//	//return the physical address corresponding to given virtual_address
//	//refer to the project presentation and documentation for details
//	return physical_address ;
	unsigned int physicalAdd=(unsigned int)virtual_to_physical(ptr_page_directory,(uint32)virtual_address);
	return physicalAdd;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT MS2 - BONUS] [KERNEL HEAP] krealloc
	// Write your code here, remove the panic and write your code
	panic("krealloc() is not implemented yet...!!");
//	struct MemBlock *block = NULL;
//	struct MemBlock *afterblock = NULL ,*beforeblock = NULL;
//	if(virtual_address==NULL){
//
//		return kmalloc((unsigned int)new_size) ;
//	}
//	if(new_size==0){
//		kfree(virtual_address);
//		return NULL;
//	}
//	else{
//		block = find_block(&AllocMemBlocksList,(uint32) virtual_address);
//		cprintf("    %d ...funst \n " , block->size );
//		afterblock=block->prev_next_info.le_next;
//		beforeblock=block->prev_next_info.le_prev;
//		if(block==NULL ){
//			cprintf(" ...funcc \n " );
//			return NULL;
//		}else if(afterblock ==  NULL ||beforeblock ==  NULL){
//			cprintf(" ...funl \n " );
//			return NULL;
//		}else if(afterblock->sva == block->size +block->sva){
//			insert_sorted_allocList(afterblock) ;
//			cprintf("%x ...fun second \n " ,block->sva);
//			return (void *)block->sva;
//		}else if(beforeblock->sva + beforeblock->size  == block->sva){
//			insert_sorted_allocList(beforeblock) ;
//			cprintf("%x ...fun second \n " ,block->sva);
//			return (void *)beforeblock->sva;
//		}else if(new_size == block->size){
//			return virtual_address ;
//		}else {
//
//			kfree( virtual_address);
//			int ret = 0 ;
//			beforeblock=alloc_block_BF(new_size-block->size);
//			if(beforeblock!=NULL){
//				uint32 end=ROUNDUP(beforeblock->sva+beforeblock->size,PAGE_SIZE);
//				ret=allocate_chunk(ptr_page_directory,beforeblock->sva,ROUNDUP(beforeblock->size,PAGE_SIZE),PERM_WRITEABLE);
//				if(ret==0){
//					insert_sorted_allocList(beforeblock) ;
//					cprintf("\n %d ...funlast \n " , block->size );
//					cprintf("\n %d ...funlast \n " , new_size );
//					return (void *)beforeblock->sva;
//				}
//			}
//
//
//		}
//	}
//
//	return NULL;
	struct MemBlock *block = NULL;
	struct MemBlock *newblock = NULL;
	struct MemBlock *afterblock = NULL;
	if(virtual_address==NULL){
		return kmalloc((unsigned int)new_size) ;
	}
	if(new_size==0){
		kfree(virtual_address);
		return NULL;
	}
	else{
		block = find_block(&AllocMemBlocksList,(uint32) virtual_address);
		cprintf("%d ...fund \n " ,block->size);
		cprintf("%d ...funs \n " ,new_size);

		if(block->size >= new_size){
			cprintf("%x ...fun \n " ,block->sva);
			return virtual_address ;
		}
		newblock=block->prev_next_info.le_next;
//		cprintf("%x ...block->sva \n " ,block->sva);
//		cprintf("%x ...newblock->sva \n " ,newblock->sva);
		if(newblock !=NULL){
			uint32 c =newblock->sva;
			insert_sorted_allocList(newblock) ;
			kfree((void *)newblock->sva);
			cprintf("%x ...funfcx \n " , (0xf68a6000- 4193280));
			allocate_chunk(ptr_page_directory,c,ROUNDDOWN(new_size,PAGE_SIZE) ,PERM_WRITEABLE);
			return (void *)block->sva;;
		}
		else {

				newblock=alloc_block_BF((new_size-block->size));

				if(newblock!=NULL){
				allocate_chunk(ptr_page_directory,newblock->sva,ROUNDDOWN(new_size,PAGE_SIZE) ,PERM_WRITEABLE);
				insert_sorted_allocList(newblock) ;
				kfree((void *)block->sva);
				return (void *)newblock->sva;
				}else{
					cprintf("%x ... \n " , (0xf68a6000- 4193280));
					return krealloc( block->prev_next_info.le_next, new_size);
				}
			}


	}
	return NULL;
}
