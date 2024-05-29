/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))
#define TLB_SIZE mp->maxsz / TLB_ENTRY_SIZE

pthread_mutex_t cache_lock;

// cache mapping: 
// 1 bit for checking availability
// 32 bit => 4 bytes for pid
// 14 bit  => 2 bytes for page number
// 13 bit  => 2 bytes for frame number (data)
// => 9 bytes in total 

/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, BYTE *value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   pthread_mutex_lock(&cache_lock);
   int cache_idx = pgnum % TLB_SIZE;
   int base_addr = cache_idx * TLB_ENTRY_SIZE;
   
   int avail = mp->storage[base_addr];
   if ((avail & 1) == 0) return -1;

   int cache_pid = 0;
   for(int j = 0; j <= 3; j++){
      cache_pid |= (mp->storage[base_addr + 4 - j] << (j * 8));
   }

   int cache_pgn = (mp->storage[base_addr + 5] << 8) | mp->storage[base_addr + 6];
   int cache_data = (mp->storage[base_addr + 7] << 8) | mp->storage[base_addr + 8]; 
      
   if(cache_pid == pid && cache_pgn == pgnum){
      *value = cache_data;
      pthread_mutex_unlock(&cache_lock);
      return 0;
   }

   pthread_mutex_unlock(&cache_lock);

   return -1;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, BYTE* value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   pthread_mutex_lock(&cache_lock);
   int cache_idx = pgnum % TLB_SIZE;
   int base_addr = cache_idx * TLB_ENTRY_SIZE;

   for(int i = 0; i <= 3; i++){
      TLBMEMPHY_write(mp, base_addr + 4 - i, (pid >> (i * 8)) & 0xFF);
   }
   
   TLBMEMPHY_write(mp, base_addr + 6, pgnum & 0xFF);
   TLBMEMPHY_write(mp, base_addr + 5, (pgnum >> 8) & 0xFF);
  
   TLBMEMPHY_write(mp, base_addr + 8, *value & 0xFF);
   TLBMEMPHY_write(mp, base_addr + 7, (*value >> 8) & 0xFF);
   
   TLBMEMPHY_write(mp, base_addr, 1);
   
   pthread_mutex_unlock(&cache_lock);

   return 0;
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   *value = mp->storage[addr];

   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   mp->storage[addr] = data;

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */


int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
   if (mp == NULL) return -1;
   for (int i = 0; i < mp->maxsz; i += TLB_ENTRY_SIZE){
      int avail = mp->storage[i];
      if ((avail & 1) == 0) continue;

      int cache_pid;
      for(int j = 0; j <= 3; j++){
         cache_pid |= (mp->storage[i + 4 - j] << (j * 8));
      }
      int cache_pgn = (mp->storage[i + 5] << 8) | mp->storage[i + 6];
      int cache_data = (mp->storage[i + 7] << 8) | mp->storage[i + 8]; 

      printf("Entry %d:\tUsed: %d\tPID: %d\tPage: %d\tFrame: %d\n", i/TLB_ENTRY_SIZE, avail, cache_pid, cache_pgn, cache_data);
   }
   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;

   return 0;
}

//#endif
