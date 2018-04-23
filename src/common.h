#ifndef BS_COMMON_H
#define BS_COMMON_H

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

// Be careful of double evals
#define Max(a, b) ((a) > (b) ? (a) : (b))

typedef struct String {
	char *str;
	size_t len;
} String;

#define string(_str) (String){_str, sizeof(_str)/sizeof(_str[0])-1}

String make_string_slow(char *c_str);
String make_empty_string_len(size_t len);
String make_string_slow_len(char *c_str, size_t len);
String make_string_copy(String s);

bool strings_match(String a, String b);

#define Array(_type) struct { \
	_type* data; \
	size_t size; \
	size_t cap; \
}
typedef Array(String) StringArray;

// Inclusive [_start, _end]
#define fori(_it, _start, _end) for(int _it = (_start); _it <= (_end); _it++)
// Exclusive [_start, _end}
#define fore(_it, _start, _end) for(int _it = (_start); _it < (_end); _it++)
// Exclusive with non-local variable
#define forev(_it, _start, _end) for((_it) = (_start); (_it) < (_end); (_it)++)

#define for_array(_arr, _val)     for(size_t it_index = 0; (_val) =  (_arr).data[it_index], it_index < (_arr).size ; it_index++)
#define for_array_ref(_arr, _val) for(size_t it_index = 0; (_val) = &(_arr).data[it_index], it_index < (_arr).size ; it_index++)

#define array_init(_arr, _cap) do { \
	assert((_arr).data == 0 && ("Called array_init with an array that is not zero!")); \
	assert((_cap) >= 0 && ("Called array_init with cap less than zero!")); \
	if((_cap) == 0) break; \
	(_arr).cap = (_cap); \
	(_arr).size = 0; \
	(_arr).data = malloc((_arr).cap * sizeof(*((_arr).data))); \
} while(0)

#define array_free(_arr) do { \
	free((_arr).data); \
	(_arr).data = 0; \
	(_arr).size = 0; \
	(_arr).cap = 0; \
} while(0)

#define array_clear(_arr) do { \
	(_arr).size = 0; \
} while(0)

#define array_add(_arr, _el) do { \
	(_arr).size++; \
	if((_arr).size <= (_arr).cap) { \
		(_arr).data[(_arr).size-1] = (_el); \
	} else { \
		if((_arr).cap >= 1024) { \
			(_arr).cap += 128; \
		} else { \
			if((_arr).cap == 0) { \
				(_arr).cap = 2; \
			} else { \
				(_arr).cap *= 2; \
			} \
		} \
		(_arr).data = realloc((_arr).data, (_arr).cap * sizeof(*((_arr).data))); \
		(_arr).data[(_arr).size-1] = (_el); \
	} \
} while(0)

typedef struct MapEntry {
	void *val;
	uint64_t hash;
} MapEntry;

// Taken from - https://github.com/pervognsen/bitwise/blob/master/ion/common.c
// This is a mess, I have no idea if this is even a remotely good map now.
typedef struct Map {
	MapEntry *entries;
	size_t len;
	size_t cap;
} Map;

uint64_t hash_bytes(const char *buf, size_t len);

uint64_t hash_uint64(uint64_t v);
uint64_t hash_ptr(void *ptr);
void* map_get(Map *map, uint64_t hash);

void map_put_hash(Map *map, uint64_t hash, void *val);
void map_grow(Map *map, size_t new_cap);
void map_put_hash(Map *map, uint64_t hash, void *val);
void map_put_string(Map *map, String str, void *val);

void* map_get_string(Map *map, String str);
void map_free(Map *map);

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

Bucket* __pool_make_bucket(Pool *pool);

void pool_init(Pool *pool, size_t element_size, size_t bucket_size);
void* pool_alloc(Pool *pool);
void pool_release(Pool *pool, void *ptr);

int bs_getch();

#endif /* BS_COMMON_H */