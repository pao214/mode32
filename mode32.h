#ifndef MODE32_H
#define MODE32_H

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>
#include "vnode.h"
#include "slab.h"

#define MEM_SZ64 (1 << 20)
#define MEM_SZ32 (1 << 20)

// Blueprint of structure
extern size_t num_prnts;
extern blprnt_t* prnts;

// Point to graph
extern size_t num_nodes;
extern vnode_t<size_t>** graph;

// Memory mapped
extern uint8_t* memory;

// Store pointers on stack
extern size_t num_addrs;
extern uint8_t** addrs;

// Cache pointer
extern cache_t* cps;
extern bool swtchd;

void prnts_init();
void graph_init();
void mem_init();
void usr_init();

void execute32();
void swtch();
void execute64();

bool eqtype(size_t type, uint8_t* paddr);

#endif
