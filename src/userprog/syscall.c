#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"


static void syscall_handler (struct intr_frame *);

static void exit (int status);
static int32_t exec (char *cmd);
static int wait (int pid);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);
static int open (const char *file);
static void close (int fd);
static int filesize (int fd);
static unsigned tell (int fd);
static void seek (int fd, unsigned position);
static int read (int fd, void *buffer, unsigned size);
static int write (int fd, const void *buffer, unsigned size);

static bool is_valid_char_pointer(char* str);
static bool is_valid_integer(void* pointer);


static int get_user (const uint8_t*);
static bool put_user (uint8_t*, uint8_t);
static int get_user_int (const uint8_t*);

static struct lock file_lock;

static unsigned next_fd = 2;

static struct file_node
{
  struct file* owned_file;
  struct thread* owner_thread;  
};

static struct file_node *files[MAX_FILES];

static void
block_filesystem (void)
{
  if (!lock_held_by_current_thread(&file_lock))
    lock_try_acquire(&file_lock);
}

static void
unblock_filesystem (void)
{
  if (lock_held_by_current_thread(&file_lock))
    lock_release(&file_lock);
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
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
      if (!is_valid_integer(arg+1) || !is_valid_integer(arg+2) || !is_valid_integer(arg+3))
        exit(-1);
      f->eax = write (*(arg+1), (void*)*(arg+2), *(arg+3));
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

    case SYS_CREATE:
      if (!is_valid_char_pointer(arg+1) || !is_valid_integer(arg+2))
        exit(-1);
      f->eax = create((char*) *(arg+1), *(arg+2));
      break;

    case SYS_REMOVE:
      if (!is_valid_char_pointer(arg+1))
        exit(-1);
      f->eax = remove ((char*) *(arg+1));
      break;

    case SYS_OPEN:
      if (!is_valid_char_pointer(arg+1))
        exit(-1);
      f->eax = open ((char*)*(arg+1));
      break;

    case SYS_CLOSE:
      if (!is_user_vaddr(arg+1) || !is_valid_integer(arg+1))
        exit(-1);
      close(*(arg+1));
    break;

    case SYS_FILESIZE:
      if (!is_valid_integer(arg+1))
        exit(-1);
      f->eax = filesize (*(arg+1));
    break;

    case SYS_TELL:
      if (!is_valid_integer(arg+1))
        exit(-1);
      f->eax = tell(*(arg+1));
    break;

    case SYS_SEEK:
      if (!is_valid_integer(arg+1) || !is_valid_integer(arg+2))
        exit(-1);
      seek(*(arg+1),*(arg+2));
    break;

    case SYS_READ:
      if (!is_valid_integer(arg+1) || !is_valid_char_pointer((char*)(arg+2)) || !is_valid_integer(arg+3))
        exit(-1);
      f->eax = read (*(arg+1), (void*)*(arg+2), *(arg+3));
    break;

    default:
      thread_exit ();
    }
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
create (const char *file, unsigned initial_size)
{
  char* aux = file;
  if (file != NULL && get_user_int((uint8_t*)aux) != -1) {
    block_filesystem ();
    bool status = filesys_create (file, initial_size);
    unblock_filesystem ();
    return status;
  }
  exit(-1);
}


static bool
remove (const char *file) {
  char *aux = file;
  if (get_user_int((uint8_t*)aux) != -1)
  {
    block_filesystem();
    bool status = filesys_remove(file);
    unblock_filesystem();
    return status;
  }
  return false;
}


static int
open (const char *file)
{
  char *aux = file;
  if (get_user_int((uint8_t*)aux) == -1)
    exit(-1);
  block_filesystem();
  struct file *file_open = filesys_open (file);
  if (file_open == NULL)
  {
    unblock_filesystem();
    return -1;
  }
  struct file_node *node = malloc (sizeof (struct file_node));
  node->owner_thread = thread_current();
  node->owned_file = file_open;
  files[next_fd] = node;

  struct file_wrapper *fw = malloc (sizeof (struct file_wrapper));
  fw->file_descriptor = next_fd;
  fw->file = file_open;
  //thread_current()->opened_files[next_fd] = fw;
  //file_deny_write(file_open);
  unblock_filesystem();
  return next_fd++;

}

static void
close (int fd)
{
  if (fd > 128 || fd < 2)
    exit(-1);
  if (files[fd] == NULL)
    exit(-1);
  // Trata de cerrar el archivo alguien que no lo creó
  if (files[fd]->owner_thread != thread_current())
    exit(-1);
  block_filesystem();
  //file_allow_write(files[fd]->owned_file);
  file_close(files[fd]->owned_file);

  files[fd] = NULL;
  //thread_current()->opened_files[fd] = NULL;
  
  unblock_filesystem();
}

static int
filesize (int fd)
{
  if (fd > 128)
    exit(-1);
  block_filesystem();
  int length = file_length (files[fd]->owned_file);
  unblock_filesystem();
  return length;
}


static unsigned
tell (int fd)
{
  block_filesystem();
  unsigned ret = file_tell(files[fd]->owned_file);
  unblock_filesystem();
  return ret;
}


static void
seek (int fd, unsigned position)
{
  block_filesystem();
  file_seek(files[fd]->owned_file, position);
  unblock_filesystem();
}


static int
read (int fd, void *buffer, unsigned size)
{
  if (!is_user_vaddr((uint8_t*)buffer) || !is_user_vaddr((uint8_t*)fd))
    exit(-1);
  if (fd > 128 || fd < 0)
    exit(-1);
  block_filesystem();
  if (fd == STDIN_FILENO)
  {
    int i;
    uint8_t* buffer = (uint8_t*) buffer;
    for (i = 0; i < size; i++)
      buffer[i] = input_getc();
    unblock_filesystem();
    return size;
  }
  if (files[fd] == NULL)
  {
    unblock_filesystem();
    exit(-1);
  }
  if (files[fd]->owner_thread != thread_current())
  {
    unblock_filesystem();
    exit(-1);
  }
  struct file *f = files[fd]->owned_file;
  int bytes = file_read(f, buffer, size);
  unblock_filesystem();
  return bytes;
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


/* Write Syscall Implementation */
static int
write (int fd, const void *buffer, unsigned size)
{
  /*
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

  // Writes in standar output
  putbuf (buffer, size);

  return size;*/
  if (get_user_int((uint8_t*)buffer) == -1)
    exit(-1);
  if (fd < 0 || fd > 128)
    exit(-1);
  if (fd == STDOUT_FILENO)
  {
    putbuf(buffer, size);
    return size;
  }
  block_filesystem();
  if (files[fd] == NULL)
  {
    unblock_filesystem();
    exit(-1);
  }
  struct file *f = files[fd]->owned_file;
  int bytes = file_write(f, buffer, size);
  unblock_filesystem();
  return bytes;
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