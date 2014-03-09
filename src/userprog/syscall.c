#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

static int32_t write (int32_t *);

static int get_user (const uint8_t*);
static bool put_user (uint8_t*, uint8_t);
static int get_user_int (const uint8_t*);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int32_t *arg = (int32_t *)f->esp;
  if (! is_user_vaddr((int8_t *)arg + 3))
    thread_exit();

  int32_t syscall_nr = get_user_int ((uint8_t *) arg);
  if (syscall_nr == -1)
    thread_exit();

  switch (syscall_nr)
    {
    case SYS_WRITE:
      f->eax = write (++arg);
      break;

    // Other syscalls are not supported
    default:
      thread_exit ();
    }
}

/* Write Syscall Implementation */
static int32_t 
write (int32_t *sp)
{
  int fd;
  char *buffer;
  unsigned size;

  // Here no verification is performed to retrieve 
  // the values given by the user
  // TODO: get the values using auxiliar functions
  // and verify their validity.
  fd = *sp;
  buffer = (char *) *++sp;
  size = (unsigned) *++sp;

  /* Writes in standar output */
  putbuf (buffer, size);

  return size;
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

/* Reads a 32-bit integer at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the i32-bit integer value if successful, -1 if a segfault
   occurred. */
static int32_t
get_user_int (const uint8_t *uaddr)
{
  int32_t result;
  asm ("movl $1f, %0; movl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}
