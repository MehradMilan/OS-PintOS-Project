#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/vaddr.h"

#define MAX_SYSCALL_ARGUMENTS     10

#define CHECK_ARGS(args, count, ...) if (!check_arguments((args), (count), __VA_ARGS__)) EXIT_WITH_ERROR

#define EXIT_WITH_ERROR { printf ("%s: exit(%d)\n", &thread_current ()->name, -1); thread_exit (); return; }

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool
is_user_mapped_memory(const void *address)
{
  if (is_user_vaddr(address))
    return (pagedir_get_page(thread_current ()->pagedir, address) != NULL);
  return false;
}

/* Checks if arguments for a system call are valid. It
 * first checks that ARRAY has ARG_COUNT members in user memory
 * (it doesn't check for only stack) and then for every argument,
 * it checks that if it's an address, it is in user space.
 *
 * The function call check_arguments(arr, 3, true, false, true)
 * checks for a system call which has 3 arguments which first and
 * last one of them is an address.
 */
static bool
check_arguments(uint32_t* array, uint32_t arg_count, uint32_t is_address, ...)
{
  if (arg_count > MAX_SYSCALL_ARGUMENTS)
    return false;

  if (!is_user_mapped_memory(array) || !is_user_mapped_memory((void *)(array + arg_count + 1) - 1))
    return false;

  if (is_address && !is_user_mapped_memory((void *) array[1]))
    return false;

  va_list args;
  va_start(args, is_address);
  for (size_t i = 2; i <= arg_count; i ++) {
    bool should_check_address = va_arg (args, int);
    if (should_check_address && !is_user_mapped_memory((void *)array[i]))
      return false;
  }
  va_end(args);

  return true;
}



struct file* get_file_by_fd(int fd) {
  struct list *fd_list = &thread_current ()->fd_list;
  struct list_elem *e;

  for (e = list_begin (fd_list); e != list_end (fd_list);
       e = list_next (e))
    {
      struct file_descriptor *f = list_entry (e, struct file_descriptor, elem);
      if (f->fd == fd)
        return f->file;
    }
  return NULL;
}


int
sys_write (int fd_num, const void *buffer, unsigned size)
{
  int bytes_written = 0;

  if (fd_num == STDOUT_FILENO)
    {
      putbuf (buffer, size);
      bytes_written = size;
    }
  else if (fd_num == STDIN_FILENO )
    {
      /* TODO: = error output */
    }
  else
    {
      struct file * f = get_file_by_fd( fd_num ); 
      if (f != NULL )
        {
          bytes_written = file_write (f , buffer, size);
        }
    }

   return bytes_written;
}


static void
syscall_handler (struct intr_frame *f UNUSED)
{
  if (!is_kernel_vaddr(f) || !is_kernel_vaddr((void *)(f + 1) - 1))
    EXIT_WITH_ERROR;
  uint32_t* args = ((uint32_t*) f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  CHECK_ARGS(args, 0, false);

  if (args[0] == SYS_EXIT)
    {
      f->eax = args[1];
      printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);
      thread_exit ();

    }

  if ( args[0] == SYS_WRITE)
    {
      int fd = args[1];
      const void* buffer = (const void*) args[2];
      unsigned size = (unsigned) args[3];
      f->eax = sys_write(fd, buffer, size);
    }
  
  if ( args[0] == SYS_CREATE)
    {
      CHECK_ARGS(args, 2, true, false);
      f->eax = filesys_create((const char *) args[1], args[2]);
    }
  if ( args[0] == SYS_PRACTICE)
    {
      CHECK_ARGS(args, 1, false);
      f->eax = args[1] + 1;
    }
  
}



