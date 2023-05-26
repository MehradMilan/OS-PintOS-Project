//
// Created by noroo on 5/22/2023.
//

#include <debug.h>
#include <string.h>
#include "cache.h"
#include "filesys/filesys.h"

struct cache_status
{
  size_t hits;
  size_t misses;
  size_t writes;
  size_t reads;
};

static struct cache_status cache_stat;


void
cache_init (void)
{
  lock_init(&cache_list_lock);
  list_init(&cache_LRU);

  for (int i=0; i < CACHE_SIZE; i++)
  {
    lock_init(&cache[i].cache_lock);
    cache[i].dirty = false;
    cache[i].valid = false;
    list_push_back (&cache_LRU, &(cache[i].cache_elem));
  }
  cache_stat = (struct cache_status) {0, 0, 0, 0};
}

void
flush_block (struct block *fs_device, struct cache_block *LRU_block)
{
  ++cache_stat.writes;
  block_write(fs_device, LRU_block->sector_num, LRU_block->data);
  LRU_block->dirty = false;
}



struct cache_block* handle_cache_miss(struct block *fs_device, block_sector_t sector, bool rt) {
    struct cache_block* LRU_block;

    ++cache_stat.misses;
    LRU_block = list_entry(list_pop_front(&cache_LRU), struct cache_block, cache_elem);
    lock_acquire(&LRU_block->cache_lock);

    if (LRU_block->valid && LRU_block->dirty) {
        flush_block(fs_device, LRU_block);
    }

    if (rt) {
        ++cache_stat.reads;
        block_read(fs_device, sector, LRU_block->data);
    }

    update_LRU_block_info(LRU_block, sector);

    lock_release(&LRU_block->cache_lock);

    return LRU_block;
}

void update_LRU_block_info(struct cache_block* LRU_block, block_sector_t sector) {
    LRU_block->sector_num = sector;
    LRU_block->valid = true;
    LRU_block->dirty = false;
    list_push_back(&cache_LRU, &LRU_block->cache_elem);
}

struct cache_block* handle_cache_hit(int index) {
    cache_stat.hits++;
    list_remove(&cache[index].cache_elem);
    list_push_back(&cache_LRU, &cache[index].cache_elem);
    return &cache[index];
}

int get_cache_index(block_sector_t sector) {
    for (int i=0; i < CACHE_SIZE; i++) {
        struct cache_block current_block = cache[i];
        if (current_block.valid && current_block.sector_num == sector) {
            return i;
        }
    }
    return -1;
}

struct cache_block* get_cache_block (struct block *fs_device, block_sector_t sector, bool rt) {
    lock_acquire (&cache_list_lock);

    int index = get_cache_index(sector);
    struct cache_block* target_block;

    if (index == -1) {
        target_block = handle_cache_miss(fs_device, sector, rt);
    } else {
        target_block = handle_cache_hit(index);
    }

    lock_release (&cache_list_lock);
    return target_block;
}




enum operation_type { READ, WRITE };

void cache_operation(enum operation_type op, struct block *fs_device, block_sector_t sector_idx, void *buffer, off_t offset, int chunk_size) 
{
    ASSERT (fs_device != NULL);
    ASSERT (offset >= 0 && chunk_size >= 0 && (offset + chunk_size) <= BLOCK_SECTOR_SIZE);

    bool read_flag = op == READ || (op == WRITE && !(offset == 0 && chunk_size >= BLOCK_SECTOR_SIZE));
    struct cache_block* cb = get_cache_block(fs_device, sector_idx, read_flag);
    lock_acquire(&cb->cache_lock);

    if (op == READ) 
    {
        memcpy(buffer, &(cb->data[offset]), chunk_size);
    }
    else if (op == WRITE) 
    {
        memcpy(&(cb->data[offset]), buffer, chunk_size);
        cb->dirty = true;
    }

    lock_release(&cb->cache_lock);
}

void cache_read(struct block *fs_device, block_sector_t sector_idx, void *buffer, off_t offset, int chunk_size)
{
    cache_operation(READ, fs_device, sector_idx, buffer, offset, chunk_size);
}

void cache_write(struct block *fs_device, block_sector_t sector_idx, void *buffer, off_t offset, int chunk_size)
{
    cache_operation(WRITE, fs_device, sector_idx, buffer, offset, chunk_size);
}


void
cache_shutdown (struct block *fs_device)
{
  for (int i=0; i<CACHE_SIZE; i++)
    {
      if (cache[i].valid == true && cache[i].dirty == true)
        flush_block (fs_device, &cache[i]);
    }

}

void spoil_block(struct cache_block *b) {
    b->valid = false;
    b->dirty = false;
}

void cache_spoil (struct cache_block *fs_device) {
    cache_shutdown (fs_device);
    for (int i = 0; i < CACHE_SIZE; i++)
    {
    cache[i].valid = false;
    cache[i].dirty = false;
    }
    cache_stat = (struct cache_status) {0, 0, 0, 0};
}

size_t get_cache_hits (void) {
  return cache_stat.hits;
}

size_t get_cache_misses (void) {
  return cache_stat.misses;
}

size_t get_cache_reads (void) {
  return cache_stat.reads;
}

size_t get_cache_writes (void) {
  return cache_stat.writes;
}
