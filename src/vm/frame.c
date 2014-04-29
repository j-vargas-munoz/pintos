#include "vm/frame.h"
#include <list.h>
#include "threads/synch.h"

struct frame
{
    void *page;
    struct thread *owner_thread;
    struct list_elem elem;
};


static struct list frame_table;
static struct lock table_lock;


void
init_frame (void) {
    list_init(&frame_table);
    lock_init(&table_lock);
}

void*
allocate_frame (enum palloc_flags flag) {
    lock_acquire(&table_lock);
    void *page = palloc_get_page(flag);
    lock_release(&table_lock);
    if (page == NULL)
        PANIC ("Not yet implemented");

    struct frame *f = malloc(sizeof(struct frame));
    if (f == NULL)
        PANIC ("Could not get any memory");

    f->owner_thread = thread_current();
    f->page = page;

    lock_acquire(&table_lock);
    list_push_back(&frame_table, &f->elem);
    lock_release(&table_lock);

    return page;
}

void
free_frame (void *page)
{
    struct frame *f;
    struct list_elem *e;
    lock_acquire (&table_lock);
    for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next(e))
    {
        f = list_entry (e, struct frame, elem);
        if (f->page == page)
        {
            list_remove(e);
            free(f);
            break;
        }
    }
    lock_release (&table_lock);
    palloc_free_page (page);
}