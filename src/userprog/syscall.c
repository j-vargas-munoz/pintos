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
exit (int status)
{
  struct thread *exiting_thread = thread_current();
  exiting_thread->exit_status = status;
  printf("%s: exit(%d)\n", exiting_thread->name, status);
  thread_exit();
}

static int32_t
exec (char *cmd)
{
  int32_t syscall = get_user_int ((uint8_t *) cmd);
  if (syscall == -1) {
    printf ("%s: exit(%d)\n", thread_current()->name,-1);
    thread_exit();
  }
  tid_t child_tid = process_execute(cmd);
  return child_tid;
}


static int
wait (int pid)
{
  return process_wait(pid);
}


static bool
is_valid_char_pointer(char* str)
{
  char *aux = str;
  while ((void*)aux < PHYS_BASE && *aux != '\0')
    if (get_user((uint8_t*)aux++) == -1) {
      return false;
    }
  return (void*)aux < PHYS_BASE;
}


static bool
is_valid_integer(void* pointer)
{
  return pointer < PHYS_BASE;
}


static void
syscall_handler (struct intr_frame *f) 
{
  int32_t *arg = (int32_t *)f->esp;
  if (!is_user_vaddr((int8_t*)arg))
    exit(-1);

  if (! is_user_vaddr((int8_t *)arg + 3))
    exit(-1);

  int32_t syscall_nr = get_user_int ((uint8_t *) arg);
  if (syscall_nr == -1)
    exit(-1);

  switch (syscall_nr)
    {
    case SYS_WRITE:
      f->eax = write (++arg);
      break;

    case SYS_HALT:
      shutdown_power_off();
      break;

    case SYS_EXIT:
      if (!is_valid_integer(++arg))
        exit(-1);
      exit(*((int*)arg));
      break;

    case SYS_EXEC:
      if (!is_valid_char_pointer(++arg))
        exit(-1);
      f->eax = exec((char*) *arg);
      break;

    case SYS_WAIT:
      if (!is_valid_integer(++arg))
        exit(-1);
      f->eax = wait(*arg);
      break;

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