#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>

void mem_init();
void swtch();
void reloc(size_t type);
void objcpy(size_t type);
void freebuf(size_t type);
uint8_t* conv64(size_t type, uint8_t* paddr);
bool eqtype(size_t type, uint8_t* paddr);

#endif