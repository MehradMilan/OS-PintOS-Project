#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "filesys/directory.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/vaddr.h"
#include <string.h>
#include "devices/shutdown.h"
#include "userprog/process.h"

#define MAX_SYSCALL_ARGUMENTS 10
#define MAX_NAME_SIZE 14

static void syscall_handler (struct intr_frame *);

void sys_exit (int status);
int sys_write (int fd_num, const char *buffer, unsigned size);
int sys_create (const char* name, unsigned initial_size);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void
sys_exit (int status)
{
  struct thread *cur = thread_current();
  printf ("%s: exit(%d)\n", &cur->name, status);
  cur->ps->exit_code = status;
  cur->ps->is_exited = 1;
  thread_exit();
}

static int
validate_addr (void *addr)
{
  if (!addr || !is_user_vaddr (addr)
      || !pagedir_get_page (thread_current ()->pagedir, addr))
    {
      sys_exit (-1);
    }
  return 1;
}

static void
validate_args (void *esp, int argc)
{
  if (argc > MAX_SYSCALL_ARGUMENTS)
    {  sys_exit(-1);
      return 1;  }
  validate_addr (esp + (argc + 1) * sizeof (void *) - 1);
return 1;
}

struct file* get_fd_by_num(int num) {
  struct list *fd_list = &thread_current ()->fd_list;
  struct list_elem *e;

  for (e = list_begin (fd_list); e != list_end (fd_list);
       e = list_next (e))
    {
      struct file_descriptor *f = list_entry (e, struct file_descriptor, elem);
      if (f->fd == num)
        return f;
    }
  return NULL;
}


int 
sys_open (const char *name)
{ 
  if(!name)
    return -1; 
  struct file *f = filesys_open(name);
  if (!f) 
    return -1;
  else { 
    struct thread *cur = thread_current();
    struct file_descriptor *new_fd = malloc(sizeof(struct file_descriptor));
    new_fd->file = f;
    new_fd->dir = NULL;
    int fd_num = cur->fd_count;
    new_fd->fd = fd_num ;
    cur->fd_count += 1;
    list_push_back(&cur->fd_list, &new_fd->elem);
    return fd_num; 
  }
}


int
sys_close (int fdnum) 
{
  if (fdnum <= STDOUT_FILENO)
    sys_exit(-1);
  struct file_descriptor *fd = get_fd_by_num(fdnum);
  if (!fd)
    return -1;
  file_close(fd->file);
  list_remove(&fd->elem);
  free(fd);
  return 0;
}


int
sys_write (int fd_num, const char *buffer, unsigned size)
{ 
  if (fd_num <= STDIN_FILENO || !fd_num || fd_num > thread_current()->fd_count) {
    sys_exit(-1);
  }
  if (size <= 0)
    return 0;
  int bytes_written = size;
  if (fd_num == STDOUT_FILENO)
  {
    putbuf (buffer, size);
    bytes_written = size;
  } else {
    struct file_descriptor *f_descriptor = get_fd_by_num(fd_num);
    struct file *f = f_descriptor->file; 
    if (!f)
      return -1;
    bytes_written = file_write (f, buffer, size);
  }
  return bytes_written;
}

int
sys_create(const char* name, unsigned initial_size)
{
  char * ptr; 
  for (ptr = name ; validate_addr (ptr) && *ptr != '\0'; ++ptr);
  if(name == (char *) NULL)
    sys_exit(-1);
  else if (strlen(name) > MAX_NAME_SIZE)
     return 0;
  else {
    return filesys_create(name, initial_size , false);
  }
}

int
sys_read(int fd, char * buff, unsigned size) {
  if (fd == STDOUT_FILENO)
    sys_exit(-1);
  if (fd < 0 || fd > thread_current()->fd_count) 
    sys_exit(-1);
  if (size <= 0)
    return 0;
  struct file_descriptor *f_descriptor = get_fd_by_num(fd);
  struct file *f = f_descriptor->file; 
  if (!f)
    return -1;
  return file_read(f, buff, size);
}

void
sys_seek(int fd_num, unsigned pos) {
  if (fd_num <= STDIN_FILENO || !fd_num || fd_num > thread_current()->fd_count)
    sys_exit(-1);
  if (fd_num <= STDOUT_FILENO)
    sys_exit(-1);
  struct file_descriptor *f_descriptor = get_fd_by_num(fd_num);
  if (f_descriptor != NULL)
    file_seek(f_descriptor->file, pos);
}

int
sys_tell(int fd_num) {
  if (fd_num <= STDIN_FILENO || !fd_num || fd_num > thread_current()->fd_count)
    sys_exit(-1);
  if (fd_num <= STDOUT_FILENO)
    sys_exit(-1);
  struct file_descriptor *f_descriptor = get_fd_by_num(fd_num);
  if (f_descriptor == NULL)
    return -1;  
  return file_tell(f_descriptor->file);
}

int
sys_filesize(int fd_num) {
  if (fd_num < 2)
    sys_exit(-1);
  else {
    struct file_descriptor *f_descriptor = get_fd_by_num(fd_num);
    if (!f_descriptor)
      return -1;
    struct file *file_ = f_descriptor->file; 
    return file_length(file_); 
  }
}

void sys_halt(void) {
  shutdown_power_off();
}

tid_t
sys_exec(char *fn) {
  return process_execute(fn);
}

int
sys_wait(tid_t t) {
  return process_wait(t);
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

  validate_addr ((void *) args);
  validate_addr ((void *) args + sizeof (void *) - 1);

  if (args[0] == SYS_EXIT) {
    validate_args(f->esp, 1);
    f->eax = args[1];
    struct thread *cur = thread_current();
    printf ("%s: exit(%d)\n", &cur->name, args[1]);
    cur->ps->exit_code = args[1];
    cur->ps->is_exited = 1;
    thread_exit();
  } else if (args[0] == SYS_WRITE) {
    validate_args (f->esp, 3);
    validate_addr (args[2]);
    f->eax = sys_write(args[1], (const void*) args[2], (unsigned) args[3]);
  } else if (args[0] == SYS_CREATE) {
    validate_args (f->esp, 2);
    f->eax = sys_create((const char *) args[1], args[2]);
  } else if (args[0] == SYS_REMOVE) {
    validate_addr((const char *) args[1]);
    validate_args (f->esp, 1);
    f->eax = filesys_remove(args[1]);
  } else if (args[0] == SYS_OPEN) {
    validate_args (f->esp, 1);
    validate_addr(args[1]);
    f->eax = sys_open((char *)args[1]); 
  } else if (args[0] == SYS_PRACTICE) {
    validate_args (f->esp, 1);
    f->eax = args[1] + 1;
  } else if (args[0] == SYS_CLOSE) {
     validate_args (f->esp, 1);
     f->eax = sys_close(args[1]);
  } else if (args[0] == SYS_FILESIZE) {
    validate_args (f->esp, 1);
    f->eax = sys_filesize(args[1]);
  } else if (args[0] == SYS_SEEK) {
    sys_seek(args[1], args[2]);
  } else if (args[0] == SYS_TELL) {
    f->eax = sys_tell(args[1]);
  } else if (args[0] == SYS_HALT) {
    shutdown_power_off();
  } else if (args[0] == SYS_READ) {
    validate_args(f->esp, 3);
    validate_addr(args[2]);
    f->eax = sys_read(args[1], args[2], args[3]);
  } else if (args[0] == SYS_EXEC) {
    validate_args(f->esp, 1);
    validate_addr(args[1]);
    f->eax = sys_exec(args[1]);
  } else if (args[0] == SYS_WAIT) {
    validate_args(f->esp, 1);
    f->eax = sys_wait((tid_t) args[1]);
  }
  else if ( args[0] == SYS_MKDIR) {
    sys_mkdir(f, (char *) args[1]);
  } else if ( args[0] == SYS_ISDIR){
    sys_isdir(f, args[1]); }
  else if ( args[0] == SYS_READDIR) {
    sys_readdir(f, args[1], (char *) args[2]);
  } else if ( args[0] == SYS_CHDIR){
    sys_chdir(f, (char *) args[1]);
  }


}




//dir  



void
  sys_isdir (struct intr_frame *f, int fid) {
  struct file_descriptor *descriptor = get_fd_by_num(fid);
  if (descriptor != NULL) {
    struct file *file = descriptor->file;

    if (file != NULL) {
      struct inode *inode = file_get_inode(file);
      f->eax =  is_directory_inode(inode);
    } else {
      f->eax = -1;
    }
  } else {
    f->eax = -1;
  }
}




struct 
dir* open_valid_directory(char *dir_name, struct intr_frame *f) {
  if (!validate_addr(dir_name)) {
    sys_exit(-1);
    return NULL;
  }

  struct dir *new_dir = dir_open_path(dir_name);

  return new_dir;
}

void 
sys_chdir(struct intr_frame *f, char *dir_name) {
  f->eax = false;
  struct dir *new_dir = open_valid_directory(dir_name, f);
  if (new_dir) {
    struct thread *t = thread_current();
    if (t->working_dir) {
      dir_close(t->working_dir);
    }
    t->working_dir = new_dir;
    f->eax = true;
  }
}






void 
sys_mkdir (struct intr_frame *f, char *dir_name)
{
    if (!validate_addr (dir_name))
    sys_exit (-1);
    else{
      f->eax = filesys_create (dir_name, 0, true);
    }
}



void 
sys_readdir (struct intr_frame *f, int fid, char *name)
{
  if (!validate_addr (name)) {
    sys_exit (-1);
    return;
  }

  struct file_descriptor *fd = get_fd_by_num(fid);
  f->eax = -1;
  if (fd && fd->file) {
    struct inode *inode = file_get_inode(fd->file);
    if (inode && is_directory_inode(inode)) {
      struct dir *dir = fd->dir;
      if (dir) {
        f->eax = dir_readdir(dir, name);
      }
    }
  }
}

