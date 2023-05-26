#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DIRECT_BLOCK_NO 123
#define INDIRECT_BLOCK_NO 128
#define UNALLOCATED 0

struct inode_disk *get_inode_disk (const struct inode *);
static bool inode_disk_allocate (struct inode_disk *disk_inode, off_t length);
static bool allocate_sector (block_sector_t *sector_idx);
static bool allocate_indirect(block_sector_t *sector_idx, size_t num_sectors_to_allocate);
static bool inode_disk_deallocate (struct inode *inode);
static bool deallocate_indirect (block_sector_t sector_num, size_t num_sectors_to_allocate);

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
{
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    bool is_dir;                        /* Refer to task 3. */

    block_sector_t direct[DIRECT_BLOCK_NO];         /* Direct blocks of inode. */
    block_sector_t indirect;            /* Indirect blocks of inode. */
    block_sector_t double_indirect;     /* Double indirect blocks of indoe. */
};

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
{
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct lock f_lock;                 /* Synchronization between users of inode. */
};

struct inode_disk *
get_inode_disk (const struct inode *inode)
{
  struct inode_disk *id = malloc ( sizeof *id);
  cache_read(fs_device, inode->sector, (void *)id, 0, BLOCK_SECTOR_SIZE);
  return id;
}

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t get_direct_block(const struct inode_disk *id, off_t block_idx) {
  return id->direct[block_idx];
}

static block_sector_t get_indirect_block(const struct inode_disk *id, off_t block_idx) {
  block_sector_t indirect_blocks[INDIRECT_BLOCK_NO];
  cache_read(fs_device, id->indirect, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  return indirect_blocks[block_idx];
}

static block_sector_t get_double_indirect_block(const struct inode_disk *id, off_t block_idx) {
  block_sector_t double_indirect_blocks[INDIRECT_BLOCK_NO];
  cache_read(fs_device, id->double_indirect, &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  off_t indirect_block_idx = (block_idx - DIRECT_BLOCK_NO - INDIRECT_BLOCK_NO) / INDIRECT_BLOCK_NO;
  off_t idx = (block_idx - DIRECT_BLOCK_NO - INDIRECT_BLOCK_NO) % INDIRECT_BLOCK_NO;
  cache_read(fs_device, double_indirect_blocks[indirect_block_idx], &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  return double_indirect_blocks[idx];
}

static block_sector_t byte_to_sector(const struct inode *inode, off_t pos) {
  ASSERT(inode != NULL);

  struct inode_disk *id = get_inode_disk(inode);
  block_sector_t res = -1;

  if (pos >= id->length) {
    free(id);
    return res;
  }

  off_t block_idx = pos / BLOCK_SECTOR_SIZE;

  if (pos < DIRECT_BLOCK_NO * BLOCK_SECTOR_SIZE) {
    res = get_direct_block(id, block_idx);
  } else if (pos < (DIRECT_BLOCK_NO + INDIRECT_BLOCK_NO) * BLOCK_SECTOR_SIZE) {
    block_idx -= DIRECT_BLOCK_NO;
    res = get_indirect_block(id, block_idx);
  } else {
    res = get_double_indirect_block(id, block_idx);
  }

  free(id);
  return res;
}


/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool inode_create(block_sector_t sector, off_t length, bool is_dir) {
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT(length >= 0);

  /* If this assertion fails, the inode structure is not exactly one sector in size, and you should fix that. */
  ASSERT(sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc(1, sizeof *disk_inode);
  if (disk_inode == NULL) {
    return false;
  }

  disk_inode->is_dir = is_dir;
  disk_inode->length = length;
  disk_inode->magic = INODE_MAGIC;

  if (inode_disk_allocate(disk_inode, length)) {
    cache_write(fs_device, sector, disk_inode, 0, BLOCK_SECTOR_SIZE);
    success = true;
  }

  free(disk_inode);
  return success;
}

static bool
inode_disk_allocate (struct inode_disk *inode_disk_info, off_t data_length)
{
  if (data_length < 0)
    return false;

  size_t sectors_required = bytes_to_sectors (data_length);
  size_t sector_idx, chunk_size;

  for (sector_idx = 0; sector_idx < sectors_required && sector_idx < DIRECT_BLOCK_NO; sector_idx++) {
    if (!inode_disk_info->direct[sector_idx] && !allocate_sector (&inode_disk_info->direct[sector_idx])) {
      return false;
    }
  }

  sectors_required -= sector_idx;

  if (sectors_required == 0) return true;
  
  if (!inode_disk_info->indirect && !allocate_sector (&inode_disk_info->indirect)) return false;

  if (!allocate_indirect(inode_disk_info->indirect, sectors_required)) return false;
  
  if (sectors_required > INDIRECT_BLOCK_NO) {
    sectors_required -= INDIRECT_BLOCK_NO;
  } else {
    return true;
  }

  if (sectors_required > INDIRECT_BLOCK_NO * INDIRECT_BLOCK_NO) {
    sectors_required = INDIRECT_BLOCK_NO * INDIRECT_BLOCK_NO;
  }

  if (inode_disk_info->double_indirect == UNALLOCATED && !allocate_sector(&inode_disk_info->double_indirect)) return false;

  block_sector_t cached_blocks[INDIRECT_BLOCK_NO];
  cache_read (fs_device, inode_disk_info->double_indirect, &cached_blocks, 0, BLOCK_SECTOR_SIZE);

  size_t sectors_for_indirect = DIV_ROUND_UP (sectors_required, INDIRECT_BLOCK_NO);
  for (sector_idx = 0; sector_idx < sectors_for_indirect; sector_idx++) {
    chunk_size = sectors_required < INDIRECT_BLOCK_NO ? sectors_required : INDIRECT_BLOCK_NO;

    if (cached_blocks[sector_idx] == UNALLOCATED && !allocate_sector (&cached_blocks[sector_idx])) return false;

    if (!allocate_indirect (cached_blocks[sector_idx], chunk_size)) return false;

    sectors_required -= chunk_size;
  }

  cache_write (fs_device, inode_disk_info->double_indirect, &cached_blocks, 0, BLOCK_SECTOR_SIZE);

  ASSERT (sectors_required == 0);
  
  return sectors_required <= INDIRECT_BLOCK_NO * INDIRECT_BLOCK_NO;
}



bool
allocate_indirect(block_sector_t *sector_idx, size_t num_sectors_to_allocate)
{
  block_sector_t indirect_blocks[INDIRECT_BLOCK_NO];
  cache_read (fs_device, sector_idx, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);

  size_t i = 0;
  while(i < num_sectors_to_allocate && i < INDIRECT_BLOCK_NO)
  {
    if (indirect_blocks[i] == 0)
    {
      if (!allocate_sector (&indirect_blocks[i]))
      {
        return false;
      }
    }
    i++;
  }

  cache_write (fs_device, sector_idx, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  return true;
}


static bool
allocate_sector (block_sector_t *sector_idx)
{
  static char new_buffer[BLOCK_SECTOR_SIZE];
  if (free_map_allocate(1, sector_idx))
    {
      cache_write (fs_device, *sector_idx, new_buffer, 0, BLOCK_SECTOR_SIZE);
      return true;
    }
  return false;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init (&inode->f_lock);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          inode_disk_deallocate(inode);
        }

      free (inode);
    }
}
/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t inode_read_at(struct inode *inode, void *buffer_, off_t size, off_t offset) {
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  lock_acquire(&inode->f_lock);

  /* Disk sector to read, starting byte offset within sector. */
  block_sector_t sector_idx = byte_to_sector(inode, offset);
  int sector_ofs = offset % BLOCK_SECTOR_SIZE;

  /* Bytes left in inode, bytes left in sector, lesser of the two. */
  off_t inode_left = inode_length(inode) - offset;
  int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
  int min_left = inode_left < sector_left ? inode_left : sector_left;

  while (size > 0 && min_left > 0) {
    /* Number of bytes to actually copy out of this sector. */
    int chunk_size = size < min_left ? size : min_left;

    cache_read(fs_device, sector_idx, (void *)(buffer + bytes_read), sector_ofs, chunk_size);

    /* Advance. */
    size -= chunk_size;
    offset += chunk_size;
    bytes_read += chunk_size;

    /* Update variables for the next iteration. */
    sector_idx = byte_to_sector(inode, offset);
    sector_ofs = offset % BLOCK_SECTOR_SIZE;
    inode_left = inode_length(inode) - offset;
    sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
    min_left = inode_left < sector_left ? inode_left : sector_left;
  }

  lock_release(&inode->f_lock);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t inode_write_at(struct inode *inode, const void *buffer_, off_t size, off_t offset) {
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  if (inode->deny_write_cnt)
    return 0;

  lock_acquire(&inode->f_lock);

  if (byte_to_sector(inode, offset + size - 1) == (size_t)-1) {
    struct inode_disk *id = get_inode_disk(inode);
    if (!inode_disk_allocate(id, offset + size)) {
      free(id);
      lock_release(&inode->f_lock);
      return bytes_written;
    }

    id->length = size + offset;
    cache_write(fs_device, inode_get_inumber(inode), (void *)id, 0, BLOCK_SECTOR_SIZE);
    free(id);
  }

  /* Sector to write, starting byte offset within sector. */
  block_sector_t sector_idx = byte_to_sector(inode, offset);
  int sector_ofs = offset % BLOCK_SECTOR_SIZE;

  /* Bytes left in inode, bytes left in sector, lesser of the two. */
  off_t inode_left = inode_length(inode) - offset;
  int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
  int min_left = inode_left < sector_left ? inode_left : sector_left;

  while (size > 0 && min_left > 0) {
    /* Number of bytes to actually write into this sector. */
    int chunk_size = size < min_left ? size : min_left;
    if (chunk_size <= 0)
      break;

    cache_write(fs_device, sector_idx, (void *)(buffer + bytes_written), sector_ofs, chunk_size);

    /* Advance. */
    size -= chunk_size;
    offset += chunk_size;
    bytes_written += chunk_size;

    /* Update variables for the next iteration. */
    sector_idx = byte_to_sector(inode, offset);
    sector_ofs = offset % BLOCK_SECTOR_SIZE;
    inode_left = inode_length(inode) - offset;
    sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
    min_left = inode_left < sector_left ? inode_left : sector_left;
  }

  lock_release(&inode->f_lock);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  struct inode_disk *id = get_inode_disk(inode);
  off_t length = id->length;
  free(id);
  return length;
}

static void free_direct_blocks(struct inode_disk *disk_inode, size_t *num_sectors_to_deallocate) {
  size_t i;

  for (i = 0; i < *num_sectors_to_deallocate && i < DIRECT_BLOCK_NO; i++)
    free_map_release(disk_inode->direct[i], 1);

  *num_sectors_to_deallocate -= i;
}

static bool free_indirect_blocks(struct inode_disk *disk_inode, size_t *num_sectors_to_deallocate) {
  if (*num_sectors_to_deallocate == 0)
    return true;

  if (!deallocate_indirect(disk_inode->indirect, *num_sectors_to_deallocate))
    return false;

  size_t i = *num_sectors_to_deallocate < INDIRECT_BLOCK_NO ? *num_sectors_to_deallocate : INDIRECT_BLOCK_NO;
  *num_sectors_to_deallocate -= i;

  return true;
}

static void free_double_indirect_blocks(struct inode_disk *disk_inode, size_t *num_sectors_to_deallocate, block_sector_t *blocks) {
  size_t num_double_indirect_blocks = DIV_ROUND_UP(*num_sectors_to_deallocate, INDIRECT_BLOCK_NO);
  size_t i;

  for (i = 0; i < num_double_indirect_blocks; i++) {
    block_sector_t double_indirect_blocks[INDIRECT_BLOCK_NO];
    cache_read(fs_device, blocks[i], &double_indirect_blocks, 0, BLOCK_SECTOR_SIZE);

    size_t j;
    for (j = 0; j < *num_sectors_to_deallocate && j < INDIRECT_BLOCK_NO; j++)
      free_map_release(double_indirect_blocks[j], 1);

    free_map_release(blocks[i], 1);
    *num_sectors_to_deallocate -= j;
  }

  free_map_release(disk_inode->double_indirect, 1);
}

static bool inode_disk_deallocate(struct inode *inode) {
  struct inode_disk *disk_inode = get_inode_disk(inode);
  if (disk_inode == NULL)
    return true;

  size_t num_sectors_to_deallocate = bytes_to_sectors(disk_inode->length);

  free_direct_blocks(disk_inode, &num_sectors_to_deallocate);
  if (num_sectors_to_deallocate == 0) {
    free(disk_inode);
    return true;
  }

  if (!free_indirect_blocks(disk_inode, &num_sectors_to_deallocate)) {
    free(disk_inode);
    return false;
  }

  block_sector_t blocks[INDIRECT_BLOCK_NO];
  cache_read(fs_device, disk_inode->double_indirect, &blocks, 0, BLOCK_SECTOR_SIZE);
  free_double_indirect_blocks(disk_inode, &num_sectors_to_deallocate, blocks);

  free(disk_inode);
  return true;
}

bool
inode_disk_isdir (const struct inode_disk *disk_inode)
{
  return disk_inode->is_dir;
}

bool
is_directory_inode(const struct inode *inode)
{
  if (inode == NULL)
    return false;

  struct inode_disk *disk_inode = get_inode_disk(inode);
  if (disk_inode == NULL)
    return false;

  bool result = disk_inode->is_dir;
  free(disk_inode);

  return result;
}

bool
inode_is_removed (const struct inode *inode)
{
  return inode->removed;
}

static bool deallocate_indirect (block_sector_t sector_num, size_t num_sectors_to_allocate)
{
  block_sector_t indirect_blocks[INDIRECT_BLOCK_NO];
  cache_read (fs_device, sector_num, &indirect_blocks, 0, BLOCK_SECTOR_SIZE);
  for (size_t i = 0; i < num_sectors_to_allocate && i < INDIRECT_BLOCK_NO; i++)
    free_map_release (indirect_blocks[i], 1);
  free_map_release (sector_num, 1);
  return true;
}
