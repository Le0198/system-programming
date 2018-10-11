/**
* Malloc Lab
* CS 241 - Fall 2018
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
typedef struct _metadata_t {
    struct _metadata_t* next;
    struct _metadata_t* prev;
    struct _metadata_t* freenext;
    struct _metadata_t* freeprev;
    
    void* ptr;
    size_t size;
    int free;

} metadata_t;

static metadata_t* tail = NULL;
static metadata_t* freehead = NULL;

void *malloc(size_t size);
void split(metadata_t* p, size_t sz);
//void merge(metadata_t* p);
//void mergeprev(metadata_t* p);

void insertfree(metadata_t* p) {
  p->freeprev = NULL;
  p->freenext = freehead;
  if (freehead != NULL) {freehead->freeprev = p;}
  freehead = p;    
}

void removefree(metadata_t* p) {

  metadata_t* tmp_prev = p->freeprev;
  metadata_t* tmp_next = p->freenext;
  p->freeprev = NULL;
  p->freenext = NULL;
  if (tmp_next != NULL) { tmp_next->freeprev = tmp_prev; }
  if (tmp_prev != NULL) { tmp_prev->freenext = tmp_next; }
  if (p == freehead) { freehead = tmp_next; }
//  if (p == freehead && freehead->freenext == NULL) {freehead = NULL;}
}



/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    void* p = malloc(num*size);
    if (p == NULL) {return NULL;}
    memset(p, 0, num*size);
    return p;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    // implement malloc!
    if (!size) {return NULL;}
/*
    if (tail == NULL && freehead == NULL) {
	freehead = sbrk(3000);
	if(freehead == (void*)-1) {freehead = NULL;}
	else {
	    freehead->ptr = (void*)(freehead + 1);
	    freehead->free = 1;
	    freehead->size = 3000 - sizeof(metadata_t);
	    freehead->next = NULL;
    	    freehead->prev = NULL;
    	    tail = freehead;
    	    freehead->freenext = NULL;
    	    freehead->freeprev = NULL;
	}
    }
*/
    metadata_t* chosen = NULL;
    metadata_t* tmp = freehead;
    
    while(tmp != NULL){
	if (tmp->size >= size) {
	   chosen = tmp;
	   removefree(tmp);
	   break;
	}
	tmp = tmp->freenext;
    }
    
    if (chosen) {
	split(chosen, size);
	chosen->free = 0;
	chosen->size = size;
	return chosen->ptr;
    }

    void* myptr = sbrk(sizeof(metadata_t) + size); 
//    if (myptr == (void*)-1) {myptr = sbrk(sizeof(metadata_t) + size);}
    if (myptr == (void*)-1) {return NULL;}
    chosen = myptr;
    chosen->ptr = (void*)(chosen + 1);
    chosen->size = size;
    chosen->free = 0;
    chosen->next = NULL;
    if (tail != NULL) {
       tail->next = chosen;
    }
    chosen->prev = tail;
    tail = chosen;
    chosen->freenext = NULL;
    chosen->freeprev = NULL;
    split(chosen, size);
    return chosen->ptr;
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!

    if (ptr == NULL) {return;}
    metadata_t* p = (metadata_t*)ptr - 1;
    if (p == NULL || p->free == 1) {return;}
    p->free = 1;
//if (!p->size){fprintf(stderr, "%d  ", (int)p->size);}
    metadata_t* temp = p->next;
    if (p->next != NULL) {
       if (p->next->free == 1) {
           p->size += (p->next->size + sizeof(metadata_t));
	   removefree(temp);
           p->next = temp->next;
           if (p->next != NULL) {p->next->prev = p;}
           if (p->next == NULL) { tail = p; }
           temp->next = NULL;
           temp->prev = NULL;
        }
    }
    
    if (p->prev != NULL) {
        if (p->prev->free == 1) {
          temp = p->prev;
          removefree(temp);
          temp->next = p->next;
          if (p->next != NULL) { p->next->prev = temp; }
          if (p->next == NULL) { tail = temp; }
          temp->size += (p->size + sizeof(metadata_t));
          p->next = NULL;
          p->prev = NULL;
          insertfree(temp);
	  return;
       }
    }

    insertfree(p);
/*
    if(p == tail && tail->free == 1 && tail->prev == NULL && tail->next == NULL) {
	sbrk(-(tail->size + sizeof(metadata_t)));
    }
*/
    return;

}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
osen = myptr;
    chosen->ptr = (void*)(chosen + 1);
    chosen->size = size;
    chosen->free = 0;
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    if (!ptr) { return malloc(size); }
    if (!size) { free(ptr); return NULL; }
    metadata_t *p = (metadata_t*)ptr - 1;
    if (p == NULL) {return NULL;}
    size_t oldsz = p->size;
    if (oldsz >= size) {return ptr;}
    
    if (p->next != NULL) {
       if (p->next->size + oldsz >= size && p->next->free == 1) {
           metadata_t* temp = p->next;
	   p->size += (p->next->size + sizeof(metadata_t));
           removefree(temp);
           p->next = temp->next;
           if (p->next != NULL) {p->next->prev = p;}
           if (p->next == NULL) { tail = p; }
           temp->next = NULL;
           temp->prev = NULL;
	   return p->ptr;
        }
    }
     
    void *newptr = malloc(size);
    if(newptr == NULL) {return NULL;}
    memcpy(newptr, ptr, oldsz);
    free(ptr);
    return newptr;
}


/*
void merge(metadata_t* p){
    if (p->next != NULL) {
	if (p->next->free == 1) {
	    p->size += (p->next->size + sizeof(metadata_t));
	    metadata_t* temp = p->next;
	    removefree(temp);
    	    p->next = temp->next;
      	    if (p->next != NULL) {p->next->prev = p;}
	    if (p->next == NULL) { tail = p; }
	    temp->next = NULL;
	    temp->prev = NULL;
	}
    }
}

void mergeprev(metadata_t* p){
    if (p->prev != NULL) {
        if (p->prev->free == 1) {
	  metadata_t* temp = p->prev;
	  removefree(temp);
          temp->next = p->next;
          if (p->next != NULL) { p->next->prev = temp; } 
          if (p->next == NULL) { tail = temp; }
          temp->size += (p->size + sizeof(metadata_t));
	  p->next = NULL;
	  p->prev = NULL;
	  p = temp;
       }
    }
}
*/

void split(metadata_t* p, size_t sz) {
   if (p->size <= sz + 3 * sizeof(metadata_t)) { return; }
   metadata_t* tmp = p->next;
   metadata_t* newptr = (metadata_t*)((char*)(p->ptr) + sz);
   if (newptr == NULL) {return;}
   newptr->size = p->size - sz - sizeof(metadata_t);
   newptr->ptr = (void*)(newptr + 1);
   newptr->free = 1;
   newptr->next = tmp;
   if (tmp != NULL) { tmp->prev = newptr; }
   if (tmp == NULL) { tail = newptr; }
   newptr->prev = p;
   newptr->freenext = NULL;
   newptr->freeprev = NULL;
   p->next = newptr;
   insertfree(newptr);
   return;
}



