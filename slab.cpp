#include "mode32.h"

/**
 * @param type to be allocated
 * @return pointer to object
 * Allocates an object
 * Use this for object allocation
**/
uint8_t *cache_alloc(uint32_t type)
{
    cache_t *cp = &cps[type];

    // Cache empty
    if (!(cp->slabs))
    {
        return cache_alloc(type, 1);
    }

    slab_t *slab = cp->slabs;
    uint64_t slab_sz = buddy_size((uint8_t *)slab);
    uint64_t slab_maxbuf = (slab_sz - sizeof(slab_t)) / (cp->size);

    // All slabs full
    if (slab->bufcount == slab_maxbuf)
    {
        return cache_alloc(type, 1);
    }

    // Remove from free list
    uint8_t *mem = (uint8_t *)slab - slab_sz + sizeof(slab_t);
    uint8_t *buf = slab->free_list;
    slab->free_list = mem + (*((uint32_t *)buf));
    (slab->bufcount)++;

    // Front slab must not be full if possible
    if (slab->bufcount == slab_maxbuf)
    {
        __slab_move_to_back(cp, slab);
    }

    return buf;
}

/**
 * @param type to be allocated
 * @param arr_sz number of objects to be allocated
 * Use only when need of array of objects
**/
uint8_t *cache_alloc(uint32_t type, uint32_t arr_sz)
{
    cache_t *cp = &cps[type];
    uint8_t *mem = buddy_alloc(cp->size * arr_sz + sizeof(slab_t));
    uint64_t slab_sz = buddy_size(mem);
    assert(slab_sz <= MEM_SZ64);
    uint32_t slab_maxbuf = (slab_sz - sizeof(slab_t)) / cp->size;
    slab_t *slab = (slab_t *)(mem + slab_sz - sizeof(slab_t));
    slab->next = slab->prev = slab;
    slab->free_list = mem + cp->size * arr_sz;
    slab->bufcount = arr_sz;
    slab->loc = NULL;
    slab->type = type;
    slab->arr_sz = arr_sz;
    slab->slab_sz = slab_sz;
    uint8_t *lastbuf = mem + cp->size * (slab_maxbuf - 1);

    // Allocate free list with tail pointed to outside memory
    for (uint8_t *p = slab->free_list; p < lastbuf; p += cp->size)
    {
        *((uint32_t *)p) = (uint32_t)((p - mem) + cp->size);
    }

    *((uint32_t *)lastbuf) = slab_sz;

    // Add slab to list
    __slab_move_to_front(cp, slab);

    // Front slab must not be full if possible
    if (slab->bufcount == slab_maxbuf)
    {
        __slab_move_to_back(cp, slab);
    }

    return mem;
}

/**
 * @param type of object
 * @param buf to be freed
 * Adds buf to free list and frees slab if not required
**/
void cache_free(uint32_t type, uint8_t *buf)
{
    cache_t *cp = &cps[type];
    uint32_t slab_sz = buddy_size(buf);
    uint32_t slab_maxbuf = (slab_sz - sizeof(slab_t)) / cp->size;
    uint8_t *mem = memory + ((buf - memory) / slab_sz) * slab_sz;
    slab_t *slab = (slab_t *)(mem + slab_sz - sizeof(slab_t));
    assert((!slab->arr_sz) || (buf >= memory + cp->size * slab->arr_sz) || (buf == mem));

    // Add buffer to free list
    if ((buf == mem) && slab->arr_sz)
    {
        uint8_t *lastbuf = mem + cp->size * (slab->arr_sz - 1);

        for (uint8_t *p = mem; p < lastbuf; p += cp->size)
        {
            *((uint32_t *)p) = (uint32_t)((p - mem) + cp->size);
        }

        *((uint32_t *)lastbuf) = (uint32_t)(slab->free_list - mem);
        slab->free_list = mem;
        slab->bufcount -= slab->arr_sz;
        slab->arr_sz = 0;

        // Front slab must not be full if possible
        if (slab->bufcount == slab_maxbuf - slab->arr_sz)
        {
            __slab_move_to_front(cp, slab);
        }
    }
    else
    {
        *((uint32_t *)buf) = (uint32_t)(slab->free_list - mem);
        slab->free_list = buf;
        (slab->bufcount)--;

        // Front slab must not be full if possible
        if (slab->bufcount == slab_maxbuf - 1)
        {
            __slab_move_to_front(cp, slab);
        }
    }

    // Empty slabs can be freed
    if (!slab->bufcount)
    {
        __slab_remove(cp, slab);
        buddy_free(mem);
    }
}

/**
 * @param cp to be freed
**/
void cache_destroy(cache_t *cp)
{
    slab_t *slab;
    uint8_t *mem;

    while (cp->slabs)
    {
        slab = cp->slabs;
        __slab_remove(cp, slab);
        uint64_t slab_sz = buddy_size((uint8_t *)slab);
        mem = (uint8_t *)slab - slab_sz + sizeof(slab_t);
        buddy_free(mem);
    }
}

/**
 * @param cp type
 * @param slab to be removed
 * Removes slab from list
**/
inline void __slab_remove(cache_t *cp, slab_t *slab)
{
    // Remove slab from list
    // Not removed if only single element is present
    slab->next->prev = slab->prev;
    slab->prev->next = slab->next;

    if (cp->slabs == slab)
    {
        // Only element
        if (slab->prev == slab)
        {
            cp->slabs = NULL;
        }
        // Previous element is new head
        else
        {
            cp->slabs = slab->prev;
        }
    }

    if (cp->slabs_back == slab)
    {
        // Only element
        if (slab->next == slab)
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

/**
 * Moves existing slab to front or appends slab to front
 * @param cp type
 * @param slab to be appended to front
**/
inline void __slab_move_to_front(cache_t *cp, slab_t *slab)
{
    if (cp->slabs == slab)
    {
        return;
    }

    __slab_remove(cp, slab);

    // Empty list
    if (!cp->slabs)
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

/**
 * Moves slab to back
 * @param cp type
 * @param slab to be moved to back
**/
inline void __slab_move_to_back(cache_t *cp, slab_t *slab)
{
    if (cp->slabs_back == slab)
    {
        return;
    }

    __slab_remove(cp, slab);

    // Never executed
    if (!cp->slabs)
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