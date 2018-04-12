Value* runtime_print(Ir *ir, ValueArray args) {
	Value *v;
	for_array(args, v) {
		if (v->kind == VALUE_STRING) {
			printf("%.*s", (int)v->string.str.len, v->string.str.str);
		}
		else if (v->kind == VALUE_NUMBER) {
			printf("%f", v->number.value);
		}
		else if (v->kind == VALUE_NULL) {
			printf("(null)");
		}
	}
	printf("\n");

	return null_value;
}

Value* runtime_msgbox(Ir *ir, ValueArray args) {
#ifdef _WIN32
#include <windows.h>
	if (args.size == 1) {
		Value *arg = args.data[0];
		if (arg->kind != VALUE_STRING) return null_value;
		MessageBoxA(0, arg->string.str.str, 0, MB_OK);
	}
	else if (args.size == 2) {
		Value *arg1 = args.data[0];
		Value *arg2 = args.data[1];
		if (arg1->kind != VALUE_STRING || arg2->kind != VALUE_STRING) return null_value;
		MessageBoxA(0, arg1->string.str.str, arg2->string.str.str, MB_OK);
	}
	return null_value;
#else
	return runtime_print(ir, args);
#endif
}

Value* make_native_function(Ir *ir, Value* (*func)(Ir *ir, ValueArray args)) {
	Value *v = alloc_value(ir);
	v->kind = VALUE_FUNCTION;
	v->func.kind = FUNCTION_NATIVE;
	v->func.native.function = func;
	return v;
}

void add_globals(Ir *ir) {
	scope_add(ir, ir->global_scope, string("print"), make_native_function(ir, runtime_print));
	scope_add(ir, ir->global_scope, string("msgbox"), make_native_function(ir, runtime_msgbox));
}