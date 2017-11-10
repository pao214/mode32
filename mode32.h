#ifndef MODE32_H
#define MODE32_H

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>
#include "vnode.h"
#include "slab.h"
#include "buddy.h"
#include "memory.h"

// Blueprint of structure
extern uint32_t num_prnts;
extern blprnt_t* prnts;

// Point to graph
extern vnode_t<uint32_t>** graph;

// Memory mapped
extern uint8_t* memory;

// Store pointers on stack
extern uint32_t num_addrs;
extern uint8_t** addrs;

// Cache pointer
extern cache_t* cps;
extern bool swtchd;

void prnts_init();
void graph_init();
void usr_init();
void execute32();
void execute64();

#endif
