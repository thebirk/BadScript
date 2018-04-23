#include "common.h"

String make_string_slow(char *c_str) {
	String result = { 0 };

	result.len = strlen(c_str);
	result.str = malloc(result.len+1);
	result.str[result.len] = 0;
	memcpy(result.str, c_str, result.len);

	return result;
}

String make_empty_string_len(size_t len) {
	String result = { 0 };

	result.len = len;
	result.str = malloc(result.len);

	return result;
}

String make_string_slow_len(char *c_str, size_t len) {
	String result = {0};

	result.len = len;
	result.str = malloc(result.len+1);
	result.str[result.len] = 0;
	memcpy(result.str, c_str, result.len);

	return result;
}

String make_string_copy(String s) {
	return make_string_slow_len(s.str, s.len);
}

bool strings_match(String a, String b) {
	if (a.len != b.len) return false;
	if (a.len == b.len && a.str == b.str) return true;

	for (size_t i = 0; i < a.len; i++) {
		if (a.str[i] != b.str[i]) return false;
	}

	return true;
}

uint64_t hash_bytes(const char *buf, size_t len) {
	uint64_t x = 0xcbf29ce484222325;
	for (size_t i = 0; i < len; i++) {
		x ^= buf[i];
		x *= 0x100000001b3;
		x ^= x >> 32;
	}
	return x;
}

uint64_t hash_uint64(uint64_t v) {
	//v *= 0xff51afd7ed558ccd;
	//v ^= v >> 32;
	//return v;
	uint8_t *bytes = (uint8_t*)&v;
	return hash_bytes(bytes, sizeof(uint64_t));
}

uint64_t hash_ptr(void *ptr) {
	return hash_uint64((uintptr_t)ptr);
}

#define IS_POW2(x) (((x) != 0) && ((x) & ((x)-1)) == 0)
void* map_get(Map *map, uint64_t hash) {
	if (map->len == 0) return 0;

	assert(IS_POW2(map->cap));
	assert(map->len < map->cap);

	size_t i = (size_t)hash;
	for (;;) {
		i &= map->cap - 1;
		MapEntry *entry = &map->entries[i];
		if (entry->hash == hash) {
			return entry->val;
		}
		else if (!entry->hash) {
			return 0;
		}
		i++;
	}

	return 0;
}

void map_put_hash(Map *map, uint64_t hash, void *val);
void map_grow(Map *map, size_t new_cap) {
	new_cap = Max(16, new_cap);
	assert(IS_POW2(new_cap));
	Map new_map = {
		.entries = calloc(new_cap, sizeof(MapEntry)),
		.cap = new_cap,
	};

	for (size_t i = 0; i < map->cap; i++) {
		MapEntry *e = &map->entries[i];
		if (e->hash) {
			map_put_hash(&new_map, e->hash, e->val);
		}
	}

	free(map->entries);
	*map = new_map;
}

//TODO: Do string interning and remove c_hashmap completly
void map_put_hash(Map *map, uint64_t hash, void *val) {
	assert(val);
	if (2 * map->len >= map->cap) {
		map_grow(map, 2 * map->cap);
	}

	assert(2 * map->len < map->cap);
	assert(IS_POW2(map->cap));
	size_t i = (size_t)hash;
	for (;;) {
		i &= map->cap - 1;
		MapEntry *e = &map->entries[i];
		if (!e->hash) {
			map->len++;
			e->val = val;
			e->hash = hash;
			return;
		}
		else if (e->hash == hash) {
			e->val = val;
			return;
		}
		i++;
	}
}

void map_put_string(Map *map, String str, void *val) {
	uint64_t hash = hash_bytes(str.str, str.len);
	map_put_hash(map, hash, val);
}

void* map_get_string(Map *map, String str) {
	uint64_t hash = hash_bytes(str.str, str.len);
	return map_get(map, hash);
}

void map_free(Map *map) {
	free(map->entries);
}

typedef struct Bucket Bucket;
struct Bucket {
	void *arena;
	size_t bucket_size;
	size_t bucket_used;
	size_t element_size;
	size_t count;
	Bucket *next;
};

typedef struct Pool {
	Bucket *current_bucket;
	Bucket *free_buckets;
	Bucket *old_buckets;
	size_t element_size;
	size_t bucket_size;
	size_t buckets;
} Pool;

Bucket* __pool_make_bucket(Pool *pool) {
	Bucket *bucket = calloc(1, sizeof(Bucket));

	bucket->element_size = pool->element_size;
	bucket->bucket_size = pool->bucket_size;
	bucket->arena = calloc(1, bucket->bucket_size);
	bucket->bucket_used = 0;
	bucket->count = 0;
	bucket->next = 0;

	pool->buckets++;

	return bucket;
}

void pool_init(Pool *pool, size_t element_size, size_t bucket_size) {
	pool->buckets = 0;
	pool->element_size = element_size + sizeof(Bucket*);
	pool->bucket_size = bucket_size * element_size;
	pool->old_buckets = 0;
	pool->current_bucket = __pool_make_bucket(pool);
}

void* pool_alloc(Pool *pool) {
	if (pool->current_bucket->bucket_used + pool->current_bucket->element_size > pool->current_bucket->bucket_size) {
		Bucket *old = pool->current_bucket;

		if (pool->free_buckets) {
			Bucket *new = pool->free_buckets;
			pool->free_buckets = new->next;
			pool->current_bucket = new;
		}
		else {
			pool->current_bucket = __pool_make_bucket(pool);
		}

		old->next = pool->old_buckets;
		pool->old_buckets = old;
	}
	Bucket **result = (Bucket**)((uint8_t*)(pool->current_bucket->arena) + pool->current_bucket->bucket_used);
	pool->current_bucket->bucket_used += pool->current_bucket->element_size;
	*result = pool->current_bucket;
	pool->current_bucket->count++;
	result++;
	memset(result, 0, pool->current_bucket->element_size - sizeof(Bucket*));
	return result;
}

void pool_release(Pool *pool, void *ptr) {
	Bucket **header = ptr;
	header--;
	Bucket *owner_bucket = *header;
	owner_bucket->count--;

	Bucket **bucket = &pool->old_buckets;
	while (*bucket) {
		if ((*bucket)->count == 0) {
			Bucket *old = *bucket;
			*bucket = old->next;

			old->bucket_used = 0;

			old->next = pool->free_buckets;
			pool->free_buckets = old;

			pool->buckets--;
		}
		else {
			bucket = &(*bucket)->next;
		}
	}
}

/*
typedef struct Bucket Bucket;
struct Bucket {
	void *arena;
	size_t bucket_size;
	size_t bucket_used;
	size_t element_size;
	size_t count;
	Bucket *next;
};

typedef struct Pool {
	Bucket *current_bucket;
	Bucket *old_buckets;
	size_t element_size;
	size_t bucket_size;
	size_t buckets;
} Pool;

Bucket* __pool_make_bucket(Pool *pool) {
	Bucket *bucket = calloc(1, sizeof(Bucket));

	bucket->element_size = pool->element_size;
	bucket->bucket_size = pool->bucket_size;
	bucket->arena = calloc(1, bucket->bucket_size);
	bucket->bucket_used = 0;
	bucket->count = 0;
	bucket->next = 0;

	pool->buckets++;

	return bucket;
}

void pool_init(Pool *pool, size_t element_size, size_t bucket_size) {
	pool->buckets = 0;
	pool->element_size = element_size + sizeof(Bucket*);
	pool->bucket_size = bucket_size * element_size;
	pool->old_buckets = 0;
	pool->current_bucket = __pool_make_bucket(pool);
}

void* pool_alloc(Pool *pool) {
	if (pool->current_bucket->bucket_used + pool->current_bucket->element_size > pool->current_bucket->bucket_size) {
		Bucket *old = pool->current_bucket;
		old->next = pool->old_buckets;
		pool->old_buckets = old;
		pool->current_bucket = __pool_make_bucket(pool);
	}
	Bucket **result = (Bucket**)((uint8_t*)(pool->current_bucket->arena) + pool->current_bucket->bucket_used);
	pool->current_bucket->bucket_used += pool->current_bucket->element_size;
	*result = pool->current_bucket;
	pool->current_bucket->count++;
	result++;
	memset(result, 0, pool->current_bucket->element_size-sizeof(Bucket*));
	return result;
}

void pool_clear_buckets(Pool *pool) {
	//printf("Total buckets before clear: %d\n", (int)pool->buckets);

	Bucket **bucket = &pool->old_buckets;
	while (*bucket) {
		if ((*bucket)->count == 0) {
			Bucket *old = *bucket;
			*bucket = old->next;

			free(old->arena);
			free(old);
			pool->buckets--;
		}
		else {
			bucket = &(*bucket)->next;
		}
	}

	//printf("Total buckets after clear: %d\n", (int)pool->buckets);
}

void pool_release(Pool *pool, void *ptr) {
	Bucket **header = ptr;
	header--;
	Bucket *owner_bucket = *header;
	owner_bucket->count--;


}
*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int bs_getch() {
	DWORD mode, cc;
	HANDLE h = GetStdHandle( STD_INPUT_HANDLE );
	if (h == NULL) {
		return '\n'; // console not found
	}
	GetConsoleMode( h, &mode );
	SetConsoleMode( h, mode & ~(ENABLE_ECHO_INPUT) );
	int c = 0;
	ReadConsoleA( h, &c, 1, &cc, NULL );
	SetConsoleMode( h, mode );
	if (c == '\r') {
		return '\n';
	}
	return c;
}
#elif POSIX
#include <termios.h>
#include <unistd.h>
int bs_getch() {
	//NOTE: Untested, pulled straight from stackoverflow
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}
#else
#error Implement bs_getch for this platform
#endif