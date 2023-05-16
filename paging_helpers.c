/*
 * paging_helpers.c
 *
 *  Created on: Sep 30, 2022
 *      Author: HP
 */
#include "memory_manager.h"

/*[2.1] PAGE TABLE ENTRIES MANIPULATION */
inline void pt_set_page_permissions(uint32* page_directory, uint32 virtual_address, uint32 permissions_to_set, uint32 permissions_to_clear)
{
	uint32 *pointer;
	get_page_table(page_directory,virtual_address,&pointer);
	if(pointer !=NULL){

		if(permissions_to_set!=0){

		uint32 var=PTX(virtual_address);
		uint32 vartoset=pointer[var]  ;          //=entry value
		pointer[var]=vartoset | permissions_to_set ;}
		if(permissions_to_clear!=0){
			uint32 var=PTX(virtual_address);
			uint32 vartoset=pointer[var] ;           //=entry value
			pointer[var]=vartoset & ~ permissions_to_clear ;

		}

	}
	 else if(pointer== NULL){
	     panic("invalid va");
	}
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline int pt_get_page_permissions(uint32* page_directory, uint32 virtual_address )
{
	uint32 *ptr;
	get_page_table(page_directory,virtual_address,&ptr);
	if(ptr!=NULL){
		uint32 address = ptr[PTX(virtual_address)];
		int perm = address<<20;
		perm = perm >>20;
		return perm;
	}
	else{
		return -1;
	}
}

inline void pt_clear_page_table_entry(uint32* page_directory, uint32 virtual_address)
{
	uint32 *cleared_entry;
	get_page_table(page_directory,virtual_address,&cleared_entry);
	if(cleared_entry == NULL){
		panic("Invalid va");
	}else {
		uint32 index_entry=PTX(virtual_address);
		cleared_entry[index_entry] = 0 ;
	}

	tlb_invalidate((void *)NULL, (void *)virtual_address);

}

/***********************************************************************************************/

/*[2.2] ADDRESS CONVERTION*/
inline int virtual_to_physical(uint32* page_directory, uint32 virtual_address)
{
	uint32 physAddress=0;

	uint32 *ptr_page_table = NULL;
	int ret = get_page_table(page_directory, virtual_address, &ptr_page_table);
	if (ret == TABLE_IN_MEMORY )
	{
		uint32 table_entry = ptr_page_table [PTX(virtual_address)];
			uint32 offset=0;
			table_entry=(table_entry>>10);
			offset=PGOFF(virtual_address);
			physAddress=table_entry*1024+offset;

	} else if(ret==TABLE_NOT_EXIST ){
		return -1;
	}

	return ROUNDDOWN(physAddress,1024);

}

/***********************************************************************************************/

/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

///============================================================================================
/// Dealing with page directory entry flags

inline uint32 pd_is_table_used(uint32* page_directory, uint32 virtual_address)
{
	return ( (page_directory[PDX(virtual_address)] & PERM_USED) == PERM_USED ? 1 : 0);
}

inline void pd_set_table_unused(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] &= (~PERM_USED);
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline void pd_clear_page_dir_entry(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] = 0 ;
	tlbflush();
}
