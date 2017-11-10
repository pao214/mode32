#ifndef BUDDY_H
#define BUDDY_H

#include <cstdint>
#include <cstddef>

inline bool __ispow2(uint64_t x);
inline uint64_t __nextpow2(uint64_t x);
inline uint64_t __idx_off(uint64_t idx, uint8_t lvl);
inline void __mark_par(uint64_t idx);
uint8_t *buddy_alloc(uint64_t s);
void combine(uint64_t idx);
void buddy_free(uint8_t *ptr);
uint64_t buddy_size(uint64_t off);
void buddy_init();

#endif