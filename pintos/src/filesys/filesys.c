#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/cache.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  cache_init();
  free_map_init ();

  if (format)
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
  cache_shutdown (fs_device);
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *path, off_t initial_size, bool is_dir)
{
  block_sector_t inode_sector = 0;
  struct dir *parent;
  char tail[NAME_MAX + 1];

  bool success = (dir_split_new (&parent, tail, path)
                  && tail[0] != '\0'
                  && parent != NULL
                  && free_map_allocate (1, &inode_sector)
                  && (is_dir
                      ?dir_create (inode_sector, 16)
                      :inode_create (inode_sector, initial_size, false))
                  && dir_add (parent, tail, inode_sector, true));
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (parent);

  return success;
}

/* Opens the file with the given NAME.
   Puts the file/directory in the given `descriptor`.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{

  if (strcmp(name, "/") == 0)
  {
    return (struct file *) dir_open_root ();
  }
  if (strcmp(name, "") == 0) {
    return NULL;
  }

  char file_name[NAME_MAX + 1];
  char directory[strlen(name) + 1];
  file_name[0] = '\0';
  directory[0] = '\0';

  struct dir *dir = dir_open_path (directory);
  struct inode *inode = NULL;
  if (dir != NULL && split_path (name, directory, file_name)){

  if (strlen (file_name) == 0)
    inode = dir_get_inode (dir);
  else
    {dir_lookup (dir, file_name, &inode);
      dir_close (dir);
    }

  if (inode == NULL || inode_is_removed (inode))
    return NULL;

  return file_open (inode);

  } else return NULL;

}


/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  struct dir *dir = dir_open_root ();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir);

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}
