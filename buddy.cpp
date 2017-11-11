#include "mode32.h"

uint8_t *btr;

/**
 * TODO: Bit manipulation optimization
 * @param s contiguous memory to be allocated
 * @return pointer to contiguous memory if allocated, NULL otherwise
 * Compute closest size that can be allocated
**/
uint8_t *buddy_alloc(uint64_t s)
{
    assert(s);
    // Page level granularity
    s = (s - 1) / PAGE_SZ + 1;
    uint64_t size = __nextpow2(s);
    uint64_t len = 1 << MAX_LVL;

    // Too large
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
                return memory + __idx_off(idx, lvl) * PAGE_SZ;
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

        // Traverse up the tree till we reach a left child
        while (true)
        {
            if (!idx)
            {
                return NULL;
            }

            lvl--;
            len *= 2;
            idx = (idx + 1) / 2 - 1;

            if (idx & 1)
            {
                ++idx;
                break;
            }
        }
    }

    return NULL;
}

/**
 * @param idx freed
 * Merges with neighbour if available
 **/
void combine(uint64_t idx)
{
    // Immediate sibling in tree
    uint64_t nbr = idx - 1 + (idx & 1) * 2;

    // Traverse up the tree
    while (idx && btr[nbr] == NODE_UNUSED)
    {
        idx = (idx + 1) / 2 - 1;
        nbr = idx - 1 + (idx & 1) * 2;
    }

    // No node allocated in subtree
    btr[idx] = NODE_UNUSED;

    // Ancestors are no longer full
    while (idx && btr[(idx + 1) / 2 - 1] == NODE_FULL)
    {
        idx = (idx + 1) / 2 - 1;
        btr[idx] = NODE_SPLIT;
    }
}

/**
 * @param ptr to be freed
 * Frees the chunk the pointer belongs to
**/
void buddy_free(uint8_t *ptr)
{
    assert(!((ptr - memory) % PAGE_SZ));
    // Page level granularity
    uint64_t off = (ptr - memory) / PAGE_SZ;
    uint64_t left = 0;
    uint64_t len = 1 << MAX_LVL;
    // Ensure we do not reach out of line
    assert(off < len);
    uint64_t idx = 0;

    // Traverse down the tree
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

    assert(btr[idx] == NODE_USED);
    assert(off == left);
    combine(idx);
}

/**
 * @param ptr present in chunk
 * @return size of chunk
**/
uint64_t buddy_size(uint8_t *ptr)
{
    // Page level granularity
    uint64_t off = (ptr - memory) / PAGE_SZ;
    uint64_t left = 0;
    uint64_t len = 1 << MAX_LVL;
    assert(off < len);
    uint64_t idx = 0;

    // Traverse down the tree
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

    assert(btr[idx] == NODE_USED);
    assert(left <= off && off < left + len);
    return len * PAGE_SZ;
}

/**
 * Initialize memory, tree and mark all nodes as unused
**/
void buddy_init()
{
    posix_memalign((void **)(&memory), PAGE_SZ, MEM_SZ64);
    btr = (uint8_t *)malloc(sizeof(uint8_t) * (2 * (1 << MAX_LVL) - 1));
    memset(btr, NODE_UNUSED, sizeof(uint8_t) * (2 * (1 << MAX_LVL) - 1));
}

/**
 * @param x
 * @return true if x is a power of 2, false otherwise
**/
inline bool __ispow2(uint64_t x)
{
    assert(x);
    return !(x & (x - 1));
}

/**
 * @param x
 * @return next power of 2
**/
inline uint64_t __nextpow2(uint64_t x)
{
    if (!x)
    {
        return 1;
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
    return x + 1;
}

/**
 * @param idx
 * @param lvl
 * @return offset from start
 * Find offset in that level and then multiply by size of chunk
**/
inline uint64_t __idx_off(uint64_t idx, uint8_t lvl)
{
    assert((idx + 1) >= (1 << lvl));
    return ((idx + 1) - (1 << lvl)) << (MAX_LVL - lvl);
}

/**
 * @param idx
 * Marks ancestors of idx that become full 
**/
inline void __mark_par(uint64_t idx)
{
    // Immediate sibling in tree
    uint64_t nbr = idx - 1 + (idx & 1) * 2;

    // Traverse up the tree
    while (idx && (btr[nbr] == NODE_USED || btr[nbr] == NODE_FULL))
    {
        idx = (idx + 1) / 2 - 1;
        btr[idx] = NODE_FULL;
        nbr = idx - 1 + (idx & 1) * 2;
    }
}