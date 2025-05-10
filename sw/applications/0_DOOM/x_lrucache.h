#ifndef X_LRUCACHE_H
#define X_LRUCACHE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cache_init(size_t cache_size); 
void cache_free(); 
uint8_t get_cache_initialized(); 

void *X_cache_read(uint32_t flash_addr, uint32_t len); 

#endif // X_LRUCACHE_H