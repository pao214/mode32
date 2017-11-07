#include "mode32.h"

void prnt_grph();

/***
** Constructs a reverse point to graph to help with relocating memory
***/
void graph_init()
{
    graph = (vnode_t<size_t>**)malloc(sizeof(vnode_t<size_t>*)*num_prnts);

    // Initialize empty graph
    for (size_t i = 0; i < num_prnts; i++)
    {
        graph[i] = NULL;
    }

    // Add edges given the fields
    for (size_t i = 0; i < num_prnts; i++)
    {
        vnode_t<field_t>* lvnode = prnts[i].fields;

        // Traverse all fields
        while (lvnode)
        {
            field_t& lfield = lvnode->val;
            size_t lprim_sz = lfield.prim_sz;

            // Add edge for a pointer
            if (!lprim_sz)
            {
                size_t ltype = lfield.type;
                graph[ltype] = vnode_distinct_insert<size_t>(graph[ltype], i);
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
    for (size_t i = 0; i < num_prnts; i++)
    {
        fprintf(stderr, "%ld:", i);

        vnode_t<size_t>* lvnode = graph[i];

        while (lvnode && (lvnode->next))
        {
            fprintf(stderr, "%ld->", lvnode->val);
            lvnode = lvnode->next;
        }

        if (lvnode)
        {
            fprintf(stderr, "%ld", lvnode->val);
        }

        fprintf(stderr, "\n");
    }
}
