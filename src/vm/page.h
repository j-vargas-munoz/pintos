
#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stddef.h>
#include "vm/swap.h"
#include "filesys/directory.h"
#include "filesys/off_t.h"
#include "filesys/file.h"
#include "threads/synch.h"

/*
 * Enumeration defining possible page locations, by the
 * moment only swap disk and file system are the possible
 * actual locations of any page in the supplemental page
 * table of any process.
 */
enum page_location 
  {
    LOC_SWAP = 001,     /* The content is in the swap disk  */
    LOC_FILESYS = 002,  /* The content is in the file system */
    LOC_EXECUTABLE = 004, /* Maybe no needed, can be represented with LOC_FILESYS */
    LOC_NONE = 010      /* No location means the page is zeroed */
  };

/*
 * This structure represents supplemental page table entry
 * for a process, it contains necessary information to be
 * brought from swap disk or file system or neither of them,
 * in the last case the page is a clean page, i.e. zeroed
 * page.
 * UPAGE must be page aligned.
 */
struct page
  {
    void* upage;                  /* User VA that this page represents*/
    bool writable;                /* Is Writable? */
    enum page_location location;  /* Location swap, filesys, etc. */
    swap_slot_t slot;             /* Swap slot if location is SWAP */
    struct file* file;            /* File if location si file system */
    off_t ofs;                    /* Offset within the specified file */
    size_t read_bytes;            /* Bytes to be read from filesys */
    struct hash_elem elem;        /* To be inserted into a hash table */
  };

/* Concurrent supplemental page table */
struct spt
  {
    struct hash table;
    struct lock lock;
  };

/* SPT initialization and destruction */
void spt_init (struct spt*);
void spt_destroy (struct spt*);

/* Functions to add elements to SPT of current process */
void page_add_entry (struct spt*, void*upage, bool writable, 
		     enum page_location, struct file*, off_t offset, 
		     size_t read_bytes, uint32_t* pagedir);
			 
void* page_remove_entry (struct spt*, struct page*);

/* Functions to perform serch over a given  */
struct page* page_lookup (struct spt*, void* user_vaddr);

#endif
