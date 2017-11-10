#ifndef BUDDY_H
#define BUDDY_H

#include "memory.h"

#define NODE_UNUSED 0
#define NODE_USED 1
#define NODE_SPLIT 2
#define NODE_FULL 3

uint8_t *buddy_alloc(uint64_t s);
void combine(uint64_t idx);
void buddy_free(uint8_t *ptr);
uint64_t buddy_size(uint8_t *ptr);
void buddy_init();

extern uint8_t* btr;

/***
** @field x
** @return true if x is a power of 2, false otherwise
***/
inline bool __ispow2(uint64_t x)
{
    assert(x);
    return !(x & (x - 1));
}

/***
** @field x
** @return next power of 2
***/
inline uint64_t __nextpow2(uint64_t x)
{
    x /= PAGE_SZ;

    if (!x)
    {
        return PAGE_SZ;
    }

    if (__ispow2(x))
    {
        return x;
    }

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return (x + 1) * PAGE_SZ;
}

/***
** @field idx 
** @field lvl
** @return offset from start
***/
inline uint64_t __idx_off(uint64_t idx, uint8_t lvl)
{
    return ((idx + 1) - (1 << lvl)) << (MEM_LVL - lvl);
}

/***
** @field idx
** Marks ancestors of idx that become full
***/
inline void __mark_par(uint64_t idx)
{
    if (!idx)
    {
        return;
    }

    uint64_t nbr = idx - 1 + (idx & 1) * 2;

    while ((btr[idx] == NODE_USED) || (btr[idx] == NODE_FULL))
    {
        idx = (idx + 1) / 2 - 1;
        btr[idx] = NODE_FULL;
        nbr = idx - 1 + (idx & 1) * 2;
    }
}

#endif