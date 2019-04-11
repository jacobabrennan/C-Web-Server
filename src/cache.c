#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"
#include "cache.h"

/**
 * Allocate a cache entry
 */
struct cache_entry *alloc_entry(char *path, char *content_type, void *content, int content_length)
{
    struct cache_entry *entry_new = malloc(sizeof(struct cache_entry));
    entry_new->path = strdup(path);
    entry_new->content_type = strdup(content_type);
    entry_new->content_length = content_length;
    entry_new->content = malloc(content_length);
    entry_new->next = NULL;
    entry_new->prev = NULL;
    memcpy(entry_new->content, content, content_length);
    return entry_new;
}

/**
 * Deallocate a cache entry
 */
void free_entry(struct cache_entry *entry)
{
    free(entry->content_type);
    free(entry->path);
    free(entry->content);
    free(entry);
}

/**
 * Insert a cache entry at the head of the linked list
 */
void dllist_insert_head(struct cache *cache, struct cache_entry *ce)
{
    // Insert at the head of the list
    if (cache->head == NULL) {
        cache->head = cache->tail = ce;
        ce->prev = ce->next = NULL;
    } else {
        cache->head->prev = ce;
        ce->next = cache->head;
        ce->prev = NULL;
        cache->head = ce;
    }
}

/**
 * Move a cache entry to the head of the list
 */
void dllist_move_to_head(struct cache *cache, struct cache_entry *ce)
{
    if (ce != cache->head) {
        if (ce == cache->tail) {
            // We're the tail
            cache->tail = ce->prev;
            cache->tail->next = NULL;

        } else {
            // We're neither the head nor the tail
            ce->prev->next = ce->next;
            ce->next->prev = ce->prev;
        }

        ce->next = cache->head;
        cache->head->prev = ce;
        ce->prev = NULL;
        cache->head = ce;
    }
}


/**
 * Removes the tail from the list and returns it
 * 
 * NOTE: does not deallocate the tail
 */
struct cache_entry *dllist_remove_tail(struct cache *cache)
{
    struct cache_entry *oldtail = cache->tail;

    cache->tail = oldtail->prev;
    cache->tail->next = NULL;

    cache->cur_size--;

    return oldtail;
}

/**
 * Create a new cache
 * 
 * max_size: maximum number of entries in the cache
 * hashsize: hashtable size (0 for default)
 */
struct cache *cache_create(int max_size, int hashsize)
{
    struct cache *cache_new = malloc(sizeof(struct cache));
    cache_new->index = hashtable_create(hashsize, NULL);
    cache_new->head = NULL;
    cache_new->tail = NULL;
    cache_new->max_size = max_size;
    cache_new->cur_size = 0;
}

void cache_free(struct cache *cache)
{
    struct cache_entry *cur_entry = cache->head;

    hashtable_destroy(cache->index);

    while (cur_entry != NULL) {
        struct cache_entry *next_entry = cur_entry->next;

        free_entry(cur_entry);

        cur_entry = next_entry;
    }

    free(cache);
}

/**
 * Store an entry in the cache
 *
 * This will also remove the least-recently-used items as necessary.
 * 
 * NOTE: doesn't check for duplicate cache entries
 */
void cache_put(struct cache *cache, char *path, char *content_type, void *content, int content_length)
{
    // Allocate a new cache entry with the passed parameters.
    struct cache_entry *entry_new = alloc_entry(path, content_type, content, content_length);
    // Insert the entry at the head of the doubly-linked list.
    dllist_insert_head(cache, entry_new);
    // Store the entry in the hashtable as well, indexed by the entry's path.
    hashtable_put(cache->index, path, entry_new);
    // Increment the current size of the cache.
    cache->cur_size++;
    // If the cache size is greater than the max size:
    if(cache->cur_size > cache->max_size)
    {
        // Remove the cache entry at the tail of the linked list.
        struct cache_entry *tail_old = dllist_remove_tail(cache);
        // Remove that same entry from the hashtable, using the entry's path and the hashtable_delete function.
        hashtable_delete(cache->index, path);
        // Free the cache entry.
        free_entry(tail_old);
        // Ensure the size counter for the number of entries in the cache is correct.
            // Taken care of by dllist_remove_tail()
    }
}

/**
 * Retrieve an entry from the cache
 */
struct cache_entry *cache_get(struct cache *cache, char *path)
{
    // Attempt to find the cache entry pointer by path in the hash table.
    struct cache_entry *entry_stored = hashtable_get(cache->index, path);
    // If not found, return NULL.
    if(NULL == entry_stored)
    {
        return NULL;
    }
    // Move the cache entry to the head of the doubly-linked list.
    dllist_move_to_head(cache, entry_stored);
    // Return the cache entry pointer.
    return entry_stored;
}
