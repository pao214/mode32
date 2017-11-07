#include "mode32.h"

/***
* @param cp type slab allocated
***/
void cache_grow(cache_t* cp)
{
    uint8_t* mem = page_alloc();
    slab_t* slab = (slab_t*)(mem+PAGE_SZ-sizeof(slab_t));
    slab->next = slab->prev = slab;
    slab->free_list = mem;
    slab->bufcount = 0;
    slab->loc = NULL;
    slab->type = cp->type;
    uint8_t* lastbuf = mem+(cp->size)*((cp->slab_maxbuf)-1);

    // Allocate free list with head at start
    // Lastbuf does not point to NULL
    for (uint8_t* p = mem; p < lastbuf; p += (cp->size))
    {
        *((uint32_t*)p) = (uint32_t)((p-memory)+(cp->size));
    }

    *((uint32_t*)lastbuf) = MEM_SZ64;

    // Add slab to list
    __slab_move_to_front(cp, slab);
}

/***
** @field type to be allocated
** @return pointer to object
***/
uint8_t* cache_alloc(size_t type)
{
    cache_t* cp = &cps[type];

    // Cache empty
    if (!(cp->slabs))
    {
        cache_grow(cp);
    }

    // All slabs full
    if ((cp->slabs->bufcount) == (cp->slab_maxbuf))
    {
        cache_grow(cp);
    }

    // Remove from free list
    slab_t* slab = cp->slabs;
    uint8_t* buf = slab->free_list;
    slab->free_list = memory+(*((uint32_t*)buf));
    (slab->bufcount)++;

    // Front slab must not be full if possible
    if ((slab->bufcount) == (cp->slab_maxbuf))
    {
        __slab_move_to_back(cp, slab);
    }

    return buf;
}

/***
** TODO: Change addressing to account for 64 bit
** Adds buf to free list and frees slab if not required
** @param type of object
** @param buf to be freed 
***/
void cache_free(size_t type, uint8_t* buf)
{
    cache_t* cp = &cps[type];
    uint8_t* mem = memory+((buf-memory)/PAGE_SZ)*PAGE_SZ;
    slab_t* slab = (slab_t*)(mem+PAGE_SZ-sizeof(slab_t));

    // Add buffer to free list
    *((uint32_t*)buf) = (uint32_t)((slab->free_list)-memory);
    slab->free_list = buf;
    (slab->bufcount)--;

    // Empty slabs can be freed
    if ((slab->bufcount) == 0)
    {
        __slab_remove(cp, slab);
        page_free(mem);
    }

    // Front slab must not be full if possible
    if ((slab->bufcount) == ((cp->slab_maxbuf)-1))
    {
        __slab_move_to_front(cp, slab);
    }
}

void cache_destroy(cache_t* cp) 
{
    slab_t* slab;
    uint8_t* mem;

    while (cp->slabs) 
    {
        slab = cp->slabs;
        __slab_remove(cp, slab);
        mem = ((uint8_t*)slab)-PAGE_SZ+sizeof(slab_t);
        page_free(mem);
    }
}

/***
** @param cp type
** @param slab to be removed
** Removes slab from list
***/
inline void __slab_remove(cache_t* cp, slab_t* slab)
{
    // Remove slab from list
    // Not removed if only single element is present
    slab->next->prev = slab->prev;
    slab->prev->next = slab->next;

    if ((cp->slabs) == slab)
    {
        // Only element
        if ((slab->prev) == slab)
        {
            cp->slabs = NULL;
        }
        // Previous element is new head
        else
        {
            cp->slabs = slab->prev;
        }
    }
    
    if ((cp->slabs_back) == slab)
    {
        // Only element
        if ((slab->next) == slab)
        {
            cp->slabs_back = NULL;
        }
        // Next element is new tail
        else
        {
            cp->slabs_back = slab->next;
        }
    }
}

/***
** Moves existing slab to front or appends slab to front
** @param cp type
** @param slab to be appended to front
***/
inline void __slab_move_to_front(cache_t* cp, slab_t* slab)
{
    if ((cp->slabs) == slab)
    {
        return;
    }

    __slab_remove(cp, slab);

    // Empty list
    if (!(cp->slabs))
    {
        slab->prev = slab;
        slab->next = slab;
        cp->slabs_back = slab;
    }
    // Append element to front
    else
    {
        slab->prev = cp->slabs;
        cp->slabs->next = slab;
        slab->next = cp->slabs_back;
        cp->slabs_back->prev = slab;
    }

    cp->slabs = slab;
}

/***
** Moves slab to back
** @param cp type
** @param slab to be moved to back
***/
inline void __slab_move_to_back(cache_t* cp, slab_t* slab)
{
    if ((cp->slabs_back) == slab)
    {
        return;
    }

    __slab_remove(cp, slab);

    // Never executed
    if (!(cp->slabs))
    {
        slab->prev = slab;
        slab->next = slab;
        cp->slabs = slab;
    }
    // Append element to back
    else
    {
        slab->prev = cp->slabs;
        cp->slabs->next = slab;
        slab->next = cp->slabs_back;
        slab->next = cp->slabs_back;
        cp->slabs_back->prev = slab;
    }

    cp->slabs_back = slab;
}