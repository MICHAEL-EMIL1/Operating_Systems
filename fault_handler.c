/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault
void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
	uint32 roundedFault=ROUNDDOWN(fault_va,PAGE_SIZE);// va
	uint32 e= env_page_ws_get_size(curenv);//used working set current page count
	uint32 stck=((roundedFault<USTACKTOP)&&(roundedFault>=USTACKBOTTOM));//stack range
	uint32 uheap =((roundedFault>=USER_HEAP_START)&&(roundedFault<USER_HEAP_MAX));//Heap range
	if(e< curenv->page_WS_max_size){

		struct FrameInfo *ptr_frame_info = NULL;
		allocate_frame(&ptr_frame_info);
		map_frame(curenv->env_page_directory, ptr_frame_info, roundedFault, PERM_WRITEABLE|PERM_PRESENT|PERM_USER);
		int found = pf_read_env_page(curenv,(void*)roundedFault);
		if(found == E_PAGE_NOT_EXIST_IN_PF && stck == 0 && uheap == 0) {
			panic("ILLEGAL MEMORY ACCESS");
		}
		for(int i=0;i<curenv->page_WS_max_size;i++)
		{

			if(env_page_ws_is_entry_empty(curenv,i))
			{
				env_page_ws_set_entry(curenv, i, roundedFault);
				break;
			}
		}
		curenv->page_last_WS_index = (curenv->page_last_WS_index + 1) % curenv->page_WS_max_size;
	}else{
		uint32 victimAddress=0;
		int modFlag=0;
		//int victimIDX=0;
		int k = curenv->page_last_WS_index;
			// cprintf("sizeof e    %d\n" ,e) ;
		while(1 == 1) {
			victimAddress = env_page_ws_get_virtual_address(curenv, k);
			uint32 x= pt_get_page_permissions(curenv->env_page_directory, victimAddress);
			if((x&PERM_USED)){ // uBitofthe entry
				 pt_set_page_permissions(curenv->env_page_directory ,victimAddress, 0,PERM_USED);
			}
			else {
			 if(x&PERM_MODIFIED){
			   modFlag=1;
			   }
			 break;

			}
			k++;
			if(k == curenv->page_WS_max_size)
				k=0;
		}
	  curenv->page_last_WS_index = k;
	  uint32 *pgt = NULL;
	  struct FrameInfo *faultFrame = get_frame_info(curenv->env_page_directory,victimAddress,&pgt);
	  if(modFlag==1){
		  struct FrameInfo *faultFrame = get_frame_info(curenv->env_page_directory,victimAddress,&pgt);
		  pf_update_env_page(curenv,victimAddress, faultFrame);

	  }
	 unmap_frame(curenv->env_page_directory,victimAddress);
	 env_page_ws_clear_entry(curenv, curenv->page_last_WS_index);

	 if(e<(uint32)curenv->page_WS_max_size){
			struct FrameInfo *ptr_frame_info = NULL;
			allocate_frame(&ptr_frame_info);
			ptr_frame_info->va=roundedFault;
			ptr_frame_info->environment=curenv;
			map_frame(curenv->env_page_directory, ptr_frame_info, roundedFault, PERM_WRITEABLE|PERM_PRESENT|PERM_USER);
			int found = pf_read_env_page(curenv,&roundedFault);
			env_page_ws_set_entry(curenv, (uint32)(curenv->page_last_WS_index)++, roundedFault);
			curenv->page_last_WS_index=(uint32)(curenv->page_last_WS_index)%curenv->page_WS_max_size;

			if(found != 0) {
			  if((stck==1)||(uheap==1)){
				//curenv->page_last_WS_index=(uint32)(curenv->page_last_WS_index+1)%curenv->page_WS_max_size-1;
				//env_page_ws_set_entry(curenv,(uint32)(curenv->page_last_WS_index)++, roundedFault);
				//curenv->page_last_WS_index=(uint32)(curenv->page_last_WS_index)%curenv->page_WS_max_size;

					}
			  else {
						unmap_frame(curenv->env_page_directory,roundedFault);
					 panic("ILLEGAL MEMORY ACCESS");   		}
			}

		}

	}
}
void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	// Write your code here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");


}
