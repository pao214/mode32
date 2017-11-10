#ifndef SLAB_H
#define SLAB_H

#include <cstdint>
#include "blprnt.h"

#define PAGE_SZ (1 << 12)

/***
** @field next is next element in list
** @field prev is previous element in list
** @field free_list is head of free objects list
** @field bufcount is complement of size of list
** @field loc is location of slab in 64 bit mode
** @field type represents type of objects in slab
** @field size is array size, 0 otherwise
***/
struct slab_t
{
    slab_t* next;
    slab_t* prev;
    uint8_t* free_list;
    size_t bufcount;
    uint8_t* loc;
    size_t type;
    size_t arr_sz;
};

/***
** @field type is object type
** @field size is object size
** @field slab_maxbuf is maximum buffers in a slab
** @field slabs is head of slab list
** @field slabs_back is tail of slab list
***/
struct cache_t
{
    size_t type;
    size_t size;
    size_t slab_maxbuf;
    slab_t* slabs;
    slab_t* slabs_back;
};

/***
** Allocation/free functions at slab level
***/
uint8_t* cache_alloc(size_t type);
void cache_free(size_t type, uint8_t* buf);
void cache_grow(cache_t* cp);
void cache_destroy(cache_t* cp);

/***
** Helper functions to deal with slabs
***/
inline void __slab_remove(cache_t* cp, slab_t* slab);
inline void __slab_move_to_front(cache_t* cp, slab_t* slab);
inline void __slab_move_to_back(cache_t* cp, slab_t* slab);

#endif
