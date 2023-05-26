#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"


/* A directory. */
struct dir {
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
};

/* A single directory entry. */
struct dir_entry {
    block_sector_t inode_sector;        /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool in_use;                        /* In use or free? */
};

static struct dir_entry
get_new_entry (block_sector_t sector, const char *name);

/* Creates a directory with space for ENTRY_CNT entries in the
   given SECTOR.  Returns true if successful, false on failure. */
bool dir_create(block_sector_t sector, size_t entry_cnt) {
  struct dir *dir_ptr;
  struct dir_entry entry;
  off_t bytes_written;

  if (!inode_create(sector, entry_cnt * sizeof(struct dir_entry), true)) {
    return false;
  }
    
  dir_ptr = dir_open(inode_open(sector));
  entry.in_use = false;
  entry.inode_sector = sector;
  bytes_written = inode_write_at(dir_get_inode(dir_ptr), &entry, sizeof(entry), 0);
  dir_close(dir_ptr);
  return bytes_written == sizeof(entry);
}


// bool
// dir_create (block_sector_t sector, size_t entry_cnt)
// {
//   // this should be checked
//   if (!inode_create (sector, (entry_cnt) * sizeof (struct dir_entry), true))
//     return false;
  
//   struct inode *dir_inode = inode_open (sector);
//   struct dir_entry parent_entry = get_new_entry (sector, "..");

//   bool success;
//   success = inode_write_at (dir_inode, &parent_entry, 
//             sizeof parent_entry, 0) == sizeof parent_entry;


//   inode_close (dir_inode);
//   return success;
// }


static struct dir_entry
get_new_entry (block_sector_t sector, const char *name)
{
  struct dir_entry e;
  e.inode_sector = sector;
  e.in_use = true;
  strlcpy (e.name, name, sizeof e.name);
  return e;
}

/* Check whether the directory is empty or not */
static bool check_directory(struct dir *dir) {
  struct dir_entry e;
  off_t ofs;

  for (ofs = 1 * sizeof(e); inode_read_at(dir->inode, &e, sizeof(e), ofs) == sizeof(e);
       ofs += sizeof(e)) {
    if (e.in_use) {
      return false;
    }
  }
  return true;
}


/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
struct dir *dir_open(struct inode *inode) {
  if (inode == NULL) {
    return NULL;
  }
  
  struct dir *dir = calloc(1, sizeof *dir);
  if (dir == NULL) {
    inode_close(inode);
    return NULL;
  }
  
  dir->inode = inode;
  dir->pos = sizeof(struct dir_entry);
  return dir;
}


/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
struct dir *
dir_open_root (void)
{
  return dir_open (inode_open (ROOT_DIR_SECTOR));
}
/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
struct dir *
dir_reopen (struct dir *dir)
{
  return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
void
dir_close (struct dir *dir)
{
  if (dir != NULL)
    {
      inode_close (dir->inode);
      free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
struct inode *
dir_get_inode (struct dir *dir)
{
  return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
static bool lookup(const struct dir *dir, const char *name, struct dir_entry *ep, off_t *ofsp) {
  struct dir_entry e;
  size_t ofs;

  ASSERT(dir != NULL);
  ASSERT(name != NULL);

  ofs = sizeof(e);
  while (inode_read_at(dir->inode, &e, sizeof(e), ofs) == sizeof(e)) {
    if (e.in_use && !strcmp(name, e.name)) {
      if (ep != NULL) {
        *ep = e;
      }
      if (ofsp != NULL) {
        *ofsp = ofs;
      }
      return true;
    }
    ofs += sizeof(e);
  }
  
  return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool dir_lookup(const struct dir *dir, const char *name, struct inode **inode) {
  struct dir_entry e;

  ASSERT(dir != NULL);
  ASSERT(name != NULL);

  if (strcmp(name, "..") == 0) {
    inode_read_at(dir->inode, &e, sizeof(e), 0);
    *inode = inode_open(e.inode_sector);
    return *inode != NULL;
  }

  if (strcmp(name, ".") == 0) {
    *inode = inode_reopen(dir->inode);
    return *inode != NULL;
  }

  if (lookup(dir, name, &e, NULL)) {
    *inode = inode_open(e.inode_sector);
    return *inode != NULL;
  }

  *inode = NULL;
  return false;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
bool dir_add(struct dir *dir, const char *name, block_sector_t inode_sector) {
  struct dir_entry e;
  off_t ofs;

  ASSERT(dir != NULL);
  ASSERT(name != NULL);

  /* Check NAME for validity. */
  if (*name == '\0' || strlen(name) > NAME_MAX) {
    return false;
  }

  /* Check that NAME is not in use. */
  if (lookup(dir, name, NULL, NULL)) {
    return false;
  }

  struct inode *dir_inode = inode_open(inode_sector);
  if (dir_inode == NULL) {
    return false;
  }

  if (inode_disk_isdir(get_inode_disk(dir_inode))) {
    struct dir_entry child_entry;
    child_entry = get_new_entry(inode_get_inumber(dir_get_inode(dir)), "..");
    /* Error occurred while writing at the child directory */
    if (inode_write_at(dir_inode, &child_entry, sizeof(child_entry), 0) != sizeof(child_entry)) {
      inode_close(dir_inode);
      return false;
    }
  }
  inode_close(dir_inode);

  ofs = sizeof(e);
  while (inode_read_at(dir->inode, &e, sizeof(e), ofs) == sizeof(e)) {
    if (!e.in_use) {
      break;
    }
    ofs += sizeof(e);
  }

  e = get_new_entry(inode_sector, name);
  return inode_write_at(dir->inode, &e, sizeof(e), ofs) == sizeof(e);
}

/* Removes any entry for NAME in DIR.
   Returns true if successful, false on failure,
   which occurs only if there is no file with the given NAME. */
bool dir_remove(struct dir *dir, const char *name) {
  struct dir_entry e;
  struct inode *inode = NULL;
  bool success = false;
  off_t ofs;

  ASSERT(dir != NULL);
  ASSERT(name != NULL);

  /* Find directory entry. */
  if (!lookup(dir, name, &e, &ofs)) {
    return false;
  }

  /* Open inode. */
  inode = inode_open(e.inode_sector);
  if (inode == NULL) {
    return false;
  }

  if (is_directory_inode(inode)) {
    struct dir *t_dir = dir_open(inode);
    bool empty = check_directory(t_dir);
    dir_close(t_dir);

    if (!empty) {
      inode_close(inode);
      return false;
    }
  }

  /* Erase directory entry. */
  e.in_use = false;
  if (inode_write_at(dir->inode, &e, sizeof(e), ofs) != sizeof(e)) {
    inode_close(inode);
    return false;
  }

  /* Remove inode. */
  inode_remove(inode);
  inode_close(inode);

  return true;
}

/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
bool dir_readdir(struct dir *dir, char name[NAME_MAX + 1]) {
  struct dir_entry e;
  bool entry_found = false;

  while (inode_read_at(dir->inode, &e, sizeof(e), dir->pos) == sizeof(e)) {
    dir->pos += sizeof(e);
    if (e.in_use) {
      strlcpy(name, e.name, NAME_MAX + 1);
      entry_found = true;
      break;
    }
  }

  if (!entry_found) {
    dir->pos = sizeof(struct dir_entry);
    return false;
  }

  return true;
}

struct dir *initialize_directory(const char *path) {
  struct thread *curr_thread = thread_current();
  struct dir *curr_dir = NULL;

  if (path[0] == '/') {
    curr_dir = dir_open_root();
  } else if (curr_thread->working_dir != NULL) {
    curr_dir = dir_reopen(curr_thread->working_dir);
  }

  return curr_dir;
}


struct dir *traverse_path(struct dir *curr_dir, char const_path[]) {
  char *dir_token, *save_ptr;

  dir_token = strtok_r(const_path, "/", &save_ptr);
  while (dir_token != NULL) {
    if (strlen(dir_token) > NAME_MAX) {
      break;
    }

    struct inode *next_inode;
    if (!dir_lookup(curr_dir, dir_token, &next_inode)) {
      break;
    }

    dir_close(curr_dir);
    curr_dir = dir_open(next_inode);
    if (curr_dir == NULL) {
      break;
    }

    dir_token = strtok_r(NULL, "/", &save_ptr);
  }

  if (dir_token != NULL || curr_dir == NULL) {
    dir_close(curr_dir);
    curr_dir = NULL;
  }

  return curr_dir;
}

static int get_next_part(char part[NAME_MAX + 1], const char **srcp) {
  const char *src = *srcp;
  char *dst = part;

  // Skip leading '/'
  while (*src == '/') {
    src++;
  }

  if (*src == '\0') {
    return 0;
  }

  // Copy characters until '/' or '\0'
  while (*src != '/' && *src != '\0' && (dst - part) < NAME_MAX) {
    *dst++ = *src++;
  }

  *dst = '\0';
  *srcp = src;

  if (*src == '/' || *src == '\0') {
    return 1;
  } else {
    return -1;
  }
}
bool
initialize_parent(struct dir **parent, const char *path)
{
  if (path[0] == '/')
  {
    *parent = dir_open_root ();
  }
  else
  {
    if (thread_current()->working_dir == NULL)
      *parent = dir_open_root ();
    else
    {
      if (inode_is_removed(thread_current()->working_dir->inode))
        return false;
      *parent = dir_reopen(thread_current()->working_dir);
    }
  }
  return true;
}

bool
process_tail(struct dir **parent, char *tail, struct inode **next_inode, bool *failed_lookup)
{
  if (strlen(tail) > 0)
  {
    if (!dir_lookup(*parent, tail, next_inode))
      *failed_lookup = true;
  }
  return true;
}

bool
process_next_part(struct dir **parent, struct inode **next_inode, bool *failed_lookup)
{
  if (*failed_lookup)
    return false;
  if (*next_inode)
  {
    if (inode_is_removed(*next_inode))
      return false;

    struct dir *next_dir = dir_open(*next_inode);
    if (next_dir == NULL)
      return false;

    dir_close(*parent);
    *parent = next_dir;
  }
  return true;
}

bool
dir_split_new(struct dir **parent, char *tail, const char *path)
{
  const char *t_path = path;

  if (!initialize_parent(parent, path))
    return false;

  *tail = '\0';
  while (true)
  {
    struct inode *next_inode = NULL;
    bool failed_lookup = false;

    process_tail(parent, tail, &next_inode, &failed_lookup);

    int result = get_next_part(tail, &t_path);
    if (result < 0)
      goto failed;
    else if (result == 0)
    {
      inode_close(next_inode);
      break;
    }

    if (!process_next_part(parent, &next_inode, &failed_lookup))
      goto failed;
  }
  return true;

failed:
  *tail = '\0';
  *parent = NULL;
  dir_close(*parent);
  return false;
}

struct dir *
get_directory(const char *path)
{
  if (strcmp(path, "/") == 0)
  {
    return dir_open_root();
  }

  char tail[NAME_MAX + 1];
  struct dir *parent = NULL;
  if (!dir_split_new(&parent, tail, path))
  {
    return NULL;
  }
  return parent;
}


void 
dir_cleanup(struct dir *dir, struct inode *inode)
{
  if (inode != NULL) inode_close(inode);
  if (dir != NULL) dir_close(dir);
}

struct dir *dir_open_path(const char *path) {
  if (strcmp(path, "/") == 0) {
    return dir_open_root();
  }

  char tail[NAME_MAX + 1];
  struct dir *parent;
  bool success = dir_split_new(&parent, tail, path);

  if (!success) {
    return NULL;
  }

  struct inode *inode;
  if (!dir_lookup(parent, tail, &inode)) {
    dir_close(parent);
    return NULL;
  }

  dir_close(parent);

  if (inode_is_removed(inode)) {
    inode_close(inode);
    return NULL;
  }

  return dir_open(inode);
}



/* Reports the parent directory and the name of file/directory
   in `tail`. Note that `tail` must have allocated at least
   `NAME_MAX + 1` bytes of memory. `tail` will change to empty
   string in case of failure. */


struct dir* get_dir_of_file(struct file *file) {
  if (file != NULL) {
    struct inode *inode = file_get_inode(file);
    if (inode != NULL && is_directory_inode(inode)) {
      return (struct dir *) file;
    }
  }
  return NULL;
}
