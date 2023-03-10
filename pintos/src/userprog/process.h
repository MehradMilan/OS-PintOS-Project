#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct process_status	
  {	
    int pid;	
    int exit_code;	
    bool is_exited;	
    struct semaphore ws;
    int rc;	
    struct lock rc_lock;	
    struct list_elem children_elem;
  };

struct process_status *find_child (struct thread *, tid_t);

#endif /* userprog/process.h */
