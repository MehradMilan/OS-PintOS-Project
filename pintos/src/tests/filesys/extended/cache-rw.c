#include <syscall.h>
#include <random.h>
#include "tests/lib.h"
#include "tests/main.h"

#define BLOCK_COUNT  128
#define BLOCK_SIZE 512
#define FILE_SIZE 65536
#define ACCEPTABLE_ERROR  6

char buf[FILE_SIZE];

void create_and_write_file(const char* file_name);
void validate_cache_counts();

void
test_main (void)
{
  create_and_write_file("a");
  validate_cache_counts();
  CHECK (remove ("a"), "Removed \"a\".");
}

void create_and_write_file(const char* file_name) {
  CHECK (create (file_name, FILE_SIZE), "create \"%s\" should not fail.", file_name);
  msg ("File creation completed.");
  spoil_cache ();
  int fd = open(file_name);
  CHECK (fd > 2, "open \"%s\" returned fd which should be greater than 2.", file_name);
  random_init (0);
  random_bytes (buf, sizeof buf);
  buf[FILE_SIZE - 1] = '\0';
  int write_bytes = write(fd, buf, FILE_SIZE);
  if (write_bytes != FILE_SIZE)
    fail ("write returned a value other than file size %d. (%d)", FILE_SIZE, write_bytes);
  close (fd);
  msg ("write completed.");
}

void validate_cache_counts() {
  size_t max_write = BLOCK_COUNT + ACCEPTABLE_ERROR;
  CHECK (cache_write () < max_write,
         "Block write count should be less than %d.", max_write);
  CHECK (cache_read () < ACCEPTABLE_ERROR,
         "Block read count should be less than acceptable error %d.", ACCEPTABLE_ERROR);
}