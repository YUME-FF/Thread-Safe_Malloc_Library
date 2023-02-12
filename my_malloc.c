#include "my_malloc.h"

#include <assert.h>

#define META_SIZE sizeof(chunk)

chunk * free_region_Start = NULL;  //Start
chunk * free_region_End = NULL;    //End

__thread chunk * free_region_Start_nolk = NULL;  //Start
__thread chunk * free_region_End_nolk = NULL;    //End
size_t data_size = 0;
size_t free_size = 0;
pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
int lock = 0;

void * ts_malloc_lock(size_t size) {
  pthread_mutex_lock(&fastmutex);
  lock = 0;
  void * p = bf_malloc(size, &free_region_Start, &free_region_End);
  pthread_mutex_unlock(&fastmutex);
  return p;
}
void ts_free_lock(void * ptr) {
  pthread_mutex_lock(&fastmutex);
  bf_free(ptr, &free_region_Start, &free_region_End);
  pthread_mutex_unlock(&fastmutex);
}

void * ts_malloc_nolock(size_t size) {
  lock = 1;
  void * p = bf_malloc(size, &free_region_Start_nolk, &free_region_End_nolk);
  return p;
}
void ts_free_nolock(void * ptr) {
  bf_free(ptr, &free_region_Start_nolk, &free_region_End_nolk);
}

void printChunk(chunk * chk) {
  printf("curr: %p\n", chk);
  printf("size: %lu\n", chk->size);
  printf("is free: %d\n", chk->free);
  printf("next: %p\n", chk->next);
  printf("prev: %p\n", chk->prev);
}

/*
Function: Request for space with size of 'size'.

When there is no free block, 
space should be allocated from the OS (request for space) 
using sbrk and add new block to the end of the struct chunk.
*/
void * allocate_space(size_t size) {
  data_size += size + META_SIZE;
  chunk * new;
  void * request;
  if (lock) {
    pthread_mutex_lock(&fastmutex);
    new = sbrk(0);
    request = sbrk(size + META_SIZE);
    pthread_mutex_unlock(&fastmutex);
  }
  else {
    new = sbrk(0);
    request = sbrk(size + META_SIZE);
  }
  assert((void *)new == request);
  if (request == (void *)-1) {
    return NULL;  // sbrk failed.
  }
  new->size = size;
  new->free = 0;
  new->next = NULL;
  new->prev = NULL;
  return (char *)new + META_SIZE;
}

/*
Function: extend free memory region in chunk
*/
void extend_chunk(chunk * ptr, chunk ** free_region_Head, chunk ** free_region_Tail) {
  if (!(*free_region_Head) || (ptr < *free_region_Head)) {
    ptr->prev = NULL;
    ptr->next = *free_region_Head;
    if (ptr->next) {
      ptr->next->prev = ptr;
    }
    else {
      *free_region_Tail = ptr;
    }
    *free_region_Head = ptr;
  }
  else {
    chunk * curr = *free_region_Head;
    //find until ptr < curr->next
    while (curr->next && (ptr > curr->next)) {
      curr = curr->next;
    }

    //curr -> ptr -> curr->next
    ptr->prev = curr;
    ptr->next = curr->next;
    curr->next = ptr;

    if (ptr->next) {  //ptr is the last
      ptr->next->prev = ptr;
    }
    else {
      *free_region_Tail = ptr;
    }
  }
}

/*
Function: remove targeted chunk
*/

void remove_chunk(chunk * ptr, chunk ** free_region_Head, chunk ** free_region_Tail) {
  if (*free_region_Tail == ptr) {
    if (*free_region_Head == ptr) {
      *free_region_Head = NULL;
      *free_region_Tail = NULL;
    }
    else {
      *free_region_Tail = ptr->prev;
      (*free_region_Tail)->next = NULL;
    }
  }
  else if (*free_region_Head == ptr) {
    *free_region_Head = ptr->next;
    (*free_region_Head)->prev = NULL;
  }
  else {
    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;
  }
}

/*
Function: split the chunk.
One of First fit's short is that it may make a small size to
be set in a big chunk, thus when the remain chunk is large
enough, the chunk should be splitted.
*/
chunk * split_chunk(size_t size, chunk * chk) {
  chunk * splitChunk;
  splitChunk = (chunk *)((char *)chk + META_SIZE + size);
  splitChunk->size = chk->size - size - META_SIZE;
  splitChunk->free = 1;
  splitChunk->next = NULL;
  splitChunk->prev = NULL;
  return splitChunk;
}

/*
Function: split the chunk and return used chunk
*/
void * reuse_chunk(chunk * ptr,
                   size_t size,
                   chunk ** free_region_Head,
                   chunk ** free_region_Tail) {
  if (ptr->size >= size + META_SIZE) {  //find space that large enough
    chunk * split = split_chunk(size, ptr);

    ptr->size = size;

    extend_chunk(split, free_region_Head, free_region_Tail);
    remove_chunk(ptr, free_region_Head, free_region_Tail);
    free_size -= size + META_SIZE;
  }
  else {
    remove_chunk(ptr, free_region_Head, free_region_Tail);
    free_size -= ptr->size + META_SIZE;
  }
  ptr->free = 0;
  ptr->next = NULL;
  ptr->prev = NULL;
  return (char *)ptr + META_SIZE;
}

/*
Function:checking Find a free chunk and return it straightforward.
Iterate through linked list to see if there's a best fit free chunk.
*/
void * bf_find_free_chunk(size_t size,
                          chunk ** free_region_Head,
                          chunk ** free_region_Tail) {
  chunk * ptr = *free_region_Head;
  chunk * bf_ptr = NULL;  //record minimal best fit chunk
  while (ptr) {
    if (ptr->size >= size) {
      if (!bf_ptr || ptr->size < bf_ptr->size) {
        bf_ptr = ptr;
      }
      if (ptr->size == size) {
        return reuse_chunk(bf_ptr, size, free_region_Head, free_region_Tail);
      }
    }
    ptr = ptr->next;
  }
  if (!bf_ptr) {
    return NULL;
  }
  else {
    return reuse_chunk(bf_ptr, size, free_region_Head, free_region_Tail);
  }
}
/*
Function: Merge chunk
*/
void mergeRight(chunk * chk, chunk ** free_region_Head, chunk ** free_region_Tail) {
  if (chk->next && ((char *)chk + chk->size + META_SIZE == (char *)chk->next)) {
    chk->size += META_SIZE + chk->next->size;
    remove_chunk(chk->next, free_region_Head, free_region_Tail);
  }
}

void mergeLeft(chunk * chk, chunk ** free_region_Head, chunk ** free_region_Tail) {
  if (chk->prev && ((char *)chk->prev + chk->prev->size + META_SIZE == (char *)chk)) {
    chk->prev->size += META_SIZE + chk->size;
    remove_chunk(chk, free_region_Head, free_region_Tail);
  }
}

void bf_free(void * ptr, chunk ** free_region_Head, chunk ** free_region_Tail) {
  if (!ptr) {  //call free with a NULL ptr
    return;
  }
  chunk * pointer;
  pointer = (chunk *)((char *)ptr - META_SIZE);
  pointer->free = 1;
  free_size += pointer->size + META_SIZE;
  extend_chunk(pointer, free_region_Head, free_region_Tail);

  //merge chunk
  mergeRight(pointer, free_region_Head, free_region_Tail);
  mergeLeft(pointer, free_region_Head, free_region_Tail);
}

void * bf_malloc(size_t size, chunk ** free_region_Head, chunk ** free_region_Tail) {
  chunk * _chunk;
  if (size <= 0) {
    return NULL;
  }
  if (!*free_region_Head) {
    _chunk = allocate_space(size);
    if (!_chunk) {
      return NULL;
    }
  }
  else {
    _chunk = bf_find_free_chunk(size, free_region_Head, free_region_Tail);
    if (!_chunk) {  //there is no space
      _chunk = allocate_space(size);
      if (!_chunk) {
        return NULL;
      }
    }
  }
  return _chunk;
}

unsigned long get_data_segment_size() {
  return data_size;
}

unsigned long get_data_segment_free_space_size() {
  return free_size;
}
