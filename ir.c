typedef enum {
	VALUE_NULL = 0,
	VALUE_NUMBER,
	VALUE_STRING,
	VALUE_TABLE,
	VALUE_FUNCTION,
} ValueKind;

typedef struct Value Value;
struct Value {
	ValueKind kind;
	union {
		struct {
			double value;
		} number;
		struct {
			String str;
		} string;
		struct {
			void *temp;
		} table;
		struct {
			void *temp;
		} func;
	};
};

Value *null_value = &(Value) { .kind = VALUE_NULL };

Value* call_function(Value *func, Array(Value*) args) {
	assert(func->kind == VALUE_FUNCTION);
	// ...
	return null_value;
}