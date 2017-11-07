#include "mode32.h"

size_t num_prnts;
blprnt_t* prnts;

size_t num_nodes;
vnode_t<size_t>** graph;

uint8_t* memory;

size_t num_addrs;
uint8_t** addrs;

cache_t* cps;
bool swtchd;

int main()
{
    clock_t t;
    t = clock();
    prnts_init();
    graph_init();
    mem_init();
    usr_init();
    fprintf(stderr, "init took %ld cycles\n", clock()-t);
    t = clock();
    execute32();
    fprintf(stderr, "execute32 took %ld cycles\n", clock()-t);
    t = clock();
    swtch();
    fprintf(stderr, "swtch took %ld cycles\n", clock()-t);
    t = clock();
    execute64();
    fprintf(stderr, "execute64 took %ld cycles\n", clock()-t);
    return 0;
}
