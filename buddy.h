#ifndef BUDDY_H
#define BUDDY_H

#include "memory.h"

/**
 * TODO: Track maximum memory that can be allocated in a subtree
 * @const NODE_UNUSED node free for alllocation
 * @const NODE_USED node used for allocation
 * @const NODE_SPLIT not every node in subtree allocated
 * @const NODE_FULL no free node in subtree
**/
#define NODE_UNUSED 0
#define NODE_USED 1
#define NODE_SPLIT 2
#define NODE_FULL 3

extern uint8_t* btr;

uint8_t *buddy_alloc(uint64_t s);
void combine(uint64_t idx);
void buddy_free(uint8_t *ptr);
uint64_t buddy_size(uint8_t *ptr);
void buddy_init();
inline bool __ispow2(uint64_t x);
inline uint64_t __nextpow2(uint64_t x);
inline uint64_t __idx_off(uint64_t idx, uint8_t lvl);
inline void __mark_par(uint64_t idx);

#endif