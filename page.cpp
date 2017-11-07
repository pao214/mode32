#include "mode32.h"

uint8_t* free_list;

/***
** Allocates a page
***/
uint8_t* page_alloc()
{
    assert(free_list);
    uint8_t* pg = free_list;
    free_list = *((uint8_t**)pg);
    return pg;
}

/***
** Frees the given page and appends to free list
***/
void page_free(uint8_t* &pg)
{
    assert(!((pg-memory)%PAGE_SZ));
    *((uint8_t**)pg) = free_list;
    free_list = pg;
    pg = NULL;
}

/***
** Allocate required memory aligned at @field PAGE_SZ
** Add all pages to free list
***/
void page_init()
{
    posix_memalign((void**)(&memory), PAGE_SZ, MEM_SZ64);

    for (uint8_t* p = memory; p < (memory+MEM_SZ64-PAGE_SZ); p += PAGE_SZ)
    {
        *((uint8_t**)p) = p+PAGE_SZ;
    }

    *((uint8_t**)(memory+MEM_SZ64-PAGE_SZ)) = NULL;
    free_list = memory;
}
