#include "mode32.h"

void prnt_grph();

/***
** Constructs a reverse point to graph to help with relocating memory
***/
void graph_init()
{
    graph = (vnode_t<uint32_t>**)malloc(sizeof(vnode_t<uint32_t>*)*num_prnts);

    // Initialize empty graph
    for (uint32_t i = 0; i < num_prnts; i++)
    {
        graph[i] = NULL;
    }

    // Add edges given the fields
    for (uint32_t i = 0; i < num_prnts; i++)
    {
        vnode_t<field_t>* lvnode = prnts[i].fields;

        // Traverse all fields
        while (lvnode)
        {
            field_t& lfield = lvnode->val;
            uint64_t lprim_sz = lfield.prim_sz;

            // Add edge for a pointer
            if (!lprim_sz)
            {
                uint64_t ltype = lfield.type;
                graph[ltype] = vnode_distinct_insert<uint32_t>(graph[ltype], i);
            }

            lvnode = lvnode->next;
        }
    }

    prnt_grph();
}

/***
** Debug function that prints graph as a sanity check
***/
void prnt_grph()
{
    for (uint32_t i = 0; i < num_prnts; i++)
    {
        fprintf(stderr, "%u:", i);

        vnode_t<uint32_t>* lvnode = graph[i];

        while (lvnode && (lvnode->next))
        {
            fprintf(stderr, "%u->", lvnode->val);
            lvnode = lvnode->next;
        }

        if (lvnode)
        {
            fprintf(stderr, "%u", lvnode->val);
        }

        fprintf(stderr, "\n");
    }
}
