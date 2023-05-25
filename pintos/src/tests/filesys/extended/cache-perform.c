#include <syscall.h>
#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"

#define FILE_SIZE  10240
#define BLOCK_SIZE 512

char buf[FILE_SIZE];

void create_and_write_file(const char* file_name);
void read_file_block_by_block(int fd);

void
test_main (void)
{
  create_and_write_file("a");
  invalidate_cache ();
  msg ("File creation completed.");

  int fd = open("a");
  read_file_block_by_block(fd);
  close(fd);

  uint32_t hit = cache_hit ();
  uint32_t miss = cache_miss ();

  msg ("First run cache hit rate calculated.");

  fd = open("a");
  read_file_block_by_block(fd);
  close(fd);

  uint32_t new_hit = cache_hit ();
  uint32_t new_miss = cache_miss ();

  msg ("Second run cumulative cache hit rate calculated.");
  CHECK (new_hit * (hit + miss) > hit * (new_hit + new_miss), "Hit rate should grow.");

  CHECK (remove ("a"), "Removed \"a\".");
}

void create_and_write_file(const char* file_name) {
  CHECK (create (file_name, FILE_SIZE), "create \"%s\" should not fail.", file_name);
  int fd = open(file_name);
  CHECK (fd > 2, "open \"%s\" returned fd which should be greater than 2.", file_name);
  random_init (0);
  random_bytes (buf, sizeof buf);
  buf[FILE_SIZE - 1] = '\0';
  CHECK (FILE_SIZE == write(fd, buf, FILE_SIZE), "write should write all the data to file.");
  close (fd);
}

void read_file_block_by_block(int fd) {
  for (size_t i = 0; i < FILE_SIZE / BLOCK_SIZE; i++)
  {
    int read_bytes = read (fd, buf, BLOCK_SIZE);
    if (read_bytes != BLOCK_SIZE)
      fail ("read should read a whole block, not %d.", read_bytes);
  }
}