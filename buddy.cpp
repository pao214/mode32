#include "mode32.h"

uint8_t *btr;

/***
** @field x
** @return true is x is a power of 2, false otherwise
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

    while ((nbr > 0) && ((btr[idx] == NODE_USED) || (btr[idx] == NODE_FULL)))
    {
        idx = (idx + 1) / 2 - 1;
        btr[idx] = NODE_FULL;
        nbr = idx - 1 + (idx & 1) * 2;
    }
}

/***
** @field s is size to be allocated
** @return pointer to continuous memory allocated, NULL if not possible
** Compute closest size that can be allocated
***/
uint8_t *buddy_alloc(uint64_t s)
{
    uint64_t size = __nextpow2(s);
    uint64_t len = 1 << MEM_LVL;

    if (size > len)
    {
        return NULL;
    }

    uint64_t idx = 0;
    uint8_t lvl = 0;

    while (idx >= 0)
    {
        if (size == len)
        {
            // Closest contiguous unused memory found
            if (btr[idx] == NODE_UNUSED)
            {
                btr[idx] = NODE_USED;
                __mark_par(idx);
                return memory + __idx_off(idx, lvl);
            }
        }
        // Split unused memory
        else if (btr[idx] == NODE_UNUSED)
        {
            btr[idx] = NODE_SPLIT;
            btr[idx * 2 + 1] = NODE_UNUSED;
            btr[idx * 2 + 2] = NODE_UNUSED;
            idx = idx * 2 + 1;
            len /= 2;
            lvl++;
            continue;
        }
        // Select left child
        else if (btr[idx] == NODE_SPLIT)
        {
            idx = idx * 2 + 1;
            len /= 2;
            lvl++;
            continue;
        }

        // Also check right child
        if (idx & 1)
        {
            ++idx;
            continue;
        }

        while (true)
        {
            if (idx)
            {
                return NULL;
            }

            lvl--;
            len *= 2;
            idx = (idx + 1) / 2 - 1;

            if (idx & 1)
            {
                break;
            }
        }
    }

    return NULL;
}

void combine(uint64_t idx)
{
    if (!idx)
    {
        btr[idx] = NODE_UNUSED;
    }

    uint64_t nbr = idx - 1 + (idx & 1) * 2;

    while (idx && (nbr >= 0) && (btr[nbr] == NODE_UNUSED))
    {
        idx = (idx + 1) / 2 - 1;
    }

    btr[idx] = NODE_UNUSED;

    while (idx && (btr[(idx + 1) / 2 - 1] == NODE_FULL))
    {
        idx = (idx + 1) / 2 - 1;
        btr[idx] = NODE_SPLIT;
    }
}

void buddy_free(uint8_t *ptr)
{
    uint64_t off = ptr - memory;
    assert(off < (1 << MEM_LVL));
    uint64_t left = 0;
    uint64_t len = 1 << MEM_LVL;
    uint64_t idx = 0;

    while ((btr[idx] == NODE_SPLIT) || (btr[idx] == NODE_FULL))
    {
        len /= 2;

        if (off < (left + len))
        {
            idx = idx * 2 + 1;
        }
        else
        {
            left += len;
            idx = idx * 2 + 2;
        }
    }

    if (btr[idx] == NODE_USED)
    {
        assert(off == left);
        combine(idx);
    }

    if (btr[idx] == NODE_UNUSED)
    {
        assert(false);
    }
}

uint64_t buddy_size(uint64_t off)
{
    uint64_t left = 0;
    uint64_t len = 1 << MEM_LVL;
    assert(off<len);
    uint64_t idx = 0;

    while ((btr[idx] == NODE_SPLIT) || (btr[idx] == NODE_FULL))
    {
        len /= 2;

        if (off < (left + len))
        {
            idx = idx * 2 + 1;
        }
        else
        {
            left += len;
            idx = idx * 2 + 2;
        }
    }

    if (btr[idx] == NODE_USED)
    {
        assert(off == left);
        return len;
    }

    if (btr[idx] == NODE_UNUSED)
    {
        assert(false);
        return len;
    }
}

void buddy_init()
{
    posix_memalign((void **)(&memory), PAGE_SZ, MEM_SZ64);
    btr = (uint8_t *)malloc(sizeof(uint8_t) * (2 * (1 << MAX_LVL) - 1));
    memset(btr, NODE_UNUSED, sizeof(uint8_t) * (2 * (1 << MAX_LVL) - 1));
}