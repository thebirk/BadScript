typedef struct String {
	char *str;
	size_t len;
} String;

#define string(_str) (String){_str, sizeof(_str)/sizeof(_str[0])-1}

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

bool strings_match(String a, String b) {
	if (a.len != b.len) return false;
	if (a.len == b.len && a.str == b.str) return true;

	for (size_t i = 0; i < a.len; i++) {
		if (a.str[i] != b.str[i]) return false;
	}

	return true;
}

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
typedef struct Map{
	MapEntry *entries;
	size_t len;
	size_t cap;
} Map;

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
	new_cap = max(16, new_cap);
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