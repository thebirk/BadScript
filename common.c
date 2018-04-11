typedef struct {
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

#define for_array    (_arr, _val) for(size_t it_index = 0; (_val) =  (_arr).data[it_index], it_index < (_arr).size ; it_index++)
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
