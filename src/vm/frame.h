#ifndef VH_FRAME
#define VH_FRAME

#include "threads/thread.h"
#include "threads/palloc.h"

void init_frame (void);
void* allocate_frame (enum palloc_flags flag);
void free_frame (void *page);

#endif