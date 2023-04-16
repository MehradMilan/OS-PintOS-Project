# سیستم‌های عامل - تمرین گروهی دوم

## مشخصات گروه

>> نام، نام خانوادگی و ایمیل خود را در ادامه وارد کنید.

علیرضا نوروزی noroozi.alirezaa@gmail.com

محمدحسین علی‌حسینی alihosseini@sharif.edu 

مهراد میلانلو milanloomehrad@gmail.com

پرنیان رضوی‌پور razaviparnian81@yahoo.com

## مقدمه

>> اگر نکته‌ای درباره فایل‌های سابمیت شده یا برای TAها دارید، لطفا اینجا بیان کنید.

>> اگر از هر منبع برخط یا غیر برخطی به غیر از مستندات Pintos، متن درس، اسلایدهای درس یا نکات گفته شده در کلاس در تمرین گروهی استفاده کرده‌اید، لطفا اینجا آن(ها) را ذکر کنید.

## ساعت زنگ‌دار

### داده ساختارها

>> پرسش اول: تعریف `struct`های جدید، `struct`های تغییر داده شده، متغیرهای گلوبال یا استاتیک، `typedef`ها یا `enumeration`ها را در اینجا آورده و برای هریک در 25 کلمه یا کمتر توضیح بنویسید.

```c
/* devices/thread.c */

struct list already_slept;       // List of threads that already slept

struct lock sleep_thread_lock;   // Used for already_slept thread and making it thread-safe

int64_t time_ticks;              // Used for wakeing thread up (ticks since OS booting)
```

```c
/* threads/thread.h */

struct thread
{
  ...

  struct list_elem slept_thread; // Used for dealing with `already_slept` list.

  ...
};
```

### الگوریتم

>> پرسش دوم: به اختصار آن‌چه هنگام صدا زدن تابع `timer_sleep()` رخ می‌دهد و همچنین اثر `timer interrupt handler` را توضیح دهید.

در تابع `timer_sleep()` اتفاقات زیر به‌ترتیب رخ می‌دهد:

* متغیر `time_ticks` آپدیت می‌شود که مقدار جدید آن برابر است با تعداد `tick`های گذشته از زمان `boot`شدن سیستم عامل به‌علاوه تعداد `tick`ها هنگام خوابیدن `thread` است.

* وقفه‌ها غیرفعال شده و قفل `sleep_thread_lock` توسط ریسه دریافت می‌شود.
* ریسه بلاک می‌شود.
* وقفه‌ها فعال شده و قفل `sleep_thread_lock` توسط ریسه آزاد می‌شود.
  
همچنین در `timer interrupt handler` داریم:

* قفل `sleep_thread_lock` دریافت می‌شود.
* اگر ریسه‌ای وجود داشت که باید فعال می‌شد، مقدار `time_ticks` برای آن آپدیت می‌شود.
* وقفه‌ها غیرفعال شده و ریسه از لیست ریسه‌های خوابیده حذف می‌شود.
* درنهایت وقفه‌ها فعال می‌شوند.


>> پرسش سوم: مراحلی که برای کوتاه کردن زمان صرف‌شده در `timer interrupt handler` صرف می‌شود را نام ببرید.

ترتیب اعضای `already_slept` همواره به‌صورت صعودی می‌باشد و هنگام درج عضو جدید نیز به این نکته توجه داریم. در نتیجه هربار ماکسیمم یک عضو اضافه که نیاز به بیدار شدن ندارد را بررسی می‌کنیم. زیرا اعضایی را بررسی می‌کنیم که `time_ticks` آن از تعداد تیک‌های سیستم عامل بیشتر باشد.

### همگام‌سازی

>> پرسش چهارم: هنگامی که چند ریسه به طور همزمان `timer_sleep()` را صدا می‌زنند، چگونه از `race condition` جلوگیری می‌شود؟

قفل `sleep_thread_lock` بدین منظور تعریف شده تا هنگام دسترسی به اعضای `already_slept` و یا حذف اعضا، با `race condition` مواجه نشویم.

>> پرسش پنجم: هنگام صدا زدن `timer_sleep()` اگر یک وقفه ایجاد شود چگونه از `race condition` جلوگیری می‌شود؟

در مراحلی که برای `timer_sleep()` در قسمت‌های قبلی توضیح دادیم، وقفه‌ها را غیرفعال می‌کنیم. پس `race condition` رخ نمی‌دهد.

### منطق

>> پرسش ششم: چرا این طراحی را استفاده کردید؟ برتری طراحی فعلی خود را بر طراحی‌های دیگری که مدنظر داشته‌اید بیان کنید.

لیست مرتب‌شده یکی از بهترین انتخاب‌هاست. مرتب‌بودن لیست این امکان را می‌دهد با کمترین تعداد `iteration` ریسه مدنظر را پیدا کرده و اردر زمانی الگوریتم خطی باشد.

## زمان‌بند اولویت‌دار

### داده ساختارها

>> پرسش اول: تعریف `struct`های جدید، `struct`های تغییر داده شده، متغیرهای گلوبال یا استاتیک، `typedef`ها یا `enumeration`ها را در اینجا آورده و برای هریک در ۲۵ کلمه یا کمتر توضیح بنویسید.

```c
// threads/thread.h

# define BASE_PRIORITY -1

# define MAX_PRIORITY 9

struct thread {
    ...

    int priority;                   // base priority for thread

    int priority_after_donation;    // priority of thread if it has donated its base priority

    bool is_donated;                // true if proirity is donated ow false

    struct list acquired_locks;     // order list of locks which this thread already has acquired.

    struct lock *waiting_lock;      // null if there is no lock that has been blocking this thread, ow blocking lock.

    ...
}
```

```c
/* threads/synch.h */

struct lock {
    ...

    int priority;               // initially is `BASE_PRIORITY` then it is highest priority value of threads which holds this lock.

    struct list_elem elem_lock; // used for dealing with `waiting_lock`

    ...
}
```

>> پرسش دوم: داده‌ساختارهایی که برای اجرای `priority donation` استفاده شده‌است را توضیح دهید. (می‌توانید تصویر نیز قرار دهید)

داده‌ساختارهای بالا درکنار یکدیگر می‌توانند این موضوع را پوشش دهند.
سناریوای را فرض کنید که ریسه اول می‌خواد قفل‌ای را بگیرد که در اختیار ریسه دوم است. در این‌صورت ریسه اول بلاک می‌شود و `waiting_lock` مقداردهی می‌شود. سپس اگر اولویت ریسه اول بالاتر از اولویت ریسه دوم بود، در این‌صورت باید `priority donation` اتفاق بی‌افتد. مقدار `is_donated` برای ریسه اول `true` می‌شود و اولویت آن بعد از اهدای اولویت `(priority_after_donation)` برابر می‌شود با اولویت پایه `(priority_after_donation)` ریسه دوم. همچنین مقدار `priority` در `lock` هم تغییر می‌کند و بیشتر می‌شود.
این عملیان مجدد برای ریسه دوم تکرار می‌شود و می‌خواهد قفل دیگری را بگیرد. مقدار `MAX_PRIORITY` بدین منطور برای بررسی‌های تودرتو در کد قرار گرفته است؛ یعنی فرآیند چک شدن این بررسی‌های تودرتو حداکثر به اندازه `MAX_PRIORITY` بار تکرار می‌شود.

### الگوریتم

>> پرسش سوم: چگونه مطمئن می‌شوید که ریسه با بیشترین اولویت که منتظر یک قفل، سمافور یا `condition variable` است زودتر از همه بیدار می‌شود؟

ما از لیست برای ذخیره‌سازی ریسه‌ها استفاده کردیم که ریسه‌ها نیز به صورت مرتب‌شده نسبت به `priority`شان  در لیست قرار گرفته‌اند. پس کافی‌‌ست اولین عضو لیست یا ریسه با بالاترین اولویت انتخاب شده و آن‌بلاک شود.

>> پرسش چهارم: مراحلی که هنگام صدازدن `lock_acquire()` منجر به `priority donation` می‌شوند را نام ببرید. دونیشن‌های تو در تو چگونه مدیریت می‌شوند؟

این مراحل در پرسش دوم تا حد خوبی توضیخ داده‌شد و اینجا بار دیگر مختصر توضیح داده می‌شود.
در ابتدا که همه وقفه‌ها غیرفعال می‌شوند. سپس ترد اول سعی می‌کند قفل‌ای را بگیرد. در صورتی که قفل در اختیار ترد دیگری نباشد، قفل به ریسه اول داده می‌شود. در غیر این‌صورت اولویت ریسه دوم که صاحب قفل است با ترد اول مقایسه می‌شود. اگر اولویت ترد اول بالاتر بود مجدد قفل به ترد اول داده می‌شود در غیر این‌صورت عملیات اهدای اولویت داریم. بدین صورت که مقدار `priority_after_donation` ریسه دوم برابر با این مقدار برای ریسه اول می‌شود و سپس قفل به ریسه اول داده می‌شود.

همچنین این مراحل برای ریسه دوم نیز بررسی می‌شود و شاید ریسه سومی وجود داشته باشد که اولویت بالاتری نسبت به ریسه دو داشته باشد که این عملیات مجدد برای این دو تکرار می‌شود. این عملیات حداکثر به اندازه `MAX_PRIORITY` تکرار می‌شود.

>> پرسش پنجم: مراحلی که هنگام صدا زدن `lock_release()` روی یک قفل که یک ریسه با اولویت بالا منتظر آن است، رخ می‌دهد را نام ببرید.

ابتدا همه وقفه‌ها غیرفعال می‌شوند. چون ریسه قفل را رها می‌کند، صاحب قبل را `NULL` قرار می‌دهیم. سپس `sema_up` فراخوانی می‌شود تا ریسه بعد قفل را بگیرد. اگر قفل تنها قفلی باشد که این ریسه دارد، مقدار `priority_after_donation` آن برابر می‌شود با مقدار پایه آن یا `base_priority`. در غیر این‌صورت برابر می‌شود اولویت اولین قفل (بالاترین اولویت).
در انتها نیز وقفه‌ها فعال می‌شوند.

### همگام‌سازی

>> پرسش ششم: یک شرایط احتمالی برای رخداد `race condition` در `thread_set_priority` را بیان کنید و توضیح دهید که چگونه پیاده‌سازی شما از رخداد آن جلوگیری می‌کند. آیا می‌توانید با استفاده از یک قفل از رخداد آن جلوگیری کنید؟

در پیاده‌سازی فعلی ما، چون وقفه‌ها قبل انجام عملیات خاموش می‌شوند، از رخداد چنین حالتی جلوگیری به‌عمل می‌آید.

### منطق

>> پرسش هفتم: چرا این طراحی را استفاده کردید؟ برتری طراحی فعلی خود را بر طراحی‌های دیگری که مدنظر داشته‌اید بیان کنید.

از نقاط قوت طراحی فعلی به ذخیره‌سازی به‌صورت مرتب اشاره کرد. در این‌صورت در بهترین زمان ممکن ریسه بعدی که باید اجرا شود انتخاب می‌شود.

## سوالات افزون بر طراحی

>> پرسش هشتم: در کلاس سه صفت مهم ریسه‌ها که سیستم عامل هنگامی که ریسه درحال اجرا نیست را ذخیره می‌کند، بررسی کردیم:‍‍ `program counter` ، ‍‍‍`stack pointer` و `registers`. بررسی کنید که این سه کجا و چگونه در `Pintos` ذخیره می‌شوند؟ مطالعه ‍`switch.S` و تابع ‍`schedule` در فایل `thread.c` می‌تواند مفید باشد.

In the ‍`schedule‍` function, a pointer to the thread that is currently running (`running_thread`) and a pointer to the next thread to be executed (`next_thread_to_run`) are defined. Then it is checked whether the next thread is the same as the current thread or not; If 
they are different, the `switch_threads` function is called, in which the registers are initially saved on the stack using

```assembly
pushl %ebx
pushl %ebp
pushl %esi
pushl %edi
```

These registers are called *callee-saved* registers and must be restored at the end.
The `switch_threads` function saves these registers on the current thread's stack using `pushl` commands and restores them for the next thread using `popl` instructions.

Saving the stack pointer:
The global variable `thread_stack_ofs` is defined in `thread.c` as follows:
```c
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
```
We use this variable to access the offset of the stack pointer field in the `struct thread` and then load it into the `edx` register as follows:
```8086
mov thread_stack_ofs, %edx
```
**Note** that `SWITCH_CUR+esp` is the address of the current thread struct and `SWITCH_next+esp` is the address of the next thread struct.

We have:
```8086
movl SWITCH_CUR(%esp), %eax
movl %esp, (%eax,%edx,1)
movl SWITCH_NEXT(%esp), %ecx
movl (%ecx,%edx,1), %esp
```

What happens in these few lines is that the current `esp` is first stored in the stack field related to the current thread struct, and then the `esp`, which is a pointer to the top of the stack, is set to the value stored in the stack field of the next thread struct.
Finally, the *callee-saved* registers are restored from the stack using
```
popl %edi
popl %esi
popl %ebp
popl %ebx
```

**Note** that the address of the previous thread is stored in the `%eax` register and the function's output value is also stored in `%eax`. That's why in the schedule function, we have:
```C
prev = switch_threads (cur, next);
```
Which means that `prev` points to the same previous thread.


>> پرسش نهم: وقتی یک ریسه‌ی هسته در ‍`Pintos` تابع `thread_exit` را صدا می‌زند، کجا و به چه ترتیبی صفحه شامل پشته و `TCB` یا `struct thread` آزاد می‌شود؟ چرا این حافظه را نمی‌توانیم به کمک صدازدن تابع ‍`palloc_free_page` داخل تابع ‍`thread_exit` آزاد کنیم؟

In the `thread_exit` function, we have:
```C
list_remove (&thread_current()->allelem);
thread_current()->status = THREAD_DYING;
```
This means that the thread is removed from the list of all threads, and the status of that thread is changed to `THREAD_DYING`. Then, in the same `thread_exit` function, the `schedule` function is called, in which we have:

```C
if (cur != next)
prev = switch_threads (cur, next);
thread_schedule_tail (prev);
```

In this part, Firstly, it switches to the new thread with `switch_threads`, and then `thread_schedule_tail` is called on the previous thread whose work is finished.

In the `thread_schedule_tail` function, we have:
```C
/* If the thread we switched from is dying, destroy its struct
thread. This must happen late so that thread_exit() doesn't
pull out the rug from under itself. (We don't free
initial_thread because its memory was not obtained via
palloc().) */
if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread)
{
ASSERT (prev != cur);
palloc_free_page (prev);
}
```
This means that if the status of the thread is `THREAD_DYING` and it is the initial thread, it will be removed from **Kernel** memory. **Note** that in `thread_exit`, the status was changed to `THREAD_DYING`, so the space taken by this thread (the finished thread) is freed by `palloc_free_page` here. Also, note that the initial thread did not obtain its space through `palloc`, so it does not need to be freed. We could not free the space earlier in `thread_exit` because we needed the thread's structural information in the `schedule` function, and if we did this before calling the `schedule` function, it would be removed from **Kernel** memory.

>> پرسش دهم: زمانی که تابع ‍`thread_tick` توسط `timer interrupt handler` صدا زده می‌شود، در کدام پشته اجرا می‌شود؟

In PintOS, the ‍‍`timer_interrupt` function serves as the `timer interrupt handler` and is invoked when a timer interrupt occurs. As the system is handling an interrupt, it operates in kernel mode. When the thread_tick function is called by the timer interrupt handler, it executes on the **kernel stack** associated with the currently executing thread.

>> پرسش یازدهم: یک پیاده‌سازی کاملا کاربردی و درست این پروژه را در نظر بگیرید که فقط یک مشکل درون تابع ‍`sema_up()` دارد. با توجه به نیازمندی‌های پروژه سمافورها(و سایر متغیرهای به‌هنگام‌سازی) باید ریسه‌های با اولویت بالاتر را بر ریسه‌های با اولویت پایین‌تر ترجیح دهند. با این حال پیاده‌سازی ریسه‌های با اولویت بالاتر را براساس اولویت مبنا `Base Priority` به جای اولویت موثر ‍`Effective Priority` انتخاب می‌کند. اساسا اهدای اولویت زمانی که سمافور تصمیم می‌گیرد که کدام ریسه رفع مسدودیت شود، تاثیر داده نمی‌شود. تستی طراحی کنید که وجود این باگ را اثبات کند. تست‌های `Pintos` شامل کد معمولی در سطح هسته (مانند متغیرها، فراخوانی توابع، جملات شرطی و ...) هستند و می‌توانند متن چاپ کنند و می‌توانیم متن چاپ شده را با خروجی مورد انتظار مقایسه کنیم و اگر متفاوت بودند، وجود مشکل در پیاده‌سازی اثبات می‌شود. شما باید توضیحی درباره این که تست چگونه کار می‌کند، خروجی مورد انتظار و خروجی واقعی آن فراهم کنید.

Consider the pseudo-code below:
```C
Thread A (priority 1){
    acquire Lock_1
    create Thread B
    create Thread C
    create Thread D
    print("A")
    release Lock_1
}

Thread B (priority 2){
    acquire Lock_2
    acquire Lock_1
    print("B")
    release Lock_2
    release Lock_1
}

Thread C (priority 3){
    acquire Lock_1
    print("C")
    release Lock_1
}

Thread D (priority 4){
    acquire Lock_2
    print("D")
    release Lock_2
}
```
This pseudocode demonstrates a scenario where an incorrect implementation of `sema_up()` leads to an undesired order of thread execution. In the given pseudocode, there are four threads (A, B, C, D) with different base priorities (1, 2, 3, 4). Two locks (Lock_1 and Lock_2) are used in the code to synchronize access to shared resources.

Initially, Thread A acquires Lock_1 and creates Threads B, C, and D. At this point, both Thread B and Thread C are waiting to acquire Lock_1, while Thread D is waiting to acquire Lock_2.

Due to priority donation, Thread A's effective priority becomes 4 (the highest priority among the threads waiting for Lock_1), and Thread B's effective priority also becomes 4 (since it's waiting for both Lock_1 and Lock_2). The priority table at this point looks like this:

```C

Thread Name  Base Priority  Effective Priority
Thread A          1              4
Thread B          2              4
Thread C          3              3
Thread D          4              4

```

The correct behavior for `sema_up()` should be to unblock the thread with the highest effective priority. In this case, Thread B should be unblocked, as it has an effective priority of 4, which is higher than Thread C's effective priority of 3. The correct order of execution should be **`ABDC`**.

However, due to the bug in the `sema_up()` function, it unblocks the thread with the highest base priority instead of the highest effective priority. As a result, Thread C gets unblocked, which has a higher base priority of 3 compared to Thread B's base priority of 2. This leads to an incorrect order of execution: **`ACBD`**.

## سوالات نظرسنجی

پاسخ به این سوالات دلخواه است، اما به ما برای بهبود این درس در ادامه کمک خواهد کرد. نظرات خود را آزادانه به ما بگوئید—این سوالات فقط برای سنجش افکار شماست. ممکن است شما بخواهید ارزیابی خود از درس را به صورت ناشناس و در انتهای ترم بیان کنید.

>> به نظر شما، این تمرین گروهی، یا هر کدام از سه وظیفه آن، از نظر دشواری در چه سطحی بود؟ خیلی سخت یا خیلی آسان؟

>> چه مدت زمانی را صرف انجام این تمرین کردید؟ نسبتا زیاد یا خیلی کم؟

>> آیا بعد از کار بر روی یک بخش خاص از این تمرین (هر بخشی)، این احساس در شما به وجود آمد که اکنون یک دید بهتر نسبت به برخی جنبه‌های سیستم عامل دارید؟

>> آیا نکته یا راهنمایی خاصی وجود دارد که بهتر است ما آنها را به توضیحات این تمرین اضافه کنیم تا به دانشجویان ترم های آتی در حل مسائل کمک کند؟

>> متقابلا، آیا راهنمایی نادرستی که منجر به گمراهی شما شود وجود داشته است؟

>> آیا پیشنهادی در مورد دستیاران آموزشی درس، برای همکاری موثرتر با دانشجویان دارید؟

این پیشنهادات میتوانند هم برای تمرین‌های گروهی بعدی همین ترم و هم برای ترم‌های آینده باشد.

>> آیا حرف دیگری دارید؟
