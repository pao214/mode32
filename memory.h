#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>

#define MEM_SZ64 (1 << 20)
#define MEM_SZ32 (1 << 20)
#define PAGE_SZ (1 << 12)

#define MAX_LVL 8
#define PG_LVL 12
#define MEM_LVL 20

void mem_init();
void swtch();
void reloc(uint32_t type);
void objcpy(uint32_t type);
void freebuf(uint32_t type);
uint8_t *conv64(uint8_t *paddr);

#endif