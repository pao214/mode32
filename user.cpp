#include "mode32.h"

#define ARR_SZ (1 << 16)
#define JUMP_SZ ((1 << 12) - 1)
#define ITER_SZ (1 << 30)

/**
 * Initialize prnts
**/
void prnts_init()
{
    num_prnts = 1;
    prnts = (blprnt_t *)malloc(sizeof(blprnt_t) * num_prnts);

    prnts[0].num_bytes32 = sizeof(uint32_t);
    prnts[0].num_bytes64 = sizeof(uint64_t);

    field_t lfield;
    lfield.prim_sz = 0;
    lfield.type = 0;

    prnts[0].fields = NULL;
    prnts[0].fields = vnode_insert<field_t>(prnts[0].fields, lfield);
}

uint32_t ptr32[ARR_SZ];

/**
 * Initilaize linked list
**/
void usr_init()
{
    for (uint64_t i = 0; i < ARR_SZ; i++)
    {
        ptr32[i] = (uint32_t)(cache_alloc(0) - memory);
    }

    for (int i = 0; i < ARR_SZ; i++)
    {
        *((uint32_t *)(memory + ptr32[i])) = ptr32[(i + JUMP_SZ) % ARR_SZ];
    }

    num_addrs = 1;
    addrs = (uint8_t **)malloc(sizeof(uint8_t *) * num_addrs);
    addrs[0] = memory + ptr32[0];
}

/**
 * Traversal in 32 bit mode
**/
void execute32()
{
    uint32_t idx = addrs[0] - memory;

    for (uint64_t i = 0; i < ITER_SZ; i++)
    {
        idx = *((uint32_t *)(memory + idx));
    }
}

/**
 * Traversal in 64 bit mode
**/
void execute64()
{
    uint64_t idx = addrs[0] - memory;

    for (uint64_t i = 0; i < ITER_SZ; i++)
    {
        idx = *(uint64_t *)(memory + idx);
    }
}
