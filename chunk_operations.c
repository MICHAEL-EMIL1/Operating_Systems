/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"

/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	uint32 sva=ROUNDDOWN(source_va,PAGE_SIZE);
	uint32 dva=ROUNDDOWN(dest_va,PAGE_SIZE);
	uint32 dend=ROUNDUP((PAGE_SIZE*num_of_pages)+dest_va,PAGE_SIZE);
	uint32 send=ROUNDUP((PAGE_SIZE*num_of_pages)+source_va,PAGE_SIZE);
	for(int i=dva;i<dend;i+=PAGE_SIZE){
		uint32 *ptr;
		struct FrameInfo * frame =get_frame_info(page_directory,i,&ptr);
		if(frame!=NULL)
			return -1;

	}
	for(int i=sva;i<send;i+=PAGE_SIZE){
		uint32 *ptr;
		get_page_table(page_directory,i,&ptr);
		struct FrameInfo * f = get_frame_info(page_directory,i,&ptr);
		uint32 perm=pt_get_page_permissions(page_directory,i);
		map_frame(page_directory,f,dva, perm);
		unmap_frame(page_directory,i);
		dva+=PAGE_SIZE;
	}
	    return 0;
}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{

	//panic("copy_paste_chunk() is not implemented yet...!!");
	uint32 *ptr_page = NULL;
	uint32 *ptr_frame = NULL;

	//page check

	for(uint32 i=dest_va;i < size+dest_va;i+=PAGE_SIZE){
		if (get_frame_info(page_directory,i,&ptr_frame)!= NULL){
			int prm = pt_get_page_permissions(page_directory,i);
			if((prm & PERM_WRITEABLE) == 0){
				return -1 ;
			}
		}
	}

	//frame check

	//copy
	for(uint32 i=dest_va, j = source_va ;i < size+dest_va;i++,j++)
	{
		if (get_page_table(page_directory,i,&ptr_page) == TABLE_NOT_EXIST ){
				create_page_table(page_directory,i);
		}
		if (get_frame_info(page_directory,i,&ptr_frame)== NULL){
			struct FrameInfo *ptr=NULL;
			uint32 prm2;
			allocate_frame(&ptr);
			prm2 = pt_get_page_permissions(page_directory,j);
			map_frame(page_directory,ptr,i,PERM_WRITEABLE|( prm2 & PERM_USER ));
		}
		unsigned char *ptr_des = (unsigned char*)i;
		unsigned char *ptr_src = (unsigned char*)j;

		*ptr_des = *ptr_src ;
	}
	return 0 ;


}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should share the given size from dest_va with the source_va
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{

	uint32* pg=NULL;
	uint32 des_add= ROUNDDOWN(dest_va,PAGE_SIZE);//start=des_va;
	uint32 sor_add= ROUNDDOWN(source_va,PAGE_SIZE);//start=sor_va;
	uint32 des_add2= ROUNDUP(dest_va+size,PAGE_SIZE);//end=des_va+size;
	uint32 sor_add2= ROUNDUP(source_va+size,PAGE_SIZE);//end=sor_va+size;
	uint32 return_pt_des ;
	for(int i=des_add ; i< des_add2 ;i+=PAGE_SIZE){
		struct FrameInfo *frame_info= get_frame_info(page_directory,i,&pg);
		if(frame_info != NULL ){
			return -1;
		}

	}
	for(int i=sor_add;i<sor_add2 ;i+=PAGE_SIZE){
		return_pt_des = get_page_table(page_directory,des_add,&pg);
		if(return_pt_des == TABLE_NOT_EXIST){
			pg = create_page_table(page_directory,des_add);
		}
	   struct FrameInfo * source_frames= get_frame_info(page_directory,i,&pg);
	   map_frame(page_directory,source_frames,des_add,perms);
	   des_add+=PAGE_SIZE;
	}
	return 0;
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate in RAM the given range [va, va+size)
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{
	uint32 va1 = ROUNDDOWN(va , PAGE_SIZE);
	uint32 va2 = ROUNDUP(va+size , PAGE_SIZE);
	struct FrameInfo *frame_info = NULL ;
	uint32 *pgTable;
	for(uint32 i=va1;i<va2;i+=PAGE_SIZE){
			frame_info= get_frame_info(page_directory , i , &pgTable ) ;
			if( frame_info != NULL)
				return -1;
	}
	for(uint32 i=va1;i<va2;i+=PAGE_SIZE){
			frame_info= get_frame_info(page_directory , i , &pgTable ) ;
			allocate_frame(&frame_info);
			map_frame(page_directory,frame_info,i , perms ) ;
	}

	 return 0;

}

/*BONUS*/
//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{

	uint32 *ptr_page_table = NULL ,*is_ptr_page_table_same=NULL ;
	uint32 count_pages =0 , count_tables = 0  ;
	uint32 va = ROUNDDOWN(sva , PAGE_SIZE);
	uint32 va2 = ROUNDUP(eva, PAGE_SIZE);
	for(uint32 i = va ; i < va2 ; i+=PAGE_SIZE){
		struct FrameInfo *frame_info= get_frame_info(page_directory , i , &ptr_page_table ) ;

		if( frame_info != NULL)
			count_pages++ ;

		if( ptr_page_table!=NULL  && ptr_page_table != is_ptr_page_table_same){
			count_tables ++ ;
			is_ptr_page_table_same = ptr_page_table;
		}


	}
	*num_pages = count_pages ;
	*num_tables = count_tables ;
}

/*BONUS*/
//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
// calculate_required_frames:
// calculates the new allocation size required for given address+size,
// we are not interested in knowing if pages or tables actually exist in memory or the page file,
// we are interested in knowing whether they are allocated or not.
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{

	uint32 x , count =0 , count1 =0;
		uint32 va = ROUNDDOWN(sva , PAGE_SIZE);
		uint32 va2 = ROUNDUP(sva+size , PAGE_SIZE);
		uint32 *ptr_page_table = NULL ,*is_ptr_page_table_same=NULL ;
		uint32 count_pages =0 , count_tables = 0  ;
		int flage = 1 ;
		struct freeFramesCounters counters = calculate_available_frames() ;
		struct FrameInfo *frame_info= get_frame_info(page_directory , sva , &ptr_page_table ) ;

		for(uint32 i = sva  ; i < sva+size ; i+=PAGE_SIZE){
			struct FrameInfo *frame_info= get_frame_info(page_directory , sva , &ptr_page_table ) ;
			if(frame_info == NULL)
				count_pages++ ;
				size-=PAGE_SIZE;
			get_page_table(page_directory , i , &ptr_page_table ) ;
			if( ptr_page_table == NULL ){
				count_tables++ ;
			}
			if(size ==0)
				break;

	 	}
		cprintf("(count-count_tables)   %d   count_tables %d    count_pages     %d        frame_info->references    %d\n" ,count ,(count_tables),  count_pages ,count1);
		cprintf("(count-count_tables)   %d\n" , size) ;
		return count_tables +count_pages;
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	// Write your code here, remove the panic and write your code
	panic("allocate_user_mem() is not implemented yet...!!");
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3] [USER HEAP - KERNEL SIDE] free_user_mem
	// Write your code here, remove the panic and write your code
//	panic("free_user_mem() is not implemented yet...!!");
	//1. Free ALL pages of the given range from the Page File
	uint32 start=ROUNDDOWN(virtual_address,PAGE_SIZE);
	uint32 end=ROUNDUP(size,PAGE_SIZE);
	uint32 range=virtual_address+size;
	range=ROUNDUP(range,PAGE_SIZE);
	for(uint32 i=start;i<range; i+=PAGE_SIZE)
	{
		pf_remove_env_page(e, i);
	}
	//2. Free ONLY pages that are resident in the working set from the memory
	for(int i=0;i<e->page_WS_max_size;i++)
	{
			uint32 va=env_page_ws_get_virtual_address(e, i);
			if((va>=start)&&(va<range))
			{
				unmap_frame(e->env_page_directory,va);
				env_page_ws_clear_entry(e, i);
			}
	}
		//3. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
	for(uint32 i=start;i<range; i+=PAGE_SIZE){
		uint32 *pgTable=NULL;
		int check = get_page_table(e->env_page_directory, i, &pgTable);
		if(check != TABLE_NOT_EXIST){
			int emptyTable=0;
			if(pgTable!=NULL){
				for(int x=0;x<1024;x++){
					if( pgTable[x]==0){
						emptyTable++;
					}
					if(emptyTable==1024){
					kfree((void*)pgTable);
					e->env_page_directory[PDX(i)] = 0;

//					pd_clear_page_dir_entry(e->env_page_directory, i);
					}
				}


			 }
		}

	}
	//This function should:
	//1. Free ALL pages of the given range from the Page File
	//2. Free ONLY pages that are resident in the working set from the memory
	//3. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
}

//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");

	//This function should:
	//1. Free ALL pages of the given range from the Page File
	//2. Free ONLY pages that are resident in the working set from the memory
	//3. Free any BUFFERED pages in the given range
	//4. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3 - BONUS] [USER HEAP - KERNEL SIDE] move_user_mem
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

	// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
	// with the given size
	// After finished, the src_virtual_address must no longer be accessed/exist in either page file
	// or main memory

	/**/
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//

