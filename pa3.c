/**********************************************************************
 * Copyright (c) 2019
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "types.h"
#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * The current process
 */
extern struct process *current;

/**
 * alloc_page()
 *
 * DESCRIPTION
 *   Allocate a page from the system. This function is implemented in vm.c
 *   and use to get a page frame from the system.
 *
 * RETURN
 *   PFN of the newly allocated page frame.
 */
extern unsigned int alloc_page(void);



/**
 * TODO translate()
 *
 * DESCRIPTION
 *   Translate @vpn of the @current to @pfn. To this end, walk through the
 *   page table of @current and find the corresponding PTE of @vpn.
 *   If such an entry exists and OK to access the pfn in the PTE, fill @pfn
 *   with the pfn of the PTE and return true.
 *   Otherwise, return false.
 *   Note that you should not modify any part of the page table in this function.
 *
 * RETURN
 *   @true on successful translation
 *   @false on unable to translate. This includes the case when @rw is for write
 *   and the @writable of the pte is false.
 */
bool translate(enum memory_access_type rw, unsigned int vpn, unsigned int *pfn)
{
	/*** DO NOT MODIFY THE PAGE TABLE IN THIS FUNCTION ***/
	struct pte_directory *pd;
	unsigned int offset = (vpn << 28) >> 28;
	unsigned int v = vpn >> 4;

	pd = current->pagetable.outer_ptes[v];
	struct pte *pte = &pd->ptes[offset];


	if (rw == 0) {
		if (pd && pte) {
			if (pte->valid == 1) {
				*pfn = pte->pfn;
				return true;
			}
		}
	}

	else if(rw == 1) {
		if (pd && pte) {
			if (pte->writable == 1) {
				*pfn = pte->pfn;
				return true;
			}
		}
	}
	return false;
}


/**
 * TODO handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the translate() for @vpn fails. This implies;
 *   1. Corresponding pte_directory is not exist
 *   2. pte is not valid
 *   3. pte is not writable but @rw is for write
 *   You can assume that all pages are writable; this means, when a page fault
 *   happens with valid PTE without writable permission, it was set for the
 *   copy-on-write.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(enum memory_access_type rw, unsigned int vpn)
{
	struct pte_directory *pd;
	struct pte_directory *current_pd;
	unsigned int offset = (vpn << 28) >> 28;
	unsigned int v = vpn >> 4;
	current_pd = current->pagetable.outer_ptes[v];

	if (!current_pd->ptes) {
		pd = (struct pte_directory *)malloc(sizeof(struct pte_directory));
		current->pagetable.outer_ptes[v] = pd;
		goto jump;
	}

	pd = current_pd;
	
jump:



	pd->ptes[offset].valid = true;
	pd->ptes[offset].writable = 1;
	pd->ptes[offset].pfn = alloc_page();

	
	return true;
}


/**
 * TODO switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put to the **TAIL** of the
 *   @processes list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes.
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table. Also, should update the writable bit properly to implement
 *   the copy-on-write feature.
 */
void switch_process(unsigned int pid)
{
	struct process *new_process = (struct process *)malloc(sizeof(struct process));
	struct process *order;
	struct pte_directory *pd;
	struct pte_directory *pd2;
	list_for_each_entry(order, &processes, list) {
		if (order->pid == pid) {
			list_replace(&order->list, &current->list);
			current = order;
			goto skip;
		}
	}

	for (int i = 0; i < NR_PTES_PER_PAGE;i++) {  
		if(current->pagetable.outer_ptes[i] != NULL ) {  

			pd = (struct pte_directory *)malloc(sizeof(struct pte_directory));
			pd2 = current->pagetable.outer_ptes[i];
			for (int j = 0; j < NR_PTES_PER_PAGE; j++) {
				pd->ptes[j].valid = pd2->ptes[j].valid;   // current를 pd로 복사한다.
				pd->ptes[j].writable = pd2->ptes[j].writable;
				pd->ptes[j].pfn = pd2->ptes[j].pfn;
				if (pd2->ptes[j].writable == 1) {
					pd2->ptes[j].writable = 0;  //current에서의 rw를 r로 바꾼다.
					pd->ptes[j].writable = 0;   //바꿀 process에서의 rw를 r로 바꾼다.
				}
			}
			new_process->pagetable.outer_ptes[i] = pd;  

		}
	}


	list_add(&current->list, &processes);

	new_process->pid = pid;
		

   	current = new_process;
skip:
	return;


}

