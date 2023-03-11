#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct process_status	
  {	
    int exit_code;	                        /* Child’s exit code. */
    int pid;	                              /* Child process's id. */
    bool is_exited;	                        /* True when execution of process is done. O.w. is false */
    struct semaphore ws;                    /* Initialized to 0. Decrement by parent on wait and increment by child on exit. */
    int rc;	                                /* Initialized to 2. Decrement if either child or parent exits. */
    struct lock rc_lock;	                  /* Lock to protect EXIT_CODE and REF_COUNT. */
    struct list_elem elem;                  /* Stored in parent thread. */
  };

struct process_status *find_child (struct thread *, tid_t);

#endif /* userprog/process.h */
