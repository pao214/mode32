#include "mode32.h"

/***
** Initializes page level memory
** Initializes @var cps
***/
void mem_init()
{
    page_init();
    cps = (cache_t*)malloc(sizeof(cache_t)*num_nodes);

    for (size_t i = 0; i < num_prnts; i++)
    {
        cps[i].type = i;
        cps[i].size = prnts[i].num_bytes32;
        cps[i].slab_maxbuf = (PAGE_SZ-sizeof(slab_t))/(cps[i].size);
        cps[i].slabs = NULL;
        cps[i].slabs_back = NULL;
    }

    cps[num_prnts].type = num_nodes;
    cps[num_prnts].size = sizeof(uint32_t);
    cps[num_prnts].slab_maxbuf = (PAGE_SZ-sizeof(slab_t))/sizeof(uint32_t);
    cps[num_prnts].slabs = NULL;
    cps[num_prnts].slabs_back = NULL;
}

void reloc(size_t type);
void objcpy(size_t type);
void freebuf(size_t type);
uint8_t* conv64(size_t type, uint8_t* paddr);
cache_t* ncps;

/***
** Logic to switch from 32 bit mode to 64 bit mode
** Occurs in 4 steps 
** 1. Relocate memory 
** 2. Copy objects
** 3. Free buffers 
** 4. Free old memory
***/
void swtch()
{
    // Store old cps
    ncps = (cache_t*)malloc(sizeof(cache_t)*num_nodes);

    for (int i = 0; i < num_nodes; i++)
    {
        ncps[i] = cps[i];
    }

    // Update cps
    for (int i = 0; i < num_prnts; i++)
    {
        cps[i].size = prnts[i].num_bytes64;
        cps[i].slab_maxbuf = (PAGE_SZ-sizeof(slab_t))/(cps[i].size);
        cps[i].slabs = NULL;
        cps[i].slabs_back = NULL;
    }

    cps[num_prnts].size = sizeof(uint64_t);
    cps[num_prnts].slab_maxbuf = (PAGE_SZ-sizeof(slab_t))/sizeof(uint64_t);
    cps[num_prnts].slabs = NULL;
    cps[num_prnts].slabs_back = NULL;

    // Relocate memory
    for (size_t i = 0; i < num_nodes; i++)
    {
        reloc(i);
    }

    addrs[0] = conv64(0, addrs[0]);

    // Object copy
    for (size_t i = 0; i < num_nodes; i++)
    {
        objcpy(i);
    }

    // Free buffers
    for (size_t i = 0; i < num_nodes; i++)
    {
        freebuf(i);
    }

    // Free old memory
    for (size_t i = 0; i < num_nodes; i++)
    {
        cache_destroy(&ncps[i]);
    }

    free(ncps);
}

/***
** Allocates memory for 64 bit objects
** Maintains a mapping from old to new objects
** loc in slabs of 32 bit points to location of first mapped memory
** loc in slabs of 64 bit mode points to next slab of memory
***/
void reloc(size_t type)
{
    slab_t* slab = ncps[type].slabs;

    // Proceed only when not null
    if (slab)
    {
        slab_t* p = slab;

        // Traverse slabs
        do
        {
            uint8_t *pbase, *pmem, *qmem;
            pbase = cache_alloc(type);
            pmem = memory+((pbase-memory)/PAGE_SZ)*PAGE_SZ;
            p->loc = pbase;

            // Assume serial allocation
            for (int i = 1; i < (ncps[type].slab_maxbuf); i++)
            {
                pbase = cache_alloc(type);
                qmem = memory+((pbase-memory)/PAGE_SZ)*PAGE_SZ;

                if (pmem != qmem)
                {
                    ((slab_t*)(pmem+PAGE_SZ-sizeof(slab_t)))->loc = qmem;
                    pmem = qmem;
                }
            }

            p = p->next;
        }
        while (p != slab);
    }
}

/***
** @param type to copy
***/
void objcpy(size_t type)
{
    vnode_t<field_t>* fnode = prnts[type].fields;
    int offset32 = 0;
    int offset64 = 0;

    while (fnode)
    {
        field_t& lfield = fnode->val;

        if (lfield.num_indirs)
        {
            slab_t* slab = ncps[type].slabs;

            if (slab)
            {
                slab_t* p = slab;

                do
                {
                    uint8_t* mem = ((uint8_t*)p)-PAGE_SZ+sizeof(slab_t);
                
                    // TODO: if num_indirs more than 1
                    for (int i = 0; i < (ncps[type].slab_maxbuf); i++)
                    {
                        uint8_t* pbase = mem+i*(ncps[type].size);
                        uint8_t* paddr = memory+(*((uint32_t*)(pbase+offset32)));
                        
                        if (eqtype(lfield.type, paddr))
                        {
                            *((uint64_t*)(conv64(type, pbase)+offset64)) = 
                                conv64(lfield.type, paddr)-memory;
                        }
                    }

                    p = p->next;
                }
                while (p != slab);
            }

            offset32 += sizeof(uint32_t);
            offset64 += sizeof(uint64_t);
        }
        // Must be a primitive type
        else
        {
            size_t ltype = lfield.type;
            assert(prnts[ltype].num_bytes32 == prnts[ltype].num_bytes64);

            slab_t* slab = ncps[type].slabs;
            
            if (slab)
            {
                slab_t* p = slab;

                do
                {
                    uint8_t* mem = ((uint8_t*)p)-PAGE_SZ+sizeof(slab_t);
                
                    for (int i = 0; i < (ncps[type].slab_maxbuf); i++)
                    {
                        uint8_t* pbase = mem+i*(ncps[type].size);
                        memcpy(conv64(ltype, pbase)+offset64, 
                            pbase+offset32, prnts[ltype].num_bytes32);
                    }

                    p = p->next;
                }
                while (p != slab);
            }

            offset32 += prnts[ltype].num_bytes32;
            offset64 += prnts[ltype].num_bytes64;
        }

        fnode = fnode->next;
    }
}

/***
** Frees corresponding buffers
** Free list reversed
***/
void freebuf(size_t type)
{
    slab_t* slab = ncps[type].slabs;

    if (slab)
    {
        slab_t* p = slab;

        // Traverse slabs
        do
        {
            uint8_t* buf = p->free_list;

            // TODO: Stores list in reverse
            for (int i = 0; i < ((ncps[i].slab_maxbuf)-(p->bufcount)); i++)
            {
                if ((buf-memory)==MEM_SZ64)
                {
                    break;
                }

                cache_free(type, conv64(type, buf));
                buf = memory+(*(uint32_t*)buf);
            }

            p = p->next;
        }
        while (p != slab);
    }
}

/***
** @param type to be converted
** @param paddr to be converted
** @return converted address
** Converts only base addresses of objects
***/
uint8_t* conv64(size_t type, uint8_t* paddr)
{
    assert(eqtype(type, paddr));
    uint8_t* mem32 = memory+((paddr-memory)/PAGE_SZ)*PAGE_SZ;
    slab_t* slab32 = (slab_t*)(mem32+PAGE_SZ-sizeof(slab_t));
    assert(!((paddr-mem32)%ncps[type].size));
    uint8_t* pmem = slab32->loc;
    uint8_t* mem64 = memory+((pmem-memory)/PAGE_SZ)*PAGE_SZ;
    slab_t* slab64 = (slab_t*)(mem64+PAGE_SZ-sizeof(slab_t));
    assert(!((pmem-mem64)%cps[type].size));

    size_t idx32 = (paddr-mem32)/(ncps[type].size);
    size_t rem64 = cps[type].slab_maxbuf-(pmem-mem64)/cps[type].size;

    // Search through the list O(1)
    while (idx32>=rem64)
    {
        idx32 -= rem64;
        pmem = slab64->loc;
        mem64 = memory+((pmem-memory)/PAGE_SZ)*PAGE_SZ;
        slab64 = (slab_t*)(mem64+PAGE_SZ-sizeof(slab_t));
        rem64 = cps[type].slab_maxbuf;
    }

    return pmem+idx32*cps[type].size;
}

bool eqtype(size_t type, uint8_t* paddr)
{
    if ((paddr-memory) == MEM_SZ64)
    {
        return false;
    }

    uint8_t* mem = memory+((paddr-memory)/PAGE_SZ)*PAGE_SZ;
    slab_t* slab = (slab_t*)(mem+PAGE_SZ-sizeof(slab_t));
    return (slab->type) == type;
}