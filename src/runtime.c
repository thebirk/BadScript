Value* runtime_print(Ir *ir, ValueArray args);
Value* runtime_input(Ir *ir, ValueArray args) {
	// Prints a prompt if there is an argument for it and return the user input
	if (args.size > 0) {
		runtime_print(ir, args);
	}
#define INPUT_BUFFER_SIZE 4096
	char buffer[INPUT_BUFFER_SIZE];
	size_t offset = 0;
	char c = 0;
	while ((c = getchar())) {
		if (offset == INPUT_BUFFER_SIZE) {
			buffer[offset-1] = 0;
			break;
		} 
		else if (c == '\n') {
			buffer[offset] = 0;
			break;
		}
		else {
			buffer[offset++] = c;
		}
	}
	return make_string_value(ir, make_string_slow(buffer));
}

Value* runtime_input_hidden(Ir *ir, ValueArray args) {
	// Prints a prompt if there is an argument for it and return the user input
	// Does not echo input
	if (args.size > 0) {
		runtime_print(ir, args);
	}
	char buffer[INPUT_BUFFER_SIZE];
	size_t offset = 0;
	char c = 0;
	while ((c = bs_getch())) {
		if (offset == INPUT_BUFFER_SIZE) {
			buffer[offset - 1] = 0;
			break;
		}
		else if (c == '\n') {
			buffer[offset] = 0;
			break;
		}
		else {
			buffer[offset++] = c;
		}
	}
	return make_string_value(ir, make_string_slow(buffer));
}

Value* runtime_type(Ir *ir, ValueArray args) {
	// When we have tables(aka array) if the
	// users passes more than one parameter return an array of strings
	if (args.size > 1) {
		assert(!"We dont have these yet!");
		Value *v;
		for_array(args, v) {

		}
		return null_value;
	}
	else if (args.size == 1) {
		Value *v = args.data[0];
		switch (v->kind)
		{
		case VALUE_NUMBER: return make_string_value(ir, make_string_slow("number"));
		case VALUE_STRING: return make_string_value(ir, make_string_slow("string"));
		case VALUE_NULL:   return make_string_value(ir, make_string_slow("null"));
		case VALUE_TABLE:  return make_string_value(ir, make_string_slow("table"));
		case VALUE_FUNCTION:  return make_string_value(ir, make_string_slow("function"));
		}
		ir_error(ir, "Hey! You found a bug. We have added a new type and have forgetten to add it to runtime_type!");
	}
	else {
		assert(!"We still need an error here"); //TODO: Error
		return null_value;
	}
}

Value* runtime_println(Ir *ir, ValueArray args) {
	runtime_print(ir, args);
	printf("\n");
	return null_value;
}

void print_value(Value *v) {
	if (v->kind == VALUE_STRING) {
		printf("%.*s", (int)v->string.str.len, v->string.str.str);
	}
	else if (v->kind == VALUE_NUMBER) {
		printf("%f", v->number.value);
	}
	else if (v->kind == VALUE_NULL) {
		printf("(null)");
	}
	else if (v->kind == VALUE_TABLE) {
		printf("{}");
	}
}

Value* runtime_print(Ir *ir, ValueArray args) {
	if (args.size > 0) {
		Value *v;
		for_array(args, v) {
			print_value(v);
		}
	}

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

Value* runtime_str2num(Ir *ir, ValueArray args) {
	// When we have table perhabs return an array on multiple args
	assert(args.size == 1); //TODO: Error message
	if (args.size != 1) {
		ir_error(ir, "str2num only takes one argument");
	}
	if (!isstring(args.data[0])) {
		return null_value;
	}
	//double n = strtod(args.data[0]->string.str.str, 0);
	double n = 0.0;
	Value *v = args.data[0];
	int ret = sscanf(v->string.str.str, "%lf", &n);
	if(ret != 1 || ret == EOF) {
		return null_value;
	}
	return make_number_value(ir, n);
}

Value* runtime_num2str(Ir *ir, ValueArray args) {
	// Take second argument specifying precision
	if (args.size != 1) {
		ir_error(ir, "num2str only takes one argument");
	}
	if (!isnumber(args.data[0])) {
		ir_error(ir, "num2str was called with a non-number");
	}
	char buffer[128];
	snprintf(buffer, 128, "%f", args.data[0]->number.value);
	return make_string_value(ir, make_string_slow(buffer));
}

Value* runtime_format(Ir *ir, ValueArray args) {
#define FORMAT_BUFFER_SIZE 4096
	char buffer[FORMAT_BUFFER_SIZE];
	int offset = 0;

	if (args.size > 0) {
		//TODO: Split into seperate function runtime__help_snprintf
		//TODO: Make safer, check return values
		Value *v;
		for_array(args, v) {
			if (v->kind == VALUE_STRING) {
				offset += snprintf(buffer + offset, FORMAT_BUFFER_SIZE - offset, "%.*s", (int)v->string.str.len, v->string.str.str);
			}
			else if (v->kind == VALUE_NUMBER) {
				offset += snprintf(buffer + offset, FORMAT_BUFFER_SIZE - offset, "%f", v->number.value);
			}
			else if (v->kind == VALUE_NULL) {
				offset += snprintf(buffer + offset, FORMAT_BUFFER_SIZE - offset, "(null)");
			}
		}

		return make_string_value(ir, make_string_slow_len(buffer, offset));
	}
	else {
		return null_value;
	}
}

Value* runtime_table_len(Ir *ir, ValueArray args) {
	if(args.size != 1) {
		ir_error(ir, "len() takes only one argument");
	}
	Value *v = args.data[0];
	if (!istable(v)) {
		ir_error(ir, "len() only works on tables");
	}

	return make_number_value(ir, (double)v->table.map.len);
}

Value* runtime_pow(Ir* ir, ValueArray args) {
	if (args.size != 2) {
		ir_error(ir, "pow() takes two argument");
	}
	Value *n = args.data[0];
	Value *exp = args.data[1];

	if (!isnumber(n) || !isnumber(exp)) {
		ir_error(ir, "pow() takes two numbers as arguments");
	}

	return make_number_value(ir, pow(n->number.value, exp->number.value));
}

Value* runtime_sqrt(Ir* ir, ValueArray args) {
	if (args.size != 1) {
		ir_error(ir, "sqrt() takes one argument");
	}
	Value *n = args.data[0];

	if (!isnumber(n)) {
		ir_error(ir, "sqrt() takes one number");
	}

	return make_number_value(ir, sqrt(n->number.value));
}

void add_globals(Ir *ir) {
	scope_add(ir, ir->global_scope, string("print"), make_native_function(ir, runtime_print));
	scope_add(ir, ir->global_scope, string("println"), make_native_function(ir, runtime_println));
	scope_add(ir, ir->global_scope, string("msgbox"), make_native_function(ir, runtime_msgbox));
	scope_add(ir, ir->global_scope, string("type"), make_native_function(ir, runtime_type));
	scope_add(ir, ir->global_scope, string("input"), make_native_function(ir, runtime_input));
	scope_add(ir, ir->global_scope, string("str2num"), make_native_function(ir, runtime_str2num));
	scope_add(ir, ir->global_scope, string("num2str"), make_native_function(ir, runtime_num2str));
	scope_add(ir, ir->global_scope, string("input_hidden"), make_native_function(ir, runtime_input_hidden));
	scope_add(ir, ir->global_scope, string("format"), make_native_function(ir, runtime_format));
	scope_add(ir, ir->global_scope, string("len"), make_native_function(ir, runtime_table_len));
	scope_add(ir, ir->global_scope, string("pow"), make_native_function(ir, runtime_pow));
	scope_add(ir, ir->global_scope, string("sqrt"), make_native_function(ir, runtime_sqrt));
}