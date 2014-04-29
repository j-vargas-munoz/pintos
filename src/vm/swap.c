#include "vm/swap.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include <stdio.h>
#include <debug.h>
#include <string.h>

static struct block *swap_block;
static struct bitmap *used_swap_slots;
static struct lock lock_swap;
static int8_t clean_buffer[BLOCK_SECTOR_SIZE];

/* Initialize the swap system to be used in vm */
void
swap_init ()
{
  swap_block = block_get_role (BLOCK_SWAP);
  block_sector_t slot_count = 
    block_size (swap_block) / SECTORS_PER_PAGE;
  printf ("Swap: Available swap slots: %d\n", slot_count);
  printf ("Swap: Sector per page: %d\n", SECTORS_PER_PAGE);
  used_swap_slots = bitmap_create (slot_count);
  bitmap_set_all (used_swap_slots, false);
  lock_init (&lock_swap);
  memset (clean_buffer, 0, BLOCK_SECTOR_SIZE);
  /* No need to free this structure because it is alive
     throughout pintos execution */
}

/* Write down the given page to a swap slot 
   if it is possible and return the slot number 
   asociated and it marks such a slot as not available. 
   If no swap is available to store the given page
   then the kernel panics. */
swap_slot_t
swap_write (void* kpage)
{
  ASSERT (pg_ofs (kpage) == 0);

  lock_acquire (&lock_swap);
  
  swap_slot_t slot = bitmap_scan_and_flip (used_swap_slots, 0, 1, false);
  ASSERT (slot != BITMAP_ERROR);
  
  block_sector_t initial_sector = slot * SECTORS_PER_PAGE;
  int8_t *tmp = (int8_t*)kpage;
  
  int32_t i;
  for (i=0; i<SECTORS_PER_PAGE; i++, initial_sector++, tmp+=BLOCK_SECTOR_SIZE)
      block_write (swap_block, initial_sector, tmp);

  lock_release (&lock_swap);
  
  return slot;
}

/* Reads the specified swap slot SLOT and writes the 
   result into page PAGE. After the read has taken
   place the asociated slot is freed and make available
   again.
   if PAGE is not page aligned the the kernel
   panics, the same happens when the specified SLOT
   has no information written. */
void 
swap_read (void* kpage, swap_slot_t slot)
{
  ASSERT (pg_ofs (kpage) == 0);

  lock_acquire (&lock_swap);
  
  ASSERT (bitmap_test (used_swap_slots, slot));

  block_sector_t initial_sector = slot * SECTORS_PER_PAGE;
  int8_t* tmp = (int8_t*)kpage;
  
  int32_t i;
  for (i=0; i<SECTORS_PER_PAGE; i++, initial_sector++, tmp+=BLOCK_SECTOR_SIZE)
    block_read (swap_block, initial_sector, tmp);      

  bitmap_reset (used_swap_slots, slot);
  lock_release (&lock_swap);
}

/* Make available the specified swap slots */
void 
swap_free_slot (swap_slot_t slot)
{
  lock_acquire (&lock_swap);
  ASSERT (bitmap_test (used_swap_slots, slot));
  bitmap_reset (used_swap_slots, slot);
  lock_release (&lock_swap);
}
