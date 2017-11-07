#include "mode32.h"

void prnt_grph();

/***
** Constructs a point to graph to help with relocating memory
***/
void graph_init()
{
    num_nodes = num_prnts+1;
    graph = (vnode_t<size_t>**)malloc(sizeof(vnode_t<size_t>*)*num_nodes);

    // Initialize empty graph
    for (size_t i = 0; i < num_nodes; i++)
    {
        graph[i] = NULL;
    }

    // Add edges from last node
    for (size_t i = 0; i < num_prnts; i++)
    {
        graph[num_prnts] = vnode_distinct_insert<size_t>(graph[num_prnts], i);
    }

    // Add edges given the fields
    for (size_t i = 0; i < num_prnts; i++)
    {
        vnode_t<field_t>* lvnode = prnts[i].fields;

        // Traverse all fields
        while (lvnode)
        {
            field_t& lfield = lvnode->val;
            size_t lnum_indirs = lfield.num_indirs;

            // Add edge for a pointer
            if (lnum_indirs)
            {
                size_t ltype = (lnum_indirs>1) ? num_prnts : (lfield.type);
                graph[i] = vnode_distinct_insert<size_t>(graph[i], ltype);
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
    for (size_t i = 0; i < num_nodes; i++)
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
