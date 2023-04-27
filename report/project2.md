# گزارش تمرین گروهی ۲

## گروه ۴

-----
> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

Mehrad Milanloo <mehrad_milanloo@yahoo.com>

Parnian Razavipour <razaviparnian81@yahoo.com>

Alireza Noroozi <noroozi.alirezaa@gmail.com> 

Hossein Alihosseini <alihosseini@sharif.edu> 


# وظیفه۱: ساعت زنگ‌دار

There was no specific change compared to the Design doc. the modified structures are:

```C
struct thread
  {
    /* Owned by thread.c. */
    ...  
   int64_t time_ticks;              // Used for wakeing thread up (ticks since OS booting)
    ...
  }
```
The `struct list_elem slept_thread` was not used because `struct list_elem elem` is defined in the `struct thread` and is shared between `thread.c` and `synch.c`.

Also `struct lock sleep_thread_lock` wan't used in the code. The intention was to thread safe `already_slept` and would be used in the interrupts. But obviously we can not use locks while handling the interrupt.

The function `thread_sleep()` saves the wake-up time in the current thread's struct. Afterwards adds the thread (with respect to the thread's wakeup time) into the `already_slept` list and blocks it. On the otherside, `update_slept_threads()` function is called during each tick and checks the list of slept threads. If any thread should be waked-up, this functions removes it from the `already_slept` list and unblocks it.



# وظیفه۲: زمان‌بند اولویت‌دار
