/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */
   pthread_mutex_lock(&cache_lock);
   for(int i = 0; i < proc->tlb->maxsz; i += TLB_ENTRY_SIZE)
   {
      int avail = mp->storage[i];
      if ((avail & 1) == 0)
         continue;
      int cache_pid = 0;
      for(int j = 0; j <= 3; j++)
      {
         cache_pid |= (proc->tlb->storage[i + 4 - j] << (j * 8));
      }
      int cache_pgn = (mp->storage[i + 5] << 8) | mp->storage[i + 6];
      int cache_fpn = (mp->storage[i + 7] << 8) | mp->storage[i + 8]; 

      if(proc->pid == cache_pid)
      {
         pte_set_fpn(&proc->mm->pgd[cache_pgn], cache_fpn);
      }
   }
   pthread_mutex_unlock(&cache_lock);

  return 0;
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
{
  /* TODO flush tlb cached*/
  if(proc == NULL || mp == NULL)
     return 0;
  pthread_mutex_lock(&cache_lock);
  for(int i = 0; i < proc->tlb->maxsz; i += TLB_ENTRY_SIZE)
  {
     int cache_pid = 0;
     for(int j = 0; j <= 3; j++)
     {
        cache_pid |= (proc->tlb->storage[i + 4 - j] << (j * 8));
     }
     if(proc->pid == cache_pid) 
        TLBMEMPHY_write(mp, i, 0);
  }
  pthread_mutex_unlock(&cache_lock);
  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  
  if(val == 0)
  {
     int pgn = proc->regs[reg_index] / PAGE_SIZE;
     int num_pages = size / PAGE_SIZE;
     if(size % PAGE_SIZE != 0)
        num_pages++; 
     for(int i = pgn; i < pgn + num_pages; i++)
     {
        int fpn = proc->mm->pgd[i] & PAGING_PTE_FPN_MASK;
        tlb_cache_write(proc->tlb, proc->pid, i, &fpn);
     }  
     TLBMEMPHY_dump(proc->tlb);
  }
  
  return val;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  pthread_mutex_lock(&cache_lock);
  int val = __free(proc, 0, reg_index);
  if(val == -1) return -1;

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  
  int pgn = proc->regs[reg_index] / PAGE_SIZE;
  
  for(int i = 0; i < proc->tlb->maxsz; i += TLB_ENTRY_SIZE)
  {
     int avail = proc->tlb->storage[i];
     if ((avail & 1) == 0)
         continue;
     int cache_pid = 0;
     for(int j = 0; j <= 3; j++)
     {
        cache_pid |= (proc->tlb->storage[i + 4 - j] << (j * 8));
     }
     int cache_pgn = (proc->tlb->storage[i + 5] << 8) | proc->tlb->storage[i + 6];
         
     if(proc->pid == cache_pid && pgn == cache_pgn)
     {
        TLBMEMPHY_write(proc->tlb, i, 0);
     }
  }

  pthread_mutex_unlock(&cache_lock);
  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{
  BYTE data;
  int frmnum = -1;
	
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/
  int pgn = proc->regs[source] / PAGE_SIZE;
  tlb_cache_read(proc->tlb, proc->pid, pgn, &frmnum);
#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  else 
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
  TLBMEMPHY_dump(proc->tlb);
#endif
  if(frmnum == -1)
  {
     frmnum = proc->mm->pgd[pgn] & PAGING_PTE_FPN_MASK;  
     tlb_cache_write(proc->tlb, proc->pid, pgn, &frmnum);
  }

  int val = __read(proc, 0, source, offset, &data);

  destination = (uint32_t) data;

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  return val;
}

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  int val;
  int frmnum = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/
  int pgn = proc->regs[destination] / PAGE_SIZE;
  tlb_cache_read(proc->tlb, proc->pid, pgn, &frmnum);

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
	else
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram); 
  TLBMEMPHY_dump(proc->tlb);
#endif
  if(frmnum == -1)
  {
     frmnum = proc->mm->pgd[pgn] & PAGING_PTE_FPN_MASK;  
     tlb_cache_write(proc->tlb, proc->pid, pgn, &frmnum);
  }

  val = __write(proc, 0, destination, offset, data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  return val;
}

//#endif
