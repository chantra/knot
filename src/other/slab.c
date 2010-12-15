#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "slab.h"

/*
 * Custom caches LUT.
 */
#define SLAB_CACHE_LUT_SIZE 1024
static const unsigned char SLAB_CACHE_LUT[SLAB_CACHE_LUT_SIZE] = {
        [24]  = SLAB_GP_COUNT + 1,
        [800] = SLAB_GP_COUNT + 2
};

/*
 * Find the next highest power of 2.
 */
static inline unsigned get_next_pow2(unsigned v)
{
	// Next highest power of 2
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

/*
 * Return binary logarithm of a number, which is a power of 2.
 */
static inline unsigned fastlog2(unsigned v)
{
	// Binary logarithm.
	// Works well when we know size is a power of 2
	register unsigned int r = (v & 0xAAAAAAAA) != 0;
	r |= ((v & 0xFFFF0000) != 0) << 4;
	r |= ((v & 0xFF00FF00) != 0) << 3;
	r |= ((v & 0xF0F0F0F0) != 0) << 2;
	r |= ((v & 0xCCCCCCCC) != 0) << 1;
	return r;
}

/*
 * Fast hashing function.
 * Finds the next highest power of 2 and returns binary logarithm.
 */
static unsigned slab_cache_id(unsigned size)
{
	// Assert cache id is at least SLAB_MIN_BUFLEN
	if(size < SLAB_MIN_BUFLEN)
		size = SLAB_MIN_BUFLEN;

	// Check LUT
	unsigned id = 0;
	if ((size < SLAB_CACHE_LUT_SIZE) && (id = SLAB_CACHE_LUT[size])) {
		fprintf(stderr, "%s: LUT hit %u has cache_id=%u\n",
		        __func__, size, id);
		return id;
	}

	// Next highest power of 2
	size = get_next_pow2(size);

	// Compute binary logarithm
	id = fastlog2(size);

	// Shift cacheid of SLAB_MIN_BUFLEN to 0
	id -= SLAB_EXP_OFFSET;
	return id;
}

/*
 * Initializers.
 */
static size_t SLAB_SIZE = 0;

void __attribute__ ((constructor)) slab_init()
{
	SLAB_SIZE = sysconf(_SC_PAGESIZE);
}

void __attribute__ ((destructor)) slab_deinit()
{

}

/*
 * Cache helper functions.
 */
static inline int slab_cache_lock(slab_cache_t* cache)
{
	return pthread_spin_lock(&cache->lock);
}

static inline int slab_cache_unlock(slab_cache_t* cache)
{
	return pthread_spin_unlock(&cache->lock);
}

static inline int slab_cache_free_slabs(slab_t* slab)
{
	int count = 0;
	while (slab) {
		slab_t* next = slab->next;
		slab_destroy(&slab);
		slab = next;
		++count;
	}
	return count;
}

/*
 * Slab helper functions.
 */
static inline void slab_list_remove(slab_t* slab)
{
	// Disconnect from list
	if (slab->prev) {
		slab->prev->next = slab->next;
	}
	if(slab->next) {
		slab->next->prev = slab->prev;
	}

	// Disconnect from cache
	slab_cache_t* cache = slab->cache;
	slab_cache_lock(cache);
	{
		if (cache->slabs_empty == slab) {
			fprintf(stderr, "%s: deleted %p from slabs_empty\n",
			        __func__, slab);
			cache->slabs_empty = slab->next;
		}
		if (cache->slabs_partial == slab) {
			fprintf(stderr, "%s: deleted %p from slabs_partial\n",
			        __func__, slab);
			cache->slabs_partial = slab->next;
		}
		if (cache->slabs_full == slab) {
			fprintf(stderr, "%s: deleted %p from slabs_full\n",
			        __func__, slab);
			cache->slabs_full = slab->next;
		}
	}
	slab_cache_unlock(cache);
}
static inline void slab_list_insert(slab_t** list, slab_t* item)
{
	// If list exists, push to the top
	slab_cache_lock(item->cache);
	item->prev = 0;
	item->next = *list;
	if (*list) {
		(*list)->prev = item;
	}

	*list = item;
	slab_cache_unlock(item->cache);

	const char* ln = "slabs_empty";
	if(*list == item->cache->slabs_partial)
		ln = "slabs_partial";
	if(*list == item->cache->slabs_full)
		ln = "slabs_full";
	fprintf(stderr, "%s: inserted %p to %s (L:%p R:%p)\n",
	        __func__, item, ln, item->prev, item->next);
}
static inline void slab_list_move(slab_t** target, slab_t* slab)
{
	slab_list_remove(slab);
	slab_list_insert(target, slab);
}

/*
 * API functions.
 */

slab_t* slab_create(slab_cache_t* cache)
{
	const size_t size = SLAB_SIZE;

	slab_t* slab = 0;
	if (posix_memalign((void**) &slab, size, size) < 0) {
		fprintf(stderr, "%s: failed to allocate aligned memory block\n",
		        __func__);
		return 0;
	}

	/* Initialize slab. */
	slab->cache = cache;
	slab_list_insert(&cache->slabs_empty, slab);

	/* Ensure the item size can hold at least a size of ptr. */
	size_t item_size = cache->bufsize;
	if (item_size < SLAB_MIN_BUFLEN) {
		item_size = SLAB_MIN_BUFLEN;
	}

	/* Ensure at least some space for coloring */
	size_t data_size = size - sizeof(slab_t);
	size_t free_space = (data_size % item_size);
	if(free_space < 64) {
		free_space = 64;
	}

	unsigned short color = __sync_fetch_and_add(&cache->color, 1);
	color = (1 << color) % free_space;
	color = color - ((color + sizeof(slab_t)) % sizeof(void*));

	/* Calculate useable data size */
	data_size -= color;
	slab->bufs_count = data_size / item_size;
	slab->bufs_free = slab->bufs_count;

	// Save first item as next free
	slab->base = (char*)slab + sizeof(slab_t) + color;
	slab->head = (void**)slab->base;

	// Create freelist
	char* item = (char*)slab->head;
	for(unsigned i = 0; i < slab->bufs_count - 1; ++i) {
		*((void**)item) = item + item_size;
		item += item_size;
	}
	*((void**)item) = (void*)0;

	// Ensure the last item has a NULL next
	fprintf(stderr, "%s: created slab (%p, %p) (%u B)\n",
	        __func__, slab, slab + size, (unsigned)size);
	/*fprintf(stderr, "%s: parent = %p, next slab = %p\n",
	        __func__, cache, slab->next);
	fprintf(stderr, "%s: item_size = %u\n",
	        __func__, (unsigned)item_size);
	fprintf(stderr, "%s: color = %hu\n",
	        __func__, color);
	fprintf(stderr, "%s: data_size = %u\n",
	        __func__, (unsigned)data_size);
	fprintf(stderr, "%s: bufs_count = %u\n",
	        __func__, slab->bufs_count);
	fprintf(stderr, "%s: return = %p\n",
	        __func__, slab);*/
	return slab;
}

void slab_destroy(slab_t** slab)
{
	// Disconnect from the list
	slab_list_remove(*slab);

	// Free slab
	free(*slab);
	fprintf(stderr, "%s: deleted slab %p\n",
	        __func__, *slab);
	*slab = 0;
}

void* slab_alloc(slab_t* slab)
{
	// Fetch first free item
	void** item =  slab->head;
	if (item == 0) {
		return 0;
	}

	slab->head = (void**)*item;
	--slab->bufs_free;

	// Move to partial?
	if (slab->bufs_free == slab->bufs_count - 1) {
		slab_list_move(&slab->cache->slabs_partial, slab);
	} else if (slab->bufs_free == 0){
		slab_list_move(&slab->cache->slabs_full, slab);
	}

	return item;
}

void slab_free(void* ptr)
{
	// Get slab start address
	slab_t* slab = (slab_t*)((char*)ptr - ((uint64_t)ptr % SLAB_SIZE));

	// Return buf to slab
	*((void**)ptr) = slab->head;
	slab->head = (void**)ptr;
	++slab->bufs_free;

	// Return to partial
	if(slab->bufs_free == 1) {
		slab_list_move(&slab->cache->slabs_partial, slab);
	} else if(slab->bufs_free == slab->bufs_count) {
		slab_list_move(&slab->cache->slabs_empty, slab);
	}
}

slab_cache_t* slab_cache_create(size_t bufsize)
{
	fprintf(stderr, "%s: created cache of size %u\n",
	        __func__, (unsigned)bufsize);
	slab_cache_t* cache = malloc(sizeof(slab_cache_t));
	cache->bufsize = bufsize;
	cache->slabs_empty = cache->slabs_partial = cache->slabs_full = 0;
	cache->color = 0;
	pthread_spin_init(&cache->lock, PTHREAD_PROCESS_SHARED);
	return cache;
}

void slab_cache_destroy(slab_cache_t** cache) {

	// Free slabs
	unsigned empty = slab_cache_free_slabs((*cache)->slabs_empty);
	unsigned partial = slab_cache_free_slabs((*cache)->slabs_partial);
	unsigned full = slab_cache_free_slabs((*cache)->slabs_full);
	fprintf(stderr, "%s: %u empty, %u partial, %u full caches\n",
	        __func__, empty, partial, full);

	// Destroy synchronisation
	pthread_spin_destroy(&(*cache)->lock);

	// Free cache
	free(*cache);
	*cache = 0;
}

void* slab_cache_alloc(slab_cache_t* cache)
{
	slab_t* slab = cache->slabs_partial;
	if(cache->slabs_partial) {
		slab = cache->slabs_partial;
	} else {
		if(cache->slabs_empty) {
			slab = cache->slabs_empty;
		} else {
			slab = slab_create(cache);
		}
	}

	return slab_alloc(slab);
}

int slab_cache_reap(slab_cache_t* cache)
{
	// For now, just free empty slabs
	return slab_cache_free_slabs(cache->slabs_empty);
}

slab_alloc_t* slab_alloc_create()
{
	slab_alloc_t* alloc = malloc(sizeof(slab_alloc_t));
	memset(alloc, 0, sizeof(slab_alloc_t));
	return alloc;
}

void slab_alloc_destroy(slab_alloc_t** alloc)
{
	// Destroy all caches
	for (unsigned i = 0; i < SLAB_CACHE_COUNT; ++i) {
		if ((*alloc)->caches[i] != 0) {
			slab_cache_destroy(&(*alloc)->caches[i]);
		}
	}

	// Free allocator
	free(*alloc);
	*alloc = 0;
}

void* slab_alloc_alloc(slab_alloc_t* alloc, size_t size)
{
	// TODO: Work around this
	if (size > SLAB_SIZE * 0.5) {
		return 0;
	}

	// Get cache id from size
	unsigned cache_id = slab_cache_id(size);

	// Check if associated cache exists
	if (alloc->caches[cache_id] == 0) {

		// Assert minimum cache size
		if (size < SLAB_MIN_BUFLEN) {
			size = SLAB_MIN_BUFLEN;
		}

		// Calculate cache bufsize
		size_t bufsize = size;
		if (cache_id < SLAB_GP_COUNT) {
			bufsize = get_next_pow2(size);
		}

		// Create cache
		fprintf(stderr, "%s: creating cache of %uB for size %uB (id=%u)\n",
		        __func__, (unsigned)bufsize, (unsigned)size, cache_id);
		alloc->caches[cache_id] = slab_cache_create(bufsize);
	}

	// Allocate from cache
	void *ret = slab_cache_alloc(alloc->caches[cache_id]);
	if (ret == 0) {
		fprintf(stderr, "%s: returned NULL for size=%u,cacheid=%u\n",
		        __func__, (unsigned)size, (unsigned)cache_id);
	}
	return ret;
}
