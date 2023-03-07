#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"

#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);


void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
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
  uint32_t* args = ((uint32_t*) f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

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
    
  
}



