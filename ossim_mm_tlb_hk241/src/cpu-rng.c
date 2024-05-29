//#ifdef CPU_RNG
/*
 * RANDOM NUMBER GENERATOR based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

void init_rng() {
    srand(time(NULL));
}

int rng_get_random_frame() {
    // Get a random frame number, assume frame numbers are in range [0, 255]
    return rand() % 255;
}

int ralloc(struct pcb_t *proc, int vmaid, uint32_t reg_index, uint32_t size, int *addr) {

    struct vm_rg_struct rgnode;

    if (get_free_vmrg_area(proc, vmaid, size, &rgnode) == 0)
    {
        proc->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
        proc->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

        *addr = rgnode.rg_start;
        return 0;
    }
    return -1;
}

int rfree(struct pcb_t *proc, int vmaid, uint32_t reg_index) {
    
    struct vm_rg_struct rgnode;

    if(reg_index < 0 || reg_index > PAGING_MAX_SYMTBL_SZ)
        return -1;

    rgnode.rg_start = caller->mm->symrgtbl[reg_index].rg_start;
    rgnode.rg_end = caller->mm->symrgtbl[reg_index].rg_end;
    
    enlist_vm_freerg_list(caller->mm, rgnode);

    return 0;
}

int rngalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index){
    int addr, val;

    val = ralloc(proc, 0, reg_index, size, &addr);

    if (val == 0) {
        int pgn = proc->regs[reg_index] / PAGE_SIZE;
        uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE : size / PAGE_SIZE + 1;

        for (int i = pgn; i < pgn + num_pages; i++) {
            int fpn = rng_get_random_frame();
            pte_set_fpn(proc->mm->pgd[i], fpn); 
        }
    }

    return val;
}

int rngfree_data(struct pcb_t *proc, uint32_t reg_index){

    int val = rfree(proc, 0, reg_index);
    if (val == -1) return -1;

    int pgn = proc->regs[reg_index] / PAGE_SIZE;

    for (int i = 0; i < proc->mm->pgd_size; i++) {
        proc->mm->pgd[i] = 0;
    }

    return 0;
}

