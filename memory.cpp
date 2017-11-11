#include "mode32.h"

/**
 * Initializes buddy allocator
 * Initializes @global cps
**/
void mem_init()
{
    buddy_init();
    cps = (cache_t *)malloc(sizeof(cache_t) * num_prnts);

    for (uint32_t i = 0; i < num_prnts; i++)
    {
        cps[i].size = prnts[i].num_bytes32;
        assert(cps[i].size >= 4);
        assert(cps[i].size < PAGE_SZ / 8);
        cps[i].slabs = NULL;
        cps[i].slabs_back = NULL;
    }
}

cache_t *ncps;

/**
 * Logic to switch from 32 bit mode to 64 bit mode
 * Occurs in 4 steps 
 * 1. Relocate memory 
 * 2. Copy objects
 * 3. Free buffers 
 * 4. Free old memory
**/
void swtch()
{
    // Store old cps
    ncps = (cache_t *)malloc(sizeof(cache_t) * num_prnts);

    for (uint32_t i = 0; i < num_prnts; i++)
    {
        ncps[i] = cps[i];
    }

    // Update cps
    for (uint32_t i = 0; i < num_prnts; i++)
    {
        cps[i].size = prnts[i].num_bytes64;
        cps[i].slabs = NULL;
        cps[i].slabs_back = NULL;
    }

    // Relocate memory
    for (uint32_t i = 0; i < num_prnts; i++)
    {
        reloc(i);
    }

    addrs[0] = conv64(addrs[0]);

    // Object copy
    for (uint32_t i = 0; i < num_prnts; i++)
    {
        objcpy(i);
    }

    // Free buffers
    for (uint32_t i = 0; i < num_prnts; i++)
    {
        freebuf(i);
    }

    // Free old memory
    for (uint32_t i = 0; i < num_prnts; i++)
    {
        cache_destroy(&ncps[i]);
    }

    free(ncps);
}

/**
 * Allocates memory for 64 bit objects
 * Maintains a mapping from old to new objects
***/
void reloc(uint32_t type)
{
    slab_t *slab = ncps[type].slabs;

    // Proceed only when not null
    if (slab)
    {
        slab_t *p = slab;

        // Traverse slabs
        do
        {
            uint32_t slab32_sz = buddy_size((uint8_t *)p);
            assert(slab32_sz == PAGE_SZ);
            uint8_t *mem32 = (uint8_t *)p - slab32_sz + sizeof(slab_t);
            uint32_t slab_maxbuf = (slab32_sz - sizeof(slab_t)) / ncps[type].size;
            uint8_t *mem64 = cache_alloc(type, slab_maxbuf);
            p->loc = mem64;
            p = p->next;
        } while (p != slab);
    }
}

/**
 * @param type to copy
**/
void objcpy(uint32_t type)
{
    vnode_t<field_t> *fnode = prnts[type].fields;
    uint32_t offset32 = 0;
    uint32_t offset64 = 0;

    while (fnode)
    {
        field_t &lfield = fnode->val;

        // Reference
        if (!lfield.prim_sz)
        {
            slab_t *slab = ncps[type].slabs;

            if (slab)
            {
                slab_t *p = slab;

                do
                {
                    uint32_t slab32_sz = buddy_size((uint8_t *)p);
                    assert(slab32_sz == PAGE_SZ);
                    uint32_t slab_maxbuf = (slab32_sz - sizeof(slab_t)) / ncps[type].size;
                    uint8_t *mem32 = (uint8_t *)p - slab32_sz + sizeof(slab_t);

                    for (uint32_t i = 0; i < slab_maxbuf; i++)
                    {
                        uint8_t *pbase = mem32 + i * ncps[type].size;
                        uint8_t *paddr = memory + *(uint32_t *)(pbase + offset32);
                        *(uint64_t *)(conv64(pbase) + offset64) = conv64(paddr) - memory;
                    }

                    p = p->next;
                } while (p != slab);
            }

            offset32 += sizeof(uint32_t);
            offset64 += sizeof(uint64_t);
        }
        // Primitive type
        else
        {
            uint64_t ltype = lfield.type;
            assert(prnts[ltype].num_bytes32 == prnts[ltype].num_bytes64);

            slab_t *slab = ncps[type].slabs;

            if (slab)
            {
                slab_t *p = slab;

                do
                {
                    uint32_t slab32_sz = buddy_size((uint8_t *)p);
                    assert(slab32_sz == PAGE_SZ);
                    uint8_t *mem32 = (uint8_t *)p - slab32_sz + sizeof(slab_t);
                    uint32_t slab_maxbuf = (slab32_sz - sizeof(slab_t)) / ncps[type].size;

                    for (uint32_t i = 0; i < slab_maxbuf; i++)
                    {
                        uint8_t *pbase = mem32 + i * (ncps[type].size);
                        memcpy(conv64(pbase) + offset64, pbase + offset32, lfield.prim_sz);
                    }

                    p = p->next;
                } while (p != slab);
            }

            offset32 += lfield.prim_sz;
            offset64 += lfield.prim_sz;
        }

        fnode = fnode->next;
    }
}

/***
** Frees corresponding buffers
** Free list reversed
***/
void freebuf(uint32_t type)
{
    slab_t *slab = ncps[type].slabs;

    if (slab)
    {
        slab_t *p = slab;

        // Traverse slabs
        do
        {
            uint8_t *buf = p->free_list;
            uint32_t slab32_sz = buddy_size((uint8_t *)p);
            assert(slab32_sz == PAGE_SZ);
            uint32_t slab_maxbuf = (slab32_sz - sizeof(slab_t)) / ncps[type].size;
            uint8_t *mem32 = (uint8_t *)p - slab32_sz + sizeof(slab_t);

            // TODO: Stores list in reverse
            while (buf < mem32 + slab32_sz)
            {
                cache_free(type, conv64(buf));
                buf = mem32 + *(uint32_t *)buf;
            }

            p = p->next;
        } while (p != slab);
    }
}

uint8_t *conv64(uint8_t *paddr)
{
    uint32_t slab32_sz = buddy_size(paddr);
    uint8_t *mem32 = ((paddr - memory) / slab32_sz) * slab32_sz + memory;
    slab_t *slab32 = (slab_t *)(mem32 + slab32_sz - sizeof(slab_t));
    uint32_t idx32 = (paddr - mem32) / ncps[slab32->type].size;
    uint8_t *mem64 = slab32->loc;
    return mem64 + idx32 * cps[slab32->type].size;
}

bool eqtype(uint32_t type, uint8_t *paddr)
{
    assert(paddr >= memory);
    assert(paddr < memory + MEM_SZ64);
    uint32_t slab_sz = buddy_size(paddr);

    if (paddr == memory + slab_sz)
    {
        return false;
    }

    uint8_t *mem = ((paddr - memory) / slab_sz) * slab_sz + memory;
    slab_t *slab = (slab_t *)(mem + slab_sz - sizeof(slab_t));
    return slab->type == type;
}