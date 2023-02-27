تمرین گروهی ۱/۰ - آشنایی با pintos
======================

شماره گروه:
-----
> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

Mehrad Milanloo mehrad_milanloo@yahoo.com

نام و نام خانوادگی <example@example.com> 

نام و نام خانوادگی <example@example.com> 

نام و نام خانوادگی <example@example.com> 

مقدمات
----------
> اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.


> لطفا در این قسمت تمامی منابعی (غیر از مستندات Pintos، اسلاید‌ها و دیگر منابع  درس) را که برای تمرین از آن‌ها استفاده کرده‌اید در این قسمت بنویسید.

آشنایی با pintos
============
>  در مستند تمرین گروهی ۱۹ سوال مطرح شده است. پاسخ آن ها را در زیر بنویسید.


## یافتن دستور معیوب

۱.
با توجه به فایل do-nothing.result، هنگام دسترسی به آدرس مجازی 0xc0000008، برنامه crash می‌کند.

۲.
آدرس مجازی دستوری که موجب crash شد،eip = 0x8048757 است.
دقت کنید که eip مخفف عبارت Extended Instruction Pointer است و برای دنبال کردن آدرس دستور فعلی که در برنامه اجرا می‌شود استفاده می‌شود.

۳.

با اجرای دستور زیر، کد اسمبلی فایل اجرایی do-nothing را تحلیل می‌کنیم.
objdump -drS do-nothing
سپس با بررسی آدرس‌های دستورها، متوجه می‌شویم که دستوری که موجع‌به crash شده، در تابع 
void _start (int argc, char *argv[]) {}
قرار دارد و اسمبلی دستور آن mov    0x24(%esp),%eax است.

۴.
کد C مربوطه در آدرس pintos/src/lib/user و در فایل entry.c وجود دارد. کد این تابع این‌گونه است:
void
_start (int argc, char *argv[]) {
  exit (main (argc, argv));}

  برای توضیح دستورات disassemble شده ابتدا Calling Convention در اسمبلی 80x86 را توضیح مختصری می‌دهیم.
  ابتدا Caller تمام آرگومان‌های تابع را در استک موردنظر Push می‌کند.استک نیز از بالا به پایین پر می‌شود و با هر پوش، اشاره‌گر استک کاهش می‌یابد. سپس Caller آدرس دستور بعدی خود را در استک تابع پوش می‌کند و به اولین دستور تابع صدا زده‌شده می‌پرد.اکنون تابع اجرا می‌شود و وقتی نوبت اجرای آن رسیده، اشاره‌گر به آدرس return اشاره می‌کند و آرگومان‌ها به‌ترتیب بالای آن قرار دارند. اگر تابع مقداری برای return داشته باشد، آن را در رجیستر eax ذخیره می‌کند. تابع نیز با Pop کردن مقدار return و پریدن به آدرس آن، از اسکوپ خود خارج می‌شود و آرگومان‌ها را نیز از استک Pop می‌کند.
  با توجه به توضیحات بالا، می‌توانیم دستورات اسمبلی را بهتر تحلیل کنیم. (دقت کنید که رجیستر esp همان Stack Pointer است و درباره‌ی عملکرد رجیستر eax توضیح داده شد.)
 8048754:	83 ec 1c             	sub    $0x1c,%esp
 8048757:	8b 44 24 24          	mov    0x24(%esp),%eax
 804875b:	89 44 24 04          	mov    %eax,0x4(%esp)
 804875f:	8b 44 24 20          	mov    0x20(%esp),%eax
 8048763:	89 04 24             	mov    %eax,(%esp)
 8048766:	e8 35 f9 ff ff       	call   80480a0 <main>
 804876b:	89 04 24             	mov    %eax,(%esp)
 804876e:	e8 49 1b 00 00       	call   804a2bc <exit>

 sub    $0x1c,%esp
 این خط اشاره‌گر استک را به مقدار ۲۸ بایت جابجا می‌کند و در واقع ۲۸ بایت به استک فضا می‌دهد.
 mov    0x24(%esp),%eax
 این دستور اولین آرگومان یعنی argc را در رجیستر eax ذخیره می‌کند.
 mov    %eax,0x4(%esp)
 اکنون با این دستور، مقدار eax را به‌عنوان آرگومان argc تابع main در استک ذخیره می‌کنیم.
 mov    0x20(%esp),%eax
 این دستور اولین آرگومان یعنی آدرس argv را در رجیستر eax ذخیره می‌کند.
 mov    %eax,(%esp)
 اکنون با این دستور، آدرس eax را به‌عنوان آرگومان argv تابع main در استک ذخیره می‌کنیم.
 call   80480a0 <main>
 این دستور به تابع main پرش می‌کند.
 (دقت کنید همانطور که در بالاتر توضیح دادیم، ابتدا مقدار return address را در استک ذخیره می‌کند.)
 mov    %eax,(%esp)
 این دستور خروجی تابع main که در رجیستر eax قرار می‌گیرد را به‌عنوان آرگومان تابع exit در استک ذخیره می‌کند.
 call   804a2bc <exit>
 با این دستور، تابع exit فراخوانی می‌شود.


۵.
در سیستم‌عامل PintOS اجرای برنامه‌های کاربر از تابع _start شروع می‌شود. بنابراین در استک این تابع ورودی‌های آن پوش نشده‌اند. به‌همین خاطر برای دسترسی به مقادیر آرگومان‌های argc و argv به‌اندازه‌ی ۳۶ بایت رو به بالا حرکت می‌کند.  دقت کنید در سیستم‌عامل PintOS، حافظه‌ی مجازی به دو قسمت تقسیم می‌شود. قسمت اول که از آدرس 0 تا PHYS_BASE است، مربوط به فضای کرنل است. از آن به بعد نیز مربوط به فضای کاربر است. به‌طور دیفالت برنامه‌ی کاربر که شروع می‌شود،‌از آدرس دیفالت 0xc0000008 است. همانطور که گفتیم بعد از اختصاص ۲۸ بایت به استک و پایین آمدن در استک، به مقدار ۳۶ بایت رو به بالا حرکت می‌کند که باعث می‌شود مرز User Space و Kernel Space را رد کند که سبب Page Fault: rights violation error می‌شود.

## به سوی crash

۶.
Firstly we'll step into `process_execute()` function by running commands below in GDB:
```bash
  b run_task
  c
  n 3
  step
```
Afterwards, by running the command below, will be able to identify which thread is running the `process_execute()` function:
```bash
info threads
```
The thread we're searching for is the thread with `main` Id.
At the end we will run the `dumplist &all_list thread allelem` command and mention the output which is the set of all the threads present in PintOS at this time. (Containing their struct threds)
```
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec <incomplete sequence \357>, priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```

۷.
Simply apply the `bt` command to print the backtrace. the output is shown below (Containing line of C code corresponding to each function call):
```
#0  process_execute (file_name=file_name@entry=0xc0007d50 \"do-nothing\") at ../../userprog/process.c:36
#1  0xc0020268 in run_task (argv=0xc00357cc <argv+12>) at ../../threads/init.c:288
#2  0xc0020921 in run_actions (argv=0xc00357cc <argv+12>) at ../../threads/init.c:340
#3  main () at ../../threads/init.c:133
```
```C
#0: sema_init (&temporary, 0);
#1: process_wait (process_execute (task));
#2: a->function (argv);
#3: run_actions (argv);
```

۸.
Firstly we'll step into `start_process()` function by running commands below in GDB:
```bash
  b start_process
  c
```
Just as before, we run this command:
```bash
dumplist &all_list thread allelem
```
the result is shown:
```
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0037314 <temporary+4>, next = 0xc003731c <temporary+12>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```
so the threads running `start_process()` funtion are `main`, `idle`, `do-nothing`.

۹.
By tracing the code, it is conculded that the thread is created in the `process_execute` function in `proccess.c` file:
```C
tid_t
process_execute (const char *file_name)
{
  char *fn_copy;
  tid_t tid;

  sema_init (&temporary, 0);
  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy);
  return tid;
}
```
Here, the calling function, `thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);`, will create a new kernel thread running the start_process function. The code is shown below:
```C
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux)
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  /* Add to run queue. */
  thread_unblock (t);

  return tid;
}
```

۱۰.
We continue debuging untill we reach the `load` function:
```bash
n 6
```
Afterwards, we print the `if_` struct (using command: `print/x if_`) to find `eip` and `esp` values:
```
{edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23 ds = 0x23, vec_no = 0x0, error_code = 0x0, frame_pointer = 0x0, eip = 0x8048754, cs = 0x1b, eflags = 0x202, esp = 0xc0000000, ss = 0x23}
```
So, values are:
`eip = 0x8048754`, `esp = 0xc0000000`.

۱۱.
Firstly, we continue debuging untill we reach executing `iret`:
```bash
n
step
n 6
step
```


۱۲.
Using GDB commands, we reach `asm volatile` function and step into it. Then we continue to `iret` and print registers.
```bash
n
step
n 6
step
info registers
```
The output is:
```
eax            0x0      0
ecx            0x0      0
edx            0x0      0
ebx            0x0      0
esp            0xc0000000       0xc0000000
ebp            0x0      0x0
esi            0x0      0
edi            0x0      0
eip            0x8048754        0x8048754
eflags         0x202    [ IF ]
cs             0x1b     27
ss             0x23     35
ds             0x23     35
es             0x23     35
fs             0x23     35
gs             0x23     35
```
As we expected, the output is similar to `if_` struct values. (It only doesn't contain `struct intr_frame`, `vec_no`, `error_code` which are discarded in `intr_exit`).

۱۳.
The commands and their outputs are mentioned below:
Command:
```bash
loadusersymbols tests/userprog/do-nothing
```
Output:
```
add symbol table from file "tests/userprog/do-nothing" at
        .text_addr = 0x80480a0
```
Command:
```bash
bt
```
Output:
```
#0  _start (argc=<unavailable>, argv=<unavailable>) at ../../lib/user/entry.c:8
```
Command:
```bash
disassemble
```
Output:
```
Dump of assembler code for function _start:
=> 0x08048754 <+0>:     sub    $0x1c,%esp
   0x08048757 <+3>:     mov    0x24(%esp),%eax
   0x0804875b <+7>:     mov    %eax,0x4(%esp)
   0x0804875f <+11>:    mov    0x20(%esp),%eax
   0x08048763 <+15>:    mov    %eax,(%esp)
   0x08048766 <+18>:    call   0x80480a0 <main>
   0x0804876b <+23>:    mov    %eax,(%esp)
   0x0804876e <+26>:    call   0x804a2bc <exit>
End of assembler dump.
```
Command:
```bash
stepi
stepi
```
Output:
```
pintos-debug: a page fault exception occurred in user mode
pintos-debug: hit 'c' to continue, or 's' to step to intr_handler
0xc0021b95 in intr0e_stub ()
```
Command:
```bash
bt
```
Output:
```
#0  0xc0021b95 in intr0e_stub ()
```
Command:
```bash:
btpagefault
```
The wanted output:
```
#0  _start (argc=<unavailable>, argv=<unavailable>) at ../../lib/user/entry.c:9
```


## دیباگ

۱۴.

۱۵.

۱۶.

۱۷.

۱۸.

۱۹.