typedef enum FunctionKind {
	FUNCTION_NORMAL,
	FUNCTION_NATIVE,
	FUNCTION_FFI,
} FunctionKind;

typedef struct Ir Ir;
typedef struct Scope Scope;
typedef struct Value Value;
typedef Array(Value*) ValueArray;
typedef struct Stmt Stmt;
typedef Array(Stmt*) StmtArray;

typedef struct Function {
	FunctionKind kind;
	union {
		struct {
			StringArray arg_names;
			Scope *scope;
			StmtArray stmts;
		} normal;
		struct {
			Value* (*function)(Ir *ir, ValueArray args);
		} native;
		struct {
			void *temp;
		} ffi;
	};
} Function;

typedef enum ValueKind {
	VALUE_NULL = 0,
	VALUE_NUMBER,
	VALUE_STRING,
	VALUE_TABLE,
	VALUE_FUNCTION,
	VALUE_BINOP,
	VALUE_UNARY,
	VALUE_NAME,
	VALUE_INDEX,
	VALUE_CALL,
	VALUE_FIELD,
} ValueKind;

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
			ValueArray array; // For 1, 2, 3, etc.
			//TODO: 100 = 5 should expand the array and set 5, do we want to support this?
			map_t keytable; // for "something" = 123
		} table;
		Function func;
		struct {
			TokenKind op;
			Value *lhs;
			Value *rhs;
		} binary;
		struct {
			TokenKind op;
			Value *v;
		} unary;
		struct {
			String name;
		} name;
		struct {
			Value *expr;
			Value *index;
		} index;
		struct {
			Value *expr;
			ValueArray args;
		} call;
		struct {
			Value *expr;
			String name;
		} field;
	};
};
Value *null_value = &(Value) { .kind = VALUE_NULL };

typedef enum StmtKind {
	STMT_VAR,
	STMT_ASSIGN,
	STMT_RETURN,
	STMT_CALL,
	STMT_BREAK,
	STMT_CONTINUE,
	STMT_IF,
	STMT_WHILE,
	STMT_BLOCK,
} StmtKind;

typedef struct Stmt {
	StmtKind kind;
	union {
		struct {
			String name;
			Value *expr;
		} var;
		struct {
			Value *left;
			Value *right;
		} assign;
		struct {
			Value *expr;
		} ret;
		struct {
			Value *expr;
			ValueArray args;
		} call;
		struct {
			int unused;
		} _break;
		struct {
			int unused;
		} _continue;
		struct {
			Value *cond;
			Stmt* if_block;
			Stmt* else_block;
		} _if;
		struct {
			Value *cond;
			Stmt *block;
		} _while;
		struct {
			StmtArray stmts;
		} block;
	};
} Stmt;

typedef struct Scope Scope;
struct Scope {
	Scope *parent;
	map_t symbols; // char*, Value*
};

Scope* alloc_scope(Ir *ir) {
	return calloc(1, sizeof(Scope));
}

Scope* make_scope(Ir *ir, Scope *parent) {
	Scope *scope = alloc_scope(ir);

	scope->parent = parent;
	scope->symbols = hashmap_new();

	return scope;
}

// Gets a symbol traveling up through the scope to find it
Value* scope_get(Ir *ir, Scope *scope, String name) {
	Value *v;
	if (hashmap_get(scope->symbols, name.str, &v) == MAP_MISSING) {
		if (scope->parent) {
			return scope_get(ir, scope->parent, name);
		}
		else {
			IncompletePath(); //TODO: Do ir_error
		}
	}
	return v;
}

// Adds to current 
void scope_add(Ir* ir, Scope *scope, String name, Value *v) {
	Value *test = 0;
	hashmap_get(scope->symbols, name.str, &test);
	if (test) {
		assert(!"Symbol name already exists!"); //TODO: Error!
	}
	else {
		hashmap_put(scope->symbols, name.str, v);
	}
}

// Updates a symbol, seach up through the scope
void scope_set(Ir* ir, Scope *scope, String name, Value *v) {
	Value *test = 0;
	hashmap_get(scope->symbols, name.str, &test);
	if (test) {
		hashmap_put(scope->symbols, name.str, v);
	}
	else {
		if (scope->parent) {
			scope_set(ir, scope->parent, name, v);
		}
		else {
			assert(!"Trying to set non-existent symbole!"); //TODO: Error message
		}
	}
}

typedef Array(Value) ValueMemory;
struct Ir {
	ValueMemory value_memory;
	Scope *global_scope;
	Scope *file_scope;
};

Value* alloc_value(Ir *ir) {
	return calloc(1, sizeof(Value));
	//Value v = { 0 };
	//array_add(ir->value_memory, v);
	//return &ir->value_memory.data[ir->value_memory.size - 1];
}

Stmt* alloc_stmt(Ir *ir) {
	return calloc(1, sizeof(Stmt));
}

StmtArray convert_nodes_to_stmts(Ir *ir, NodeArray nodes);
Value* expr_to_value(Ir *ir, Node *n);
Stmt* convert_node_to_stmt(Ir *ir, Node *n) {
	switch (n->kind) {
	case NODE_VAR: {
		Stmt *stmt = alloc_stmt(ir);
		stmt->kind = STMT_VAR;
		stmt->var.name = n->var.name;
		stmt->var.expr = expr_to_value(ir, n->var.expr);
		return stmt;
	} break;
	case NODE_RETURN: {
		Stmt *stmt = alloc_stmt(ir);
		stmt->kind = STMT_RETURN;
		stmt->ret.expr = 0;
		if (n->ret.expr) stmt->ret.expr = expr_to_value(ir, n->ret.expr);
		return stmt;
	} break;
	case NODE_BREAK: {
		Stmt *stmt = alloc_stmt(ir);
		stmt->kind = STMT_BREAK;
		return stmt;
	} break;
	case NODE_CONTINUE: {
		Stmt *stmt = alloc_stmt(ir);
		stmt->kind = STMT_CONTINUE;
		return stmt;
	} break;
	case NODE_BLOCK: {
		Stmt *stmt = alloc_stmt(ir);
		stmt->kind = STMT_BLOCK;
		stmt->block.stmts = convert_nodes_to_stmts(ir, n->block.stmts);
		return stmt;
	} break;
	case NODE_ASSIGN: {
		Stmt *stmt = alloc_stmt(ir);
		stmt->kind = STMT_ASSIGN;
		stmt->assign.left = expr_to_value(ir, n->assign.left);
		stmt->assign.right = expr_to_value(ir, n->assign.right);
		return stmt;
	} break;
	case NODE_CALL: {
		Stmt *stmt = alloc_stmt(ir);
		memset(stmt, 0, sizeof(Stmt)); // Clear so that we can use the array
		stmt->kind = STMT_CALL;
		stmt->call.expr = expr_to_value(ir, n->call.expr);
		Node *arg;
		for_array(n->call.args, arg) {
			array_add(stmt->call.args, expr_to_value(ir, arg));
		}
		return stmt;
	} break;
	case NODE_IF: {
		Stmt *stmt = alloc_stmt(ir);
		stmt->kind = STMT_IF;
		stmt->_if.cond = expr_to_value(ir, n->_if.cond);
		stmt->_if.if_block = convert_node_to_stmt(ir, n->_if.block);
		if(stmt->_if.else_block) {
			stmt->_if.else_block = convert_node_to_stmt(ir, n->_if.else_block);
		}
		return stmt;
	} break;
	case NODE_WHILE: {
		Stmt *stmt = alloc_stmt(ir);
		stmt->kind = STMT_WHILE;
		stmt->_while.cond = expr_to_value(ir, n->_while.cond);
		stmt->_while.block = convert_node_to_stmt(ir, n->_while.block);
		return stmt;
	} break;
	default: {
		assert(!"Unhandled node to stmt case!");
		exit(1);
	}
	}
}

StmtArray convert_nodes_to_stmts(Ir *ir, NodeArray nodes) {
	StmtArray stmts = { 0 };

	Node *n;
	for_array(nodes, n) {
		array_add(stmts, convert_node_to_stmt(ir, n));
	}

	return stmts;
}

void convert_top_levels_to_ir(Ir *ir, NodeArray stmts) {
	Node *n;
	for_array(stmts, n) {
		switch (n->kind) {
		case NODE_VAR: {
			Value *v = expr_to_value(ir, n->var.expr);
			scope_add(ir, ir->file_scope, n->var.name, v);
		} break;
		case NODE_FUNC: {
			Value *v = alloc_value(ir);
			v->kind = VALUE_FUNCTION;
			Function *f = &v->func;
			f->kind = FUNCTION_NORMAL;
			f->normal.scope = make_scope(ir, ir->file_scope);
			f->normal.arg_names = n->func.args;
			f->normal.stmts = convert_nodes_to_stmts(ir, n->func.block->block.stmts);

			scope_add(ir, ir->file_scope, n->func.name, v);
		} break;
		default: {
			assert(!"Unhandled top level to ir");
			exit(1);
		} break;
		}
	}
}

Value* global_print(Ir *ir, ValueArray args) {
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

Value* make_native_function(Ir *ir, Value* (*func)(Ir *ir, ValueArray args)) {
	Value *v = alloc_value(ir);
	v->kind = VALUE_FUNCTION;
	v->func.kind = FUNCTION_NATIVE;
	v->func.native.function = func;
	return v;
}

void add_globals(Ir *ir) {
	scope_add(ir, ir->global_scope, string("print"), make_native_function(ir, global_print));
}

void init_ir(Ir *ir, NodeArray stmts) {
	array_init(ir->value_memory, 512);
	ir->global_scope = make_scope(ir, 0);
	ir->file_scope = make_scope(ir, ir->global_scope);
	convert_top_levels_to_ir(ir, stmts);

	add_globals(ir);
}

Value* call_function(Ir *ir, Value *func_value, ValueArray args);
Value* eval_value(Ir *ir, Scope *scope, Value *v);
// True if we had a return,break,continue, etc
bool eval_stmt(Ir *ir, Scope *scope, Stmt *stmt, Value **return_value) {
	switch (stmt->kind) {
	case STMT_VAR: {
		scope_add(ir, scope, stmt->var.name, stmt->var.expr);
	} break;
	case STMT_RETURN: {
		*return_value = eval_value(ir, scope, stmt->ret.expr);
		return true;
	} break;
	case STMT_ASSIGN: {
		Value *lhs = stmt->assign.left;
		if (lhs->kind != VALUE_NAME && lhs->kind != VALUE_FIELD && lhs->kind != VALUE_INDEX) {
			// Error not supported assignemtn or something else?
			lhs = eval_value(ir, scope, stmt->assign.left);
		}
		Value *rhs = eval_value(ir, scope, stmt->assign.right);

		switch (lhs->kind) {
		case VALUE_NAME: {
			scope_set(ir, scope, lhs->name.name, rhs);
		} break;
		case VALUE_FIELD: {
			IncompletePath();
		} break;
		case VALUE_INDEX: {
			IncompletePath();
		} break;
		default: {
			assert(!"Cant assign to whatever lhs is!");
			exit(1);
		} break;
		}
	} break;
	case STMT_CALL: {
		Value *func = eval_value(ir, scope, stmt->call.expr);
		assert(func->kind == VALUE_FUNCTION); //TODO: Can we call other stuff
		ValueArray args = { 0 };
		Value *arg;
		for_array(stmt->call.args, arg) {
			array_add(args, eval_value(ir, scope, arg));
		}
		call_function(ir, func, args);
	} break;
	case STMT_BREAK: {
		IncompletePath();
	} break;
	case STMT_CONTINUE: {
		IncompletePath();
	} break;
	case STMT_IF: {
		Value *cond = eval_value(ir, scope, stmt->_if.cond);
		//TODO: Better error like function/string etc cant be used as boolean
		assert(cond->kind == TOKEN_NUMBER || cond->kind == TOKEN_NULL);
		if (cond->kind == TOKEN_NULL || cond->number.value == 0.0) {
			// else
			if (stmt->_if.else_block) {
				return eval_stmt(ir, scope, stmt->_if.else_block, return_value);
			}
		}
		else {
			// true
			return eval_stmt(ir, scope, stmt->_if.if_block, return_value);
		}
	} break;
	case STMT_WHILE: {
		Value *cond = eval_value(ir, scope, stmt->_while.cond);
		//TODO: Better error like function/string etc cant be used as boolean
		assert(cond->kind == TOKEN_NUMBER || cond->kind == TOKEN_NULL);
		while (cond->kind != TOKEN_NULL && cond->number.value != 0) {
			bool returned = eval_stmt(ir, scope, stmt->_while.block, return_value);
			if (returned) return returned;
			cond = eval_value(ir, scope, stmt->_while.cond);
		}
	} break;
	case STMT_BLOCK: {
		Scope *block_scope = make_scope(ir, scope);
		Stmt *block_stmt;
		for_array(stmt->block.stmts, block_stmt) {
			bool returned = eval_stmt(ir, block_scope, block_stmt, return_value);
			if (returned) return returned;
		}
	} break;
	default: {
		assert(!"Unhandled stmt!");
		exit(1);
	} break;
	}

	return false;
}

Value* eval_function(Ir *ir, Function func, ValueArray args) {
	// Check that we recieved the right amount of args
	// Register args with appropiate names
	// Eval all stmts
	Value *return_value = null_value;

	assert(func.normal.arg_names.size == args.size);
	Scope *scope = make_scope(ir, ir->file_scope);
	for (int i = 0; i < args.size; i++) {
		scope_add(ir, scope, func.normal.arg_names.data[i], args.data[i]);
	}
	Stmt *stmt;
	for_array(func.normal.stmts, stmt) {
		bool returned = eval_stmt(ir, scope, stmt, &return_value);
		if (returned) return return_value;
	}

	return return_value;
}

Value* call_function(Ir *ir, Value *func_value, ValueArray args) {
	assert(func_value->kind == VALUE_FUNCTION);
	
	Function func = func_value->func;
	switch (func.kind) {
	case FUNCTION_NORMAL: {
		return eval_function(ir, func, args);
	} break;
	case FUNCTION_NATIVE: {
		assert(func.native.function);
		return (*func.native.function)(ir, args);
	} break;
	case FUNCTION_FFI: {
		assert(!"Not implemented");
	} break;
	}

	return null_value;
}

Value* eval_unary(Ir *ir, Scope *scope, TokenKind op, Value *v) {
	IncompletePath();
}

Value* eval_binop(Ir *ir, Scope *scope, TokenKind op, Value *lhs, Value *rhs) {
	switch (op) {
	case TOKEN_PLUS: {
		if (lhs->kind == VALUE_NUMBER) {
			if (rhs->kind != VALUE_NUMBER) {
				IncompletePath(); //TODO: Implement ir_error
			}
			Value *v = alloc_value(ir);
			v->kind = VALUE_NUMBER;
			v->number.value = lhs->number.value + rhs->number.value;
			return v;
		}
		else if(lhs->kind== VALUE_STRING) {
			// If lhs is a string we concat multiple strings and conver the rhs if its a number
			IncompletePath();
		}
	} break;
	case TOKEN_MINUS: {
		//TODO: Error handling
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = lhs->number.value - rhs->number.value;
		return v;
	} break;
	case TOKEN_ASTERISK: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = lhs->number.value * rhs->number.value;
		return v;
	} break;
	case TOKEN_SLASH: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		//TODO: Check for zero and report the error, or just use NaN and roll with it
		v->number.value = lhs->number.value / rhs->number.value;
		return v;
	} break;
	case TOKEN_MOD: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = fmod(lhs->number.value, rhs->number.value);
		return v;
	} break;
	case TOKEN_EQUALS: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = lhs->number.value == rhs->number.value;
		return v;
	} break;
	case TOKEN_LT: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = lhs->number.value < rhs->number.value;
		return v;
	} break;
	case TOKEN_LTE: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = lhs->number.value <= rhs->number.value;
		return v;
	} break;
	case TOKEN_GT: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = lhs->number.value > rhs->number.value;
		return v;
	} break;
	case TOKEN_GTE: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = lhs->number.value >= rhs->number.value;
		return v;
	} break;
	case TOKEN_NE: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = lhs->number.value != rhs->number.value;
		return v;
	} break;
	case TOKEN_LAND: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		//TODO: Verify this works, perhaps convert the floats to integers
		v->number.value = lhs->number.value && rhs->number.value;
		return v;
	} break;
	case TOKEN_LOR: {
		assert(lhs->kind == VALUE_NUMBER);
		assert(rhs->kind == VALUE_NUMBER);
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		//TODO: Verify this works, perhaps convert the floats to integers
		v->number.value = lhs->number.value || rhs->number.value;
		return v;
	} break;
	default: {
		assert(!"Unhandled binop kind!");
		exit(1);
	} break;
	}
	assert(!"How did we get here?");
	exit(1);
}

Value* eval_value(Ir *ir, Scope *scope, Value *v) {
	switch (v->kind) {
	case VALUE_NAME: {
		Value *var = scope_get(ir, scope, v->name.name);
		assert(var);//TODO: Error message
		return eval_value(ir, scope, var);
	} break;
	case VALUE_BINOP: {
		Value *lhs = eval_value(ir, scope, v->binary.lhs);
		Value *rhs = eval_value(ir, scope, v->binary.rhs);
		return eval_binop(ir, scope, v->binary.op, lhs, rhs);
	} break;
	case VALUE_UNARY: {
		Value *rhs = eval_value(ir, scope, v);
		return eval_unary(ir, scope, v->unary.op, rhs);
	} break;
	case VALUE_CALL: {
		Value *func = eval_value(ir, scope, v->call.expr);
		assert(func->kind == VALUE_FUNCTION);
		ValueArray args = { 0 };
		Value *arg;
		for_array(v->call.args, arg) {
			array_add(args, eval_value(ir, scope, arg));
		}
		return call_function(ir, func, args);
	} break;
	case VALUE_INDEX: {
		//NOTE: Give error if expr is not a table
		IncompletePath();
	} break;
	case VALUE_FIELD: {
		IncompletePath();
	} break;
	default: {
		return v;
	} break;
	}
}

Value* expr_to_value(Ir *ir, Node *n) {
	switch (n->kind) {
	case NODE_NULL: {
		return null_value;
	} break;
	case NODE_NUMBER: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_NUMBER;
		v->number.value = n->number.value;
		return v;
	} break;
	case NODE_STRING: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_STRING;
		//TODO: Should we copy this?
		v->string.str = n->string.string;
		return v;
	} break;
	case NODE_NAME: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_NAME;
		v->name.name = n->name.name;
		return v;
	} break;
	case NODE_TABLE: {
		IncompletePath();
	} break;
	case NODE_BINOP: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_BINOP;
		v->binary.op = n->binary.op;
		v->binary.lhs = expr_to_value(ir, n->binary.lhs);
		v->binary.rhs = expr_to_value(ir, n->binary.rhs);
		return v;
	} break;
	case NODE_UNARY: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_UNARY;
		v->unary.op = n->unary.op;
		v->unary.v = expr_to_value(ir, n->unary.rhs);
		return v;
	} break;
	case NODE_FIELD: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_FIELD;
		v->field.expr = expr_to_value(ir, n->field.expr);
		v->field.name = n->field.name;
		return v;
	} break;
	case NODE_INDEX: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_INDEX;
		v->index.expr = expr_to_value(ir, n->index.expr);
		v->index.index = expr_to_value(ir, n->index.index);
		return v;
	} break;
	case NODE_CALL: {
		Value *v = alloc_value(ir);
		memset(v, 0, sizeof(Value));
		v->kind = VALUE_CALL;
		v->call.expr = expr_to_value(ir, n->call.expr);
		Node *arg;
		for_array(n->call.args, arg) {
			array_add(v->call.args, expr_to_value(ir, arg));
		}
		return v;
	} break;
	default: {
		assert(!"Node to value conversion not added");
		exit(1);
	} break;
	}
}

Value* ir_run(Ir *ir) {
	ValueArray args = {0};
	array_add(args, null_value);
	Value *main_func = scope_get(ir, ir->file_scope, string("main"));
	return call_function(ir, main_func, args);
}
