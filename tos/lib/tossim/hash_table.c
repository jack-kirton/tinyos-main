/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1988-2004 Keith Packard and Bart Massey.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors
 * or their institutions shall not be used in advertising or
 * otherwise to promote the sale, use or other dealings in this
 * Software without prior written authorization from the
 * authors.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Keith Packard <keithp@keithp.com>
 */

// from: https://raw.githubusercontent.com/anholt/hash_table/master/hash_table.c

#include <stdlib.h>
#include <string.h>

#include "hash_table.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

/*
 * From Knuth -- a good choice for hash/rehash values is p, p-2 where
 * p and p-2 are both prime.  These tables are sized to have an extra 10%
 * free to avoid exponential performance degradation as the hash table fills
 */

static const uint32_t deleted_key_value = 0;
static const void * const deleted_key = &deleted_key_value;

static const struct {
   uint32_t max_entries, size, rehash;
} hash_sizes[] = {
//    { 2,            5,            3            },
//    { 4,            7,            5            },
//    { 8,            13,           11           },
    { 16,           19,           17           },
    { 32,           43,           41           },
    { 64,           73,           71           },
    { 128,          151,          149          },
    { 256,          283,          281          },
    { 512,          571,          569          },
    { 1024,         1153,         1151         },
    { 2048,         2269,         2267         },
    { 4096,         4519,         4517         },
    { 8192,         9013,         9011         },
    { 16384,        18043,        18041        },
    { 32768,        36109,        36107        },
    { 65536,        72091,        72089        },
    { 131072,       144409,       144407       },
    { 262144,       288361,       288359       },
    { 524288,       576883,       576881       },
    { 1048576,      1153459,      1153457      },
    { 2097152,      2307163,      2307161      },
    { 4194304,      4613893,      4613891      },
    { 8388608,      9227641,      9227639      },
    { 16777216,     18455029,     18455027     },
    { 33554432,     36911011,     36911009     },
    { 67108864,     73819861,     73819859     },
    { 134217728,    147639589,    147639587    },
    { 268435456,    295279081,    295279079    },
    { 536870912,    590559793,    590559791    },
    { 1073741824,   1181116273,   1181116271   },
    { 2147483648ul, 2362232233ul, 2362232231ul }
};

static inline int
entry_is_free(const struct hash_entry *entry)
{
	return entry->key == NULL;
}

static inline int
entry_is_deleted(const struct hash_entry *entry)
{
	return entry->key == deleted_key;
}

static inline int
entry_is_present(const struct hash_entry *entry)
{
	return ((entry->key != NULL) & (entry->key != deleted_key));
}

bool
hash_table_create(
			struct hash_table *ht,
			uint32_t (*hash_function)(const void *key),
		  	int (*key_equals_function)(const void *a, const void *b))
{
	if (ht == NULL)
		return false;

	ht->size_index = 0;
	ht->size = hash_sizes[ht->size_index].size;
	ht->rehash = hash_sizes[ht->size_index].rehash;
	ht->max_entries = hash_sizes[ht->size_index].max_entries;
	ht->hash_function = hash_function;
	ht->key_equals_function = key_equals_function;
	ht->table = (struct hash_entry *)calloc(ht->max_entries, sizeof(*ht->table));
	ht->indexes = (uint16_t *)malloc(ht->size * sizeof(*ht->indexes));
	ht->entries = 0;

	if (ht->table == NULL || ht->indexes == NULL) {
		free(ht->table);
		free(ht->indexes);
		ht->table = NULL;
		ht->indexes = NULL;
		return false;
	}

	memset(ht->indexes, 0xFFFF, ht->size * sizeof(*ht->indexes));

	return true;
}

/**
 * Frees the given hash table.
 *
 * If delete_function is passed, it gets called on each entry present before
 * freeing.
 */
void
hash_table_destroy(struct hash_table *ht,
		   void (*delete_function)(struct hash_entry* entry))
{
	if (!ht)
		return;

	if (delete_function) {
		struct hash_entry *entry;

		hash_table_foreach(ht, entry) {
			delete_function(entry);
		}
	}
	free(ht->table);
	free(ht->indexes);
	memset(ht, 0, sizeof(*ht));
}

/**
 * Finds a hash table entry with the given key.
 *
 * Returns NULL if no entry is found.  Note that the data pointer may be
 * modified by the user.
 */
struct hash_entry *
hash_table_search(struct hash_table *ht, const void *key)
{
	const uint32_t hash = ht->hash_function(key);

	return hash_table_search_pre_hashed(ht, hash, key);
}

/**
 * Finds a hash table entry with the given key and hash of that key.
 *
 * Returns NULL if no entry is found.  Note that the data pointer may be
 * modified by the user.
 */
struct hash_entry *
hash_table_search_pre_hashed(struct hash_table *ht, uint32_t hash,
			     const void *key)
{
	const uint32_t rehash = ht->rehash;
	const uint32_t size = ht->size;

	const uint32_t start_hash_address = hash % size;
	uint32_t hash_address = start_hash_address;

	do {
		const uint16_t index = ht->indexes[hash_address];

		if (index == 0xFFFF)
			return NULL;

		struct hash_entry *entry = ht->table + index;

		/*if (entry_is_free(entry)) {
			return NULL;
		} else*/
		if (entry_is_present(entry) && entry->hash == hash) {
			if (ht->key_equals_function(key, entry->key)) {
				return entry;
			}
		}

		const uint32_t double_hash = 1 + hash % rehash;

		hash_address = (hash_address + double_hash) % size;
	} while (hash_address != start_hash_address);

	return NULL;
}

static void
hash_table_rehash(struct hash_table *ht, uint32_t new_size_index)
{
	if (new_size_index >= ARRAY_SIZE(hash_sizes))
		return;

	const uint32_t new_size = hash_sizes[new_size_index].size;
	const uint32_t max_entries = hash_sizes[new_size_index].max_entries;

	struct hash_entry * const table = (struct hash_entry *)calloc(max_entries, sizeof(*ht->table));
	uint16_t * const indexes = (uint16_t *)malloc(new_size * sizeof(*ht->indexes));
	if (table == NULL || indexes == NULL) {
		free(indexes);
		free(table);
		return;
	}

	memset(indexes, 0xFFFF, new_size * sizeof(*ht->indexes));

	struct hash_table old_ht = *ht;

	ht->table = table;
	ht->indexes = indexes;
	ht->size_index = new_size_index;
	ht->size = new_size;
	ht->rehash = hash_sizes[new_size_index].rehash;
	ht->max_entries = max_entries;
	ht->entries = 0;

	struct hash_entry *entry;
	hash_table_foreach(&old_ht, entry) {
		hash_table_insert_pre_hashed(ht, entry->hash,
					     entry->key, entry->data);
	}

	free(old_ht.table);
	free(old_ht.indexes);
}

/**
 * Inserts the key into the table.
 *
 * Note that insertion may rearrange the table on a resize or rehash,
 * so previously found hash_entries are no longer valid after this function.
 */
struct hash_entry *
hash_table_insert(struct hash_table *ht, const void *key, void *data)
{
	const uint32_t hash = ht->hash_function(key);

	return hash_table_insert_pre_hashed(ht, hash, key, data);
}

/**
 * Inserts the key with the given hash into the table.
 *
 * Note that insertion may rearrange the table on a resize or rehash,
 * so previously found hash_entries are no longer valid after this function.
 */
struct hash_entry *
hash_table_insert_pre_hashed(struct hash_table *ht, uint32_t hash,
			     const void *key, void *data)
{
	uint32_t start_hash_address, hash_address;
	uint16_t index;

	if (ht->entries >= ht->max_entries) {
		hash_table_rehash(ht, ht->size_index + 1);
	}

	const uint32_t rehash = ht->rehash;
	const uint32_t size = ht->size;

	start_hash_address = hash % size;
	hash_address = start_hash_address;
	do {
		index = ht->indexes[hash_address];

		if (index == 0xFFFF) {
			struct hash_entry * const available_entry = ht->table + ht->entries;
			ht->indexes[hash_address] = ht->entries;

			available_entry->hash = hash;
			available_entry->key = key;
			available_entry->data = data;

			ht->entries++;

			return available_entry;
		}

		const uint32_t double_hash = 1 + hash % rehash;

		hash_address = (hash_address + double_hash) % size;
	} while (hash_address != start_hash_address);

	/* We could hit here if a required resize failed. An unchecked-malloc
	 * application could ignore this result.
	 */
	return NULL;
}

/**
 * This function searches for, and removes an entry from the hash table.
 *
 * If the caller has previously found a struct hash_entry pointer,
 * (from calling hash_table_search or remembering it from
 * hash_table_insert), then hash_table_remove_entry can be called
 * instead to avoid an extra search.
 */
void
hash_table_remove(struct hash_table *ht, const void *key)
{
	struct hash_entry * const entry = hash_table_search(ht, key);

	hash_table_remove_entry(ht, entry);
}

/**
 * This function deletes the given hash table entry.
 *
 * Note that deletion doesn't otherwise modify the table, so an iteration over
 * the table deleting entries is safe.
 */
void
hash_table_remove_entry(struct hash_table *ht, struct hash_entry *entry)
{
	if (!entry)
		return;

	entry->key = deleted_key;
}

/**
 * This function is an iterator over the hash table.
 *
 * Pass in NULL for the first entry, as in the start of a for loop.  Note that
 * an iteration over the table is O(table_size) not O(entries).
 */
struct hash_entry *
hash_table_next_entry(struct hash_table *ht, struct hash_entry *entry)
{
	struct hash_entry* const end = ht->table + ht->entries;

	// Start at beginning
	if (entry == NULL)
	{
		entry = ht->table;
	}
	else
	{
		++entry;
	}

	if (entry >= end) {
		return NULL;
	}

	// Skip over deleted entries
	while (entry_is_deleted(entry) && entry < end)
	{
		++entry;
	}

	if (entry_is_present(entry)) {
		return entry;
	}

	return NULL;
}

/**
 * This function is a reverse iterator over the hash table.
 *
 * Pass in NULL for the first entry, as in the start of a for loop.  Note that
 * an iteration over the table is O(table_size) not O(entries).
 */
struct hash_entry *
hash_table_next_entry_reverse(struct hash_table *ht, struct hash_entry *entry)
{
	struct hash_entry* const end = ht->table + ht->entries;

	// Start at end
	if (entry == NULL)
	{
		entry = end - 1;
	}
	else
	{
		--entry;
	}

	if (entry < ht->table || entry >= end) {
		return NULL;
	}

	// Skip over deleted entries
	while (entry_is_deleted(entry) && entry > ht->table)
	{
		--entry;
	}

	if (entry_is_present(entry))
		return entry;

	return NULL;
}
