#include "x_spi.h"
#include "n_mem.h"
#include "x_texcache.h"

/****************************************************************************/
/**                                                                        **/
/**                       TYPEDEFS AND STRUCTURES                          **/
/**                                                                        **/
/****************************************************************************/

//Might need to increase this  
#define HASH_TABLE_SIZE 16

// Cache entry structure
typedef struct texcache_entry {
    uintptr_t key;              // Unique identifier 
    void *data;             // Pointer to the cached data (allocated dynamically)
    size_t size;            // Size in bytes of the cached data
    struct texcache_entry *next;  // Next entry in hash bucket (separate chaining)
} texcache_entry_t;

// The overall cache structure:
typedef struct {
    texcache_entry_t *buckets[HASH_TABLE_SIZE];  // Hash table buckets
    size_t cache_size;      // Maximum allowed cache size (bytes)
    size_t used;            // Current bytes used in the cache
} texcache_t;

/****************************************************************************/
/**                                                                        **/
/*                            GLOBAL VARIABLES                              */
/**                                                                        **/
/****************************************************************************/

texcache_t my_texcache;

/****************************************************************************/
/**                                                                        **/
/*                      PROTOTYPES OF LOCAL FUNCTIONS                       */
/**                                                                        **/
/****************************************************************************/

static unsigned int hash_function_addr(uintptr_t addr);

// Lookup a cache entry by key.
// If found return the data.
static void *cache_get(texcache_t *cache, uintptr_t addr); 

// Insert a new entry into the cache.
// On success, the entry is added to the hash table 
static int cache_put(texcache_t *cache, uintptr_t addr, void *data, size_t size); 

/****************************************************************************/
/**                                                                        **/
/**                          EXPORTED FUNCTIONS                            **/
/**                                                                        **/
/****************************************************************************/

// Initialize the cache.
void texcache_init(size_t cache_size) {
    memset(my_texcache.buckets, 0, sizeof(my_texcache.buckets));
    my_texcache.cache_size = cache_size;
    my_texcache.used = 0;
}

// Free all cache entries.
void texcache_free() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        texcache_entry_t *entry = my_texcache.buckets[i];
        while (entry) {
            texcache_entry_t *next = entry->next;
            N_free(entry->data, entry->size);
            N_free(entry, sizeof(texcache_entry_t));
            entry = next;
        }
        my_texcache.buckets[i] = NULL;
    }
    my_texcache.used = 0;
}

void *X_texcache_read(uint32_t flash_addr) {
    void *cached_data = cache_get(&my_texcache, flash_addr);
    return cached_data;
}

bool X_texcache_write(uint32_t flash_addr, uint32_t len) {
    //Check if data is already in cache 
    void *cached_data = cache_get(&my_texcache, flash_addr);
    if (cached_data != NULL)
        return true; 

    // If not enough space, just skip caching
    if (len + sizeof(texcache_entry_t) > my_texcache.cache_size || len == 0 || my_texcache.used + len + sizeof(texcache_entry_t)> my_texcache.cache_size)
        return false;

    void *buffer = N_malloc(len);
    if (!buffer)
        return false;

    if (len % 4 == 0)
        X_spi_read(flash_addr, buffer, len / 4);
    else {
        uint32_t size = (len + 3) / 4;
        uint32_t raw_buffer[size];
        X_spi_read(flash_addr, raw_buffer, size);
        memcpy(buffer, raw_buffer, len);
    }

    if (cache_put(&my_texcache, flash_addr, buffer, len) != 0) {
        N_free(buffer, len);
        return false;
    }
    return true;
}

/****************************************************************************/
/**                                                                        **/
/*                            LOCAL FUNCTIONS                               */
/**                                                                        **/
/****************************************************************************/

static unsigned int hash_function_addr(uintptr_t addr) {
    // Simple hashing for pointer/address
    return (addr >> 4) ^ (addr >> 12);
}

static void *cache_get(texcache_t *cache, uintptr_t addr) {
    unsigned int hash = hash_function_addr(addr) % HASH_TABLE_SIZE;
    texcache_entry_t *entry = cache->buckets[hash]; 
    while (entry) {
        if (entry->key == addr) {
            return entry->data;
        }
        entry = entry->next;
    }
    return NULL;
}

static int cache_put(texcache_t *cache, uintptr_t addr, void *data, size_t size) {
    if (cache->used + size > cache->cache_size)
        return -1; 

    unsigned int hash = hash_function_addr(addr) % HASH_TABLE_SIZE;
    texcache_entry_t *entry = N_malloc(sizeof(texcache_entry_t));
    if (!entry)
        return -1;

    entry->key = addr;
    entry->data = data;
    entry->size = size;
    entry->next = cache->buckets[hash];
    cache->buckets[hash] = entry;

    cache->used += size + sizeof(texcache_entry_t);
    return 0;
}



















