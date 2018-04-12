/**
* Malloc Lab
* CS 241 - Spring 2018
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOCK_SIZE sizeof(struct block)

typedef struct block {
    size_t size;
    struct block *next;  //boundary
    struct block *prev;  //boundary

    struct block *next_free;
    struct block *prev_free;
} block;

static block* head = NULL;  //head of free list
static block* tail = NULL; //tail of the actual address

void printList(){
    block* ret = head;
    while(ret){
        printf("%p\n",ret+1);
        ret = ret->next_free;
    }
}

size_t isFree(block* md){
    if(md == head){
        return 1;
    }
    if(md->next_free == NULL&& md->prev_free == NULL){
        return 0;
    }
    return 1;
}

void removeFree(block* md){        //removeFree from free list
    if(md==head){
        head = md->next_free;
        if(head){
            head->prev_free = NULL;
        }
        md->prev_free = NULL;
        md->next_free = NULL;
        return;
    }
    if(md->prev_free){
        md->prev_free->next_free = md->next_free;
    }
    if(md->next_free){
        md->next_free->prev_free = md->prev_free;
    }
    md->prev_free = NULL;
    md->next_free = NULL;
}

void split(block* md, int size){
    if(md->size - size <= BLOCK_SIZE){
        return;
    }
    //printf("%zu\n", md->size-size-BLOCK_SIZE);
    block* new_mb = (block*)((char*)(md+1)+size);
    //if(!new_mb->size){printf("fuck\n");}
    new_mb->size = md->size - size - BLOCK_SIZE;
    new_mb->next = md->next;
    if(md->next){
        md->next->prev = new_mb;
    }
    new_mb->prev = md;
    md->size = size;
    md->next = new_mb;
    if(md==tail){
        tail  = new_mb;
    }
    //boundary tag modified
    //now dll
    new_mb->prev_free = NULL;
    new_mb->next_free = head;
    if(head){
        head->prev_free = new_mb;
    }
    head = new_mb;
}

void merge(block* ret){
    //block* ret = *data;
    if(ret->next && isFree(ret->next)){
          //removeFree from free list
            removeFree(ret->next);
            ret->size += BLOCK_SIZE + ret->next->size;
            if(ret->next == tail){
                tail = ret;
                ret->next = NULL;
            }
            else{
            	ret->next->next->prev = ret;
            	ret->next = ret->next->next;
        	}
    }
    else{

    }
}



block* find(size_t size){
    block* ret = head;
//    int i =0;
    while(ret){
        //printf("%d\n", i);
        if(ret->next && isFree(ret->next)){
            merge(ret);
            //if(ret->next) ret->next_free = ret->next->next_free;
        }
        if(ret->size>=size){
            return ret;
        }
        ret = ret->next_free;
 //       i++;
    }
    return ret;
}


block* get_md(void* ptr){
    return (block*)ptr - 1;
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
void* malloc(size_t size){
    block* md;  //no free block in free list
    if(head==NULL){
        md = sbrk(0);
        sbrk(BLOCK_SIZE+size);
        md->size = size;
        if(tail==NULL){
            tail = md;
            md->prev = NULL;
            md->next = NULL;

        }else{
            md->prev = tail;
            tail->next = md;
            md->next = NULL;
            tail = md;
        }
            md->prev_free = NULL;
            md->next_free = NULL;
            head = NULL;

    }else{
        md = find(size);
        if(md==NULL){
            md = sbrk(0);
            sbrk(BLOCK_SIZE+size);
            md->size = size;
            if(tail == NULL){
                tail = md;
                md->prev = NULL;
                md->next = NULL;
            }else{
                md->prev = tail;
                tail->next = md;
                md->next = NULL;
                tail = md;
            }
            md->prev_free = NULL;
            md->next_free = NULL;
        }else{
            //r femoveFree from free list
            // if(md==head){
            //     head = md->next_free;
            // }
            if(md->size-size>BLOCK_SIZE+3*size/4){
                split(md,size);
            }
            removeFree(md);
        }
    }
    //printList();
    return (void*)(md+1);
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
    size_t total = num*size;
    void* ret = malloc(total);
    if(!ret){
        return NULL;
    }
    memset(ret,0,total);
    return ret;
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
void free(void* ptr){
    if(!ptr) return;

    block* md = get_md(ptr);

     if(!head){
        head = md;
        md->prev_free = NULL;
        md->next_free = NULL;
    }else {
        md->next_free = head;
        md->prev_free = NULL;
        head->prev_free = md;
        head = md;
    }

    if(md->next&&isFree(md->next)){
        merge(md);
    }
    if(md->prev&&isFree(md->prev)){
        merge(md->prev);
        md = md->prev;
    }

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
    if(!ptr){
        return malloc(size);
    }
    if(size ==0){
        free(ptr);
    }
    block* md = get_md(ptr);
    if(md->size>size){
        split(md,size);
        return ptr;
    }else{
        if(size==md->size){
            return ptr;
        }
        if(md->next&&isFree(md->next)&&md->next->size+BLOCK_SIZE+md->size>=size){
            merge(md);
            split(md,size);
        }else{
            //printf("here\n");
            void* vnew = malloc(size);
            memcpy(vnew,ptr,md->size);
            free(ptr);
            return vnew;
        }
    }
    return ptr;
}
