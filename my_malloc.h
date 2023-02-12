#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct chunk_meta {
  size_t size;               //the size of this chunk
  int free;                  //it is free or not
  struct chunk_meta * next;  //double linkedlist to next chunk
  struct chunk_meta * prev;  //double linkedlist to previous chunk
};
typedef struct chunk_meta chunk;

//Thread Safe malloc/free: locking version
void * ts_malloc_lock(size_t size);
void ts_free_lock(void * ptr);

//Thread Safe malloc/free: non-locking version
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);

void printChunk(chunk * chk);

void * allocate_space(size_t size);
void * reuse_chunk(chunk * ptr,
                   size_t size,
                   chunk ** free_region_Head,
                   chunk ** free_region_Tail);
void * bf_find_free_chunk(size_t size,
                          chunk ** free_region_Head,
                          chunk ** free_region_Tail);
void extend_chunk(chunk * ptr, chunk ** free_region_Head, chunk ** free_region_Tail);
void remove_chunk(chunk * ptr, chunk ** free_region_Head, chunk ** free_region_Tail);
chunk * split_chunk(size_t size, chunk * chk);
void mergeRight(chunk * chk, chunk ** free_region_Head, chunk ** free_region_Tail);
void mergeLeft(chunk * chk, chunk ** free_region_Head, chunk ** free_region_Tail);

//Best Fit malloc/free
void * bf_malloc(size_t size, chunk ** free_region_Head, chunk ** free_region_Tail);
void bf_free(void * ptr, chunk ** free_region_Head, chunk ** free_region_Tail);

unsigned long get_data_segment_size();
unsigned long get_data_segment_free_space_size();
