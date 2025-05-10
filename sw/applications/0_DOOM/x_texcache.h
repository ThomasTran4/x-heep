#ifndef X_TEXCACHE_H
#define X_TEXCACHE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void texcache_init(size_t cache_size); 
void texcache_free(); 

void *X_texcache_read(uint32_t flash_addr); 
bool X_texcache_write(uint32_t flash_addr, uint32_t len); 

#endif // X_TEXCACHE_H