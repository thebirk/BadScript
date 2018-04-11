typedef enum ValueKind {
	VALUE_NULL = 0,
	VALUE_NUMBER,
	VALUE_STRING,
	VALUE_TABLE,
	VALUE_FUNCTION,
} ValueKind;

typedef enum FunctionKind {
	FUNCTION_NORMAL,
	FUNCTION_NATIVE,
	FUNCTION_FFI,
} FunctionKind;

typedef struct Value Value;
typedef struct Function {
	FunctionKind kind;
	union {
		struct {
			Array(Value*) args;
		} normal;
		struct {
			Value* (*function)(Array(Value*) args);
		} native;
		struct {
			void *temp;
		} ffi;
	};
} Function;

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
		Function func;
	};
};

Value *null_value = &(Value) { .kind = VALUE_NULL };

Value* call_function(Value *func_value, Array(Value*) args) {
	assert(func_value->kind == VALUE_FUNCTION);
	
	Function func = func_value->func;
	switch (func.kind) {
	case FUNCTION_NORMAL: {
		// Eval all stmts
		assert(!"Not implemented");
	} break;
	case FUNCTION_NATIVE: {
		assert(func.native.function);
		//return (*func.native.function)(args);
	} break;
	case FUNCTION_FFI: {
		assert(!"Not implemented");
	} break;
	}

	return null_value;
}