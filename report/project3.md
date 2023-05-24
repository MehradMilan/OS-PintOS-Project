<div dir="rtl" style="text-align: justify;">

# گزارش تمرین گروهی ۳

گروه ۴
-----

Mehrad Milanloo <mehrad_milanloo@yahoo.com>

Parnian Razavipour <razaviparnian81@yahoo.com>

Alireza Noroozi <noroozi.alirezaa@gmail.com> 

Hossein Alihosseini <alihosseini@sharif.edu> 


بافر کش
====================
تغییری در این بخش نداشتیم و طبق داک طراحی، با الگوریتم `LRU` اقدام به پیاده‌سازی `cache` کردیم.

همچنین در پرونده `cache.c` به‌دلیل وجود تست‌های `persistance` که ابتدا سیستم را خاموش و سپس تست‌ها را ران می‌کرد، از تابع `flush_block` استفاده کردیم که تغییرات را در دیسک ذخیره می‌کرد.

پرونده‌های توسعه پذیر
====================
الگوریتم‌ها و داده‌ساختارهای ذکرشده در داک طراحی پیاده‌سازی شده و تغییری در این مورد نداشتیم.

پوشه
====
داده‌ساختارهای `inode` و `dir` که در سند طراحی نیز بود، در پیاده‌سازی این بخش به ما کمک کرد.

```C
struct dir
  {
    ...
	struct dir *parent_dir;
	int use_cnt;
	struct lock *lock;
	...
  };
```

```c
// filesys/inode.c

struct inode_disk
  {
	...
    bool is_dir;                        /* Refer to task 3. */

    block_sector_t direct[123];         /* Direct blocks of inode. */
    block_sector_t indirect;            /* Indirect blocks of inode. */
    block_sector_t double_indirect;     /* Double indirect blocks of indoe. */
  };

struct inode
  {
    ...
    struct lock f_lock;                 /* Synchronization between users of inode. */
  };

```

تست‌های افزون بر طراحی
====================

۲ تست اضافه‌شده شامل تست‌های عملکرد `cache` و عملیات `read` و `write` رو کش بودند.

در تست عملکرد، بررسی نسبت `hit/miss` بود که باید بیشتر می‌شد. در تست `write` نیز هدف بررسی حافظه نهان قبل از دسترسی به دیسک بود.


در تست عملکرد ابتدا داده را به‌صورت فایل درآورده و آن‌را می‌خوانیم. سپس در عملیات خواندن دوم باید سرعت و همچنین نسبت ذکرشده بیشتر شود.

در تست `write` نیز یک فایل با حجمی بزرگتر از کش را می‌خوانیم و تعداد `read` و `write` را بررسی می‌کنیم.

تست‌ها هرکدام با نام‌های `cache-performance` و `cache-rw` در کدبیس موجود هستند.

مسئولیت هر فرد
===============
تسک‌ها را به ۳ بخش تقسیم کردیم:
تسک مربوط به `cache` توسط علیرضا، تسک مربوط به `syscall‍` توسط پرنیان، تسک مربوط به `directory` توسط حسین و مهراد تکمیل شد. درنهایت هر فرد باگ‌های مربوط به بخش خود را دیباگ کرد.

</div>