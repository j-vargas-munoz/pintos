#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <inttypes.h>
#include "devices/block.h"
#include "threads/vaddr.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

typedef uint32_t swap_slot_t;

void swap_init (void);

void swap_read (void *buffer, swap_slot_t slot);
swap_slot_t swap_write (void *buffer);

void swap_free_slot (swap_slot_t);

#endif
