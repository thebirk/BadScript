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

Value* eval_value(Ir *ir, Scope *scope, Value *v);
void table_put(Ir *ir, Value *table, Value *key, Value *expr);
void table_put_name(Ir *ir, Value *table, String name, Value *expr);
Value* call_function(Ir *ir, Value *func_value, ValueArray args);
StmtArray convert_nodes_to_stmts(Ir *ir, NodeArray nodes);
Value* expr_to_value(Ir *ir, Node *n);
void add_globals(Ir *ir); // Found in runtime.c

#ifdef _WIN32
__declspec(noreturn)
#else
__attribute__((noreturn))
#endif
void ir_error(Ir *ir, char *format, ...);

typedef struct Function {
	FunctionKind kind;
	String name;
	SourceLoc loc;
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
	VALUE_TABLE_CONSTANT,
	VALUE_FUNCTION,
	VALUE_BINOP,
	VALUE_UNARY,
	VALUE_NAME,
	VALUE_INDEX,
	VALUE_CALL,
	VALUE_FIELD,
} ValueKind;

#define isnull(_v)     ((_v)->kind == VALUE_NULL)
#define isnumber(_v)   ((_v)->kind == VALUE_NUMBER)
#define isstring(_v)   ((_v)->kind == VALUE_STRING)
#define istable(_v)    ((_v)->kind == VALUE_TABLE)
#define isfunction(_v) ((_v)->kind == VALUE_FUNCTION)
#define isbinop(_v)    ((_v)->kind == VALUE_BINOP)
#define isname(_v)     ((_v)->kind == VALUE_NAME)
#define isunary(_v)    ((_v)->kind == VALUE_UNARY)
#define isindex(_v)    ((_v)->kind == VALUE_INDEX)
#define iscall(_v)     ((_v)->kind == VALUE_CALL)
#define isfield(_v)    ((_v)->kind == VALUE_FIELD)

// lowest level of expr: null,number,string,table
#define isbasic(_v) (isnull(_v) || isnumber(_v) || isstring(_v) || istable(_v))

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
			Map map;
		} table;
		struct {
			TableEntryArray entries;
		} table_constant;
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
	SourceLoc loc;
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
	Map symbols; // char*, Value*
};

Scope* alloc_scope(Ir *ir) {
	return calloc(1, sizeof(Scope));
}

Scope* make_scope(Ir *ir, Scope *parent) {
	Scope *scope = alloc_scope(ir);

	scope->parent = parent;

	return scope;
}

// Gets a symbol traveling up through the scope to find it
Value* scope_get(Ir *ir, Scope *scope, String name) {
	Value *v = map_get_string(&scope->symbols, name);
	if (!v) {
		if (scope->parent) {
			return scope_get(ir, scope->parent, name);
		}
		else {
			ir_error(ir, "Symbol '%.*s' does not exist.", (int)name.len, name.str);
		}
	}
	return v;
}

// Adds to current 
void scope_add(Ir* ir, Scope *scope, String name, Value *v) {
	Value *test = map_get_string(&scope->symbols, name);
	if (test) {
		ir_error(ir, "Symbol '%.*s' already exists in this scope!", (int)name.len, name.str);
	}
	else {
		map_put_string(&scope->symbols, name, v);
	}
}

// Updates a symbol, seach up through the scope
void scope_set(Ir* ir, Scope *scope, String name, Value *v) {
	Value *test = map_get_string(&scope->symbols, name);
	if (test) {
		switch (test->kind) {
		case VALUE_NULL:
		case VALUE_STRING:
		case VALUE_NUMBER:
		case VALUE_TABLE: {
			map_put_string(&scope->symbols, name, v);
		} break;
		case VALUE_FUNCTION: {
			ir_error(ir, "Cannot assign to a function!");
		} break;
		default: {
			ir_error(ir, "Cannot assign to symbol!");
		}break;
		}
	}
	else {
		if (scope->parent) {
			scope_set(ir, scope->parent, name, v);
		}
		else {
			ir_error(ir, "Symbol '%*.s' does not exist.", (int)name.len, name.str);
		}
	}
}

typedef struct StackCall {
	SourceLoc loc;
	String name;
	FunctionKind kind;
} StackCall;
typedef Array(StackCall) CallStack;

typedef Array(Value) ValueMemory;
struct Ir {
	SourceLoc loc;
	ValueMemory value_memory;
	Scope *global_scope;
	Scope *file_scope;
	CallStack callstack;
};

void print_stacktrace(Ir *ir) {
	assert(ir->callstack.size);
	printf("Stack trace (most recent call at top)\n");
	
	for (int i = (int)ir->callstack.size - 1; i >= 0; i--) {
	//for (int i = 0; i < (int)ir->callstack.size; i++) {
		StackCall *c = &ir->callstack.data[i];
		char *type = "";
		if (c->kind == FUNCTION_NATIVE) {
			type = "native:";
		}
		else if (c->kind == FUNCTION_FFI) {
			type = "ffi:";
		}
		printf("  %d\t- %s%.*s(%.*s:%d)\n", i, type, (int)c->name.len, c->name.str, (int)c->loc.file.len, c->loc.file.str, (int)c->loc.line);
	}
}

void push_call(Ir *ir, StackCall call) {
	array_add(ir->callstack, call);
}

void pop_call(Ir *ir) {
	ir->callstack.size--;
}

#ifdef _WIN32
__declspec(noreturn)
#else
__attribute__((noreturn))
#endif
void ir_error(Ir *ir, char *format, ...) {
	printf("%s(%lld, %lld): ", ir->loc.file.str, ir->loc.line, ir->loc.offset);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
	print_stacktrace(ir);

#ifdef _WIN32
	if (IsDebuggerPresent()) {
		assert(!"ir_error assert for dev");
	}
#endif

	exit(1);
}

Value* alloc_value(Ir *ir) {
	return calloc(1, sizeof(Value));
	//Value v = { 0 };
	//array_add(ir->value_memory, v);
	//return &ir->value_memory.data[ir->value_memory.size - 1];
}

Value* make_string_value(Ir *ir, String str) {
	Value *v = alloc_value(ir);
	v->kind = VALUE_STRING;
	v->string.str = str;
	return v;
}

Value* make_number_value(Ir *ir, double n) {
	Value *v = alloc_value(ir);
	v->kind = VALUE_NUMBER;
	v->number.value = n;
	return v;
}

Stmt* alloc_stmt(Ir *ir, SourceLoc loc) {
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->loc = loc;
	return stmt;
}

Stmt* convert_node_to_stmt(Ir *ir, Node *n) {
	switch (n->kind) {
	case NODE_VAR: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_VAR;
		stmt->var.name = n->var.name;
		if (n->var.expr) {
			stmt->var.expr = expr_to_value(ir, n->var.expr);
		}
		else {
			stmt->var.expr = null_value;
		}
		return stmt;
	} break;
	case NODE_RETURN: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_RETURN;
		stmt->ret.expr = 0;
		if (n->ret.expr) stmt->ret.expr = expr_to_value(ir, n->ret.expr);
		return stmt;
	} break;
	case NODE_BREAK: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_BREAK;
		return stmt;
	} break;
	case NODE_CONTINUE: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_CONTINUE;
		return stmt;
	} break;
	case NODE_BLOCK: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_BLOCK;
		if (n->block.stmts.size > 0) {
			stmt->block.stmts = convert_nodes_to_stmts(ir, n->block.stmts);
		}
		return stmt;
	} break;
	case NODE_ASSIGN: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_ASSIGN;
		stmt->assign.left = expr_to_value(ir, n->assign.left);
		stmt->assign.right = expr_to_value(ir, n->assign.right);
		return stmt;
	} break;
	case NODE_CALL: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_CALL;
		stmt->call.expr = expr_to_value(ir, n->call.expr);
		if (n->call.args.size > 0) {
			Node *arg;
			for_array(n->call.args, arg) {
				array_add(stmt->call.args, expr_to_value(ir, arg));
			}
		}
		return stmt;
	} break;
	case NODE_IF: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_IF;
		stmt->_if.cond = expr_to_value(ir, n->_if.cond);
		stmt->_if.if_block = convert_node_to_stmt(ir, n->_if.block);
		if(n->_if.else_block) {
			stmt->_if.else_block = convert_node_to_stmt(ir, n->_if.else_block);
		}
		return stmt;
	} break;
	case NODE_WHILE: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
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

	if (nodes.size > 0) {
		Node *n;
		for_array(nodes, n) {
			array_add(stmts, convert_node_to_stmt(ir, n));
		}
	}

	return stmts;
}

void ir_import_file(Ir *ir, String path);
void convert_top_levels_to_ir(Ir *ir, Scope *scope, NodeArray stmts) {
	Node *n;
	for_array(stmts, n) {
		ir->loc = n->loc;
		switch (n->kind) {
		case NODE_IMPORT: {
			ir_import_file(ir, n->import.name);
		} break;
		case NODE_VAR: {
			Value *v = eval_value(ir, ir->file_scope, expr_to_value(ir, n->var.expr));
			scope_add(ir, ir->file_scope, n->var.name, v);
		} break;
		case NODE_FUNC: {
			Value *v = alloc_value(ir);
			v->kind = VALUE_FUNCTION;
			Function *f = &v->func;
			f->kind = FUNCTION_NORMAL;
			f->name = n->func.name;
			f->loc = n->loc;
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

void ir_import_file(Ir *ir, String path) {
	Parser p;
	memset(&p, 0, sizeof(Parser));
	init_parser(&p, path);
	NodeArray stmts = parse(&p);

	convert_top_levels_to_ir(ir, ir->file_scope, stmts);
}

void init_ir(Ir *ir, NodeArray stmts) {
	array_init(ir->value_memory, 512);
	ir->global_scope = make_scope(ir, 0);
	ir->file_scope = make_scope(ir, ir->global_scope);
	convert_top_levels_to_ir(ir, ir->file_scope, stmts);

	add_globals(ir);
}

// True if we had a return,break,continue, etc
bool eval_stmt(Ir *ir, Scope *scope, Stmt *stmt, Value **return_value) {
	ir->loc = stmt->loc;
	switch (stmt->kind) {
	case STMT_VAR: {
		Value *v = eval_value(ir, scope, stmt->var.expr);
		scope_add(ir, scope, stmt->var.name, v);
	} break;
	case STMT_RETURN: {
		if (stmt->ret.expr) {
			*return_value = eval_value(ir, scope, stmt->ret.expr);
		}
		return true;
	} break;
	case STMT_ASSIGN: {
		Value *lhs = stmt->assign.left;
		if (lhs->kind != VALUE_NAME && lhs->kind != VALUE_FIELD && lhs->kind != VALUE_INDEX) {
			// Error not supported assignemtn or something else?
			lhs = eval_value(ir, scope, stmt->assign.left);
		}
		if (lhs->kind != VALUE_NAME && lhs->kind != VALUE_FIELD && lhs->kind != VALUE_INDEX) {
			ir_error(ir, "Cannot assign to left hand");
		}

		Value *rhs = eval_value(ir, scope, stmt->assign.right);

		switch (lhs->kind) {
		case VALUE_NAME: {
			scope_set(ir, scope, lhs->name.name, rhs);
		} break;
		case VALUE_FIELD: {
			Value *expr = eval_value(ir, scope, lhs->field.expr);
			table_put_name(ir, expr, lhs->field.name, rhs);
		} break;
		case VALUE_INDEX: {
			Value *expr = eval_value(ir, scope, lhs->index.expr);
			Value *index = eval_value(ir, scope, lhs->index.index);
			table_put(ir, expr, index, rhs);
		} break;
		default: {
			ir_error(ir, "Cannot assign to left hand");
		} break;
		}
	} break;
	case STMT_CALL: {
		Value *func = eval_value(ir, scope, stmt->call.expr);
		if (!isfunction(func)) {
			ir_error(ir, "Tried to call a non-function value!");
		}
		ValueArray args = { 0 };
		if (stmt->call.args.size > 0) {
			Value *arg;
			for_array(stmt->call.args, arg) {
				Value *v = eval_value(ir, scope, arg);
				array_add(args, v);
			}
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
		if (!isnumber(cond) && !isnull(cond)) {
			ir_error(ir, "Condition does not evaluate to number or null.");
		}
		if (cond->kind == VALUE_NULL || cond->number.value == 0.0) {
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
		if (!isnumber(cond) && !isnull(cond)) {
			ir_error(ir, "Condition does not evaluate to number or null.");
		}

		while (cond->kind != VALUE_NULL && cond->number.value != 0) {
			bool returned = eval_stmt(ir, scope, stmt->_while.block, return_value);
			if (returned) return returned;
			
			cond = eval_value(ir, scope, stmt->_while.cond);
			if (!isnumber(cond) && !isnull(cond)) {
				ir_error(ir, "Condition does not evaluate to number or null.");
			}
		}
	} break;
	case STMT_BLOCK: {
		if (stmt->block.stmts.size > 0) {
			Scope *block_scope = make_scope(ir, scope);
			Stmt *block_stmt;
			for_array(stmt->block.stmts, block_stmt) {
				bool returned = eval_stmt(ir, block_scope, block_stmt, return_value);
				if (returned) return returned;
			}
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

	if (func.normal.arg_names.size != args.size) {
		ir_error(ir, "Argument count mismatch! Wanted %d got %d.", (int)func.normal.arg_names.size, (int)args.size);
	}

	if (func.normal.stmts.size > 0) {
		Scope *scope = make_scope(ir, ir->file_scope);
		for (int i = 0; i < args.size; i++) {
			scope_add(ir, scope, func.normal.arg_names.data[i], args.data[i]);
		}
		Stmt *stmt;
		for_array(func.normal.stmts, stmt) {
			bool returned = eval_stmt(ir, scope, stmt, &return_value);
			if (returned) return return_value;
		}
	}

	return return_value;
}

Value* call_function(Ir *ir, Value *func_value, ValueArray args) {
	assert(func_value->kind == VALUE_FUNCTION);

	Value *return_value = null_value;

	Function func = func_value->func;
	switch (func.kind) {
	case FUNCTION_NORMAL: {
		StackCall call = { 0 };
		call.loc = func.loc;
		call.kind = func.kind;
		call.name = func.name;
		push_call(ir, call);
		return_value = eval_function(ir, func, args);
		pop_call(ir);
	} break;
	case FUNCTION_NATIVE: {
		assert(func.native.function);
		return_value = (*func.native.function)(ir, args);
	} break;
	case FUNCTION_FFI: {
		assert(!"Not implemented");
	} break;
	}

	return return_value;
}

Value* eval_unary(Ir *ir, Scope *scope, TokenKind op, Value *v) {
	Value *res = alloc_value(ir);
	res->kind = VALUE_NUMBER;
	Value *rhs = eval_value(ir, scope, v->unary.v);
	if (!isnumber(rhs)) {
		ir_error(ir, "Unary operators only work with numbers.");
	}
	switch (op) {
	case TOKEN_PLUS: {
		res->number.value = +rhs->number.value;
	} break;
	case TOKEN_MINUS: {
		res->number.value = -rhs->number.value;
	} break;
	default: {
		assert(!"Invalid unary op");
		exit(1);
	} break;
	}

	return res;
}

Value* eval_number_op(Ir *ir, Scope *scope, TokenKind op, Value *lhs, Value *rhs) {
	Value *v = alloc_value(ir);
	v->kind = VALUE_NUMBER;
	switch (op) {
	case TOKEN_PLUS:     v->number.value = lhs->number.value + rhs->number.value; break;
	case TOKEN_MINUS:    v->number.value = lhs->number.value - rhs->number.value; break;
	case TOKEN_ASTERISK: v->number.value = lhs->number.value * rhs->number.value; break;
	case TOKEN_SLASH:    v->number.value = lhs->number.value / rhs->number.value; break;
	case TOKEN_MOD:      v->number.value = fmod(lhs->number.value, rhs->number.value); break;
	case TOKEN_EQUALS:   v->number.value = lhs->number.value == rhs->number.value; break;
	case TOKEN_LT:       v->number.value = lhs->number.value < rhs->number.value; break;
	case TOKEN_LTE:      v->number.value = lhs->number.value <= rhs->number.value; break;
	case TOKEN_GT:       v->number.value = lhs->number.value > rhs->number.value; break;
	case TOKEN_GTE:      v->number.value = lhs->number.value >= rhs->number.value; break;
	case TOKEN_NE:       v->number.value = lhs->number.value != rhs->number.value; break;
	case TOKEN_LAND:     v->number.value = lhs->number.value && rhs->number.value; break;
	case TOKEN_LOR:      v->number.value = lhs->number.value || rhs->number.value; break;
	default: {
		assert(!"Unimplemented binary op");
	}
	}
	return v;
}

Value* eval_binop(Ir *ir, Scope *scope, TokenKind op, Value *lhs, Value *rhs) {
	switch (op) {
	case TOKEN_PLUS: {
		if (isnumber(lhs)) {
			if (!isnumber(rhs)) {
				ir_error(ir, "Operator '%s' only work with numbers and strings.", token_kind_to_string(op));
			}
			Value *v = alloc_value(ir);
			v->kind = VALUE_NUMBER;
			v->number.value = lhs->number.value + rhs->number.value;
			return v;
		}
		else if(lhs->kind== VALUE_STRING) {
			// If lhs is a string we concat multiple strings and conver the rhs if its a number
			ir_error(ir, "String concatination is not yet implemented!");
		}
	} break;

	case TOKEN_MINUS:
	case TOKEN_ASTERISK:
	case TOKEN_SLASH:
	case TOKEN_MOD: {
		if (!isnumber(lhs) || !isnumber(rhs)) {
			ir_error(ir, "Operator '%s' is only allowed with numbers", token_kind_to_string(op));
		}
		else {
			return eval_number_op(ir, scope, op, lhs, rhs);
		}
	} break;

	case TOKEN_LT:
	case TOKEN_LTE:
	case TOKEN_GT:
	case TOKEN_GTE:
	case TOKEN_LAND:
	case TOKEN_LOR: {
		if (!isnumber(lhs) || !isnumber(rhs)) {
			ir_error(ir, "Operator '%s' is only allowed with numbers", token_kind_to_string(op));
		}
		return eval_number_op(ir, scope, op, lhs, rhs);
	} break;

	case TOKEN_EQUALS: {
		if (isstring(lhs)) {
			if (!isstring(rhs)) {
				ir_error(ir, "Cannot compare string to rhs!");
			}
			Value *v = alloc_value(ir);
			v->kind = VALUE_NUMBER;
			v->number.value = strings_match(lhs->string.str, rhs->string.str);
			return v;
		}
		else {
			if (!isnumber(lhs) || !isnumber(rhs)) {
				ir_error(ir, "Operator '%s' only works with numbers and strings", token_kind_to_string(op));
			}
			Value *v = alloc_value(ir);
			v->kind = VALUE_NUMBER;
			v->number.value = lhs->number.value == rhs->number.value;
			return v;
		}
	} break;
	case TOKEN_NE: {
		if (isstring(lhs)) {
			if (!isstring(rhs)) {
				ir_error(ir, "Can only compare strings with strings.");
			}
			Value *v = alloc_value(ir);
			v->kind = VALUE_NUMBER;
			v->number.value = !strings_match(lhs->string.str, rhs->string.str);
			return v;
		}
		else {
			if (!isnumber(lhs) || !isnumber(rhs)) {
				ir_error(ir, "Operator '%s' only works with strings and numbers.", token_kind_to_string(op));
			}
			Value *v = alloc_value(ir);
			v->kind = VALUE_NUMBER;
			v->number.value = lhs->number.value != rhs->number.value;
			return v;
		}
	} break;
	
	default: {
		assert(!"Unhandled binop kind!");
		exit(1);
	} break;
	}
	assert(!"How did we get here?");
	exit(1);
}

uint64_t hash_value(Ir *ir, Value *v) {
	switch (v->kind) {
	case VALUE_NULL: return hash_ptr(null_value);
	case VALUE_NUMBER: {
		uint64_t num = 0;
		assert(sizeof(uint64_t) == sizeof(double));
		memcpy(&num, &v->number.value, sizeof(uint64_t));
		return hash_uint64(num);
	}
	case VALUE_STRING: {
		return hash_bytes(v->string.str.str, v->string.str.len);
	}
	case VALUE_TABLE: {
		ir_error(ir, "A table cannot be used as an index");
	}
	case VALUE_NAME: {
		return hash_bytes(v->name.name.str, v->name.name.len);
	}
	default: {
		assert(!"Unimplemented hash_value case");
		exit(1);
	} break;
	}
}

void table_put(Ir *ir, Value *table, Value *key, Value *val) {
	assert(table);
	assert(key);
	assert(val);

	map_put_hash(&table->table.map, hash_value(ir, key), val);
}

void table_put_name(Ir *ir, Value *table, String name, Value *val) {
	assert(table);
	assert(val);

	map_put_hash(&table->table.map, hash_bytes(name.str, name.len), val);
}

Value* table_get(Ir *ir, Value *table, Value *key) {
	uint64_t hash = hash_value(ir, key);
	return map_get(&table->table.map, hash);
}

Value* table_get_name(Ir *ir, Value *table, String name) {
	uint64_t hash = hash_bytes(name.str, name.len);
	return map_get(&table->table.map, hash);
}

Value* eval_value(Ir *ir, Scope *scope, Value *v) {
	switch (v->kind) {
	case VALUE_NAME: {
		Value *var = scope_get(ir, scope, v->name.name);
		assert(var); // scope_get should complain about missing symbols
		return eval_value(ir, scope, var);
	} break;
	case VALUE_BINOP: {
		Value *lhs = eval_value(ir, scope, v->binary.lhs);
		Value *rhs = eval_value(ir, scope, v->binary.rhs);
		return eval_binop(ir, scope, v->binary.op, lhs, rhs);
	} break;
	case VALUE_UNARY: {
		return eval_unary(ir, scope, v->unary.op, v);
	} break;
	case VALUE_CALL: {
		Value *func = eval_value(ir, scope, v->call.expr);
		assert(func->kind == VALUE_FUNCTION);
		ValueArray args = { 0 };
		if (v->call.args.size > 0) {
			Value *arg;
			for_array(v->call.args, arg) {
				Value *v = eval_value(ir, scope, arg);
				array_add(args, v);
			}
		}
		return eval_value(ir, scope, call_function(ir, func, args));
	} break;
	case VALUE_TABLE_CONSTANT: {
		Value *t = alloc_value(ir);
		t->kind = VALUE_TABLE;

		if (v->table_constant.entries.size > 0) {
			size_t index = 0;
			TableEntry *e;
			for_array_ref(v->table_constant.entries, e) {
				switch (e->kind) {
				case ENTRY_NORMAL: {  // v
					table_put(ir, t, make_number_value(ir, (double)index), eval_value(ir, scope, expr_to_value(ir, e->expr)));
					index++;
				} break;
				case ENTRY_INDEX: { // [blah] = v
					Value *index = eval_value(ir, scope, expr_to_value(ir, e->index));
					//TODO: Handle null index
					table_put(ir, t, index, eval_value(ir, scope, expr_to_value(ir, e->expr)));
				} break;
				case ENTRY_KEY: {     // name = v
					Value *name = expr_to_value(ir, e->key);
					if (!isname(name) && !isstring(name)) {
						ir_error(ir, "Expected left hand side of assignment to be a name or string!");
					}
					table_put(ir, t, name, eval_value(ir, scope, expr_to_value(ir, e->expr)));
				} break;

				default: {
					assert(!"Invalid table entry kind!");
					exit(1);
				} break;
				}
			}
		}

		return t;
	} break;
	case VALUE_INDEX: {
		Value *expr = eval_value(ir, scope, v->index.expr);
		if (!istable(expr)) {
			ir_error(ir, "Left hand side of '[]' operator is not a table!");
		}

		Value *index = eval_value(ir, scope, v->index.index);
		//TODO: Where do we handle a null index?

		Value *table_value = table_get(ir, expr, index);
		if (table_value) {
			return eval_value(ir, scope, table_value);
		}
		else {
			return null_value;
		}
		
	} break;
	case VALUE_FIELD: {
		Value *expr = eval_value(ir, scope, v->field.expr);
		if (!istable(expr)) {
			ir_error(ir, "Left hand side of '.' it not a table!");
		}

		Value *table_value = table_get_name(ir, expr, v->field.name);
		if (table_value) {
			return eval_value(ir, scope, table_value);
		}
		else {
			return null_value;
		}
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
		Value *v = alloc_value(ir);
		v->kind = VALUE_TABLE_CONSTANT;
		v->table_constant.entries = n->table.entries;
		return v;
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
		v->kind = VALUE_CALL;
		v->call.expr = expr_to_value(ir, n->call.expr);
		if (n->call.args.size > 0) {
			Node *arg;
			for_array(n->call.args, arg) {
				array_add(v->call.args, expr_to_value(ir, arg));
			}
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
