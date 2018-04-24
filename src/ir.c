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

typedef enum GCColor {
	GC_WHITE = 1,
	GC_GREY  = 2,
	GC_BLACK = 3,
} GCColor;

typedef enum GCKind {
	GC_VALUE = 1,
	GC_STMT  = 2,
	GC_SCOPE = 3,
} GCKind;


typedef struct GCObject GCObject;
struct GCObject {
	GCColor color;
	GCKind gc_kind;
	GCObject *prev;
	GCObject *next;
};

Value* eval_value(Ir *ir, Scope *scope, Value *v);
void table_put(Ir *ir, Value *table, Value *key, Value *expr);
void table_put_name(Ir *ir, Value *table, String name, Value *expr);
Value* call_function(Ir *ir, Value *func_value, ValueArray args, bool is_method_call);
StmtArray convert_nodes_to_stmts(Ir *ir, NodeArray nodes);
Value* expr_to_value(Ir *ir, Node *n);
void add_globals(Ir *ir); // Found in runtime.c
void free_stmt(Ir *ir, Stmt *stmt);
void gc_add_to_grey(Ir *ir, GCObject *obj);

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
	VALUE_METHOD_CALL,
	VALUE_FIELD,
	VALUE_USERDATA,
	VALUE_INCDEC,
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
#define isuserdata(_v) ((_v)->kind == VALUE_USERDATA)

// lowest level of expr: null,number,string,table
#define isbasic(_v) (isnull(_v) || isnumber(_v) || isstring(_v) || istable(_v))

struct Value {
	GCObject gc;
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
		struct {
			Value *expr;
			String name;
			ValueArray args;
		} method_call;
		struct {
			void *data;
		} userdata;
		struct {
			Value *expr;
			TokenKind op;
			bool post;
		} incdec;
	};
};
Value *null_value = &(Value) { .kind = VALUE_NULL };

typedef enum StmtKind {
	STMT_VAR,
	STMT_ASSIGN,
	STMT_RETURN,
	STMT_CALL,
	STMT_METHOD_CALL,
	STMT_BREAK,
	STMT_CONTINUE,
	STMT_IF,
	STMT_WHILE,
	STMT_BLOCK,
	STMT_INCDEC,
} StmtKind;

struct Stmt {
	GCObject gc;
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
			Value *expr;
			String name;
			ValueArray args;
		} method_call;
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
		struct {
			Value *expr;
			TokenKind op;
		} incdec;
	};
};

typedef struct StackCall {
	SourceLoc loc;
	String name;
	FunctionKind kind;
} StackCall;
typedef Array(StackCall) CallStack;

typedef Array(Scope*) ScopeStack;
struct Ir {
	SourceLoc loc;
	Scope *global_scope;
	Scope *file_scope;
	CallStack callstack;
	ScopeStack scope_stack;

	Pool value_pool;
	Pool scope_pool;
	Pool stmt_pool;

	GCObject *white_list;
	GCObject *grey_list;
	GCObject *black_list;

	int allocated_values;
	int max_allocated_values;
	bool do_gc;
};

void print_stacktrace(Ir *ir) {
	assert(ir->callstack.size);
	
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
		printf("    %s%.*s() - %.*s:(%d)\n", type, (int)c->name.len, c->name.str, (int)c->loc.file.len, c->loc.file.str, (int)c->loc.line);
	}
}

void push_call(Ir *ir, StackCall call) {
	array_add(ir->callstack, call);
}

void pop_call(Ir *ir) {
	ir->callstack.size--;
	StackCall *call = &ir->callstack.data[ir->callstack.size];
	free(call->name.str);
}

typedef struct Scope Scope;
struct Scope {
	GCObject gc;
	Scope *parent;
	Map symbols; // char*, Value*
};

Scope* alloc_scope(Ir *ir) {
	Scope *scope = pool_alloc(&ir->scope_pool);
	
	scope->gc.gc_kind = GC_SCOPE;
	scope->gc.color = GC_GREY;
	scope->gc.next = ir->grey_list;
	scope->gc.prev = 0;
	ir->grey_list = (GCObject*)scope;

	return scope;
}

void free_scope(Ir *ir, Scope *scope) {
	pool_release(&ir->scope_pool, scope);
}

/*
Scope* make_scope_no_gc(Ir *ir, Scope *parent) {
	Scope *scope = calloc(1, sizeof(Scope));

	scope->parent = parent;

	return scope;
}
*/

Scope* make_scope(Ir *ir, Scope *parent) {
	Scope *scope = alloc_scope(ir);

	scope->parent = parent;

	return scope;
}

Scope* push_scope(Ir *ir, Scope *parent) {
	Scope *scope = make_scope(ir, parent);
	array_add(ir->scope_stack, scope);
	return scope;
}

void pop_scope(Ir *ir) {
	assert(ir->scope_stack.size > 0);
	ir->scope_stack.size--;
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

uint64_t hash_value(Ir *ir, Value *v) {
	switch (v->kind) {
	case VALUE_NULL: return hash_ptr(null_value); //TODO: This is constant, we only need to hash this ptr once
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

Value* alloc_value(Ir *ir) {
	Value *v = pool_alloc(&ir->value_pool);
	ir->allocated_values++;

	v->gc.gc_kind = GC_VALUE;
	v->gc.color = GC_GREY;
	v->gc.next = ir->grey_list;
	v->gc.prev = 0;
	ir->grey_list = (GCObject*)v;

	return v;
}

void free_value(Ir *ir, Value *v) {
	pool_release(&ir->value_pool, v);
}

void gc_mark(Ir *ir) {
	if (ir->global_scope && ir->global_scope->symbols.cap > 0) {
		for (size_t i = 0; i < ir->global_scope->symbols.cap; i++) {
			MapEntry *e = &ir->global_scope->symbols.entries[i];
			if (e->hash) {
				Value *v = e->val;
				gc_add_to_grey(ir, (GCObject*)v);
			}
		}
	}
	if (ir->file_scope && ir->file_scope->symbols.cap > 0) {
		for (size_t i = 0; i < ir->file_scope->symbols.cap; i++) {
			MapEntry *e = &ir->file_scope->symbols.entries[i];
			if (e->hash) {
				Value *v = e->val;
				gc_add_to_grey(ir, (GCObject*)v);
			}
		}
	}
	if (ir->scope_stack.size > 0) {
		Scope *scope;
		for_array(ir->scope_stack, scope) {
			gc_add_to_grey(ir, (GCObject*) scope);
		}
	}
}

void gc_remove_from_specific_list(GCObject **list, GCObject *obj) {
	if (obj->prev) {
		obj->prev->next = obj->next;
		if (obj->next) {
			obj->next->prev = obj->prev;
		}
		obj->prev = 0;
		obj->next = 0;
	}
	else {
		*list = obj->next;
		if (obj->next) {
			obj->next->prev = 0;
			if ((*list)->next) {
				(*list)->next->prev = *list;
			}
		}
		obj->next = 0;
		obj->prev = 0;
	}
}

void gc_remove_from_list(Ir *ir, GCObject *obj) {
	switch (obj->color) {
	case GC_WHITE: {
		gc_remove_from_specific_list(&ir->white_list, obj);
	} break;
	case GC_GREY: {
		gc_remove_from_specific_list(&ir->grey_list, obj);
	} break;
	case GC_BLACK: {
		gc_remove_from_specific_list(&ir->black_list, obj);
	} break;
	default: {
		assert(!"Invalid color!");
	}
	}

	
}

void gc_add_to_grey(Ir *ir, GCObject *obj) {
	if (obj->color == GC_BLACK || obj->color == GC_GREY) return;
	gc_remove_from_list(ir, obj);
	obj->color = GC_GREY;
	obj->next = ir->grey_list;
	if (obj->next) {
		obj->next->prev = obj;
	}
	obj->prev = 0;
	ir->grey_list = obj;
}

void gc_add_to_black(Ir *ir, GCObject *obj) {
	gc_remove_from_list(ir, obj);
	obj->color = GC_BLACK;
	obj->next = ir->black_list;
	if (obj->next) {
		obj->next->prev = obj;
	}
	obj->prev = 0;
	ir->black_list = obj;
}

void gc_mark_value(Ir *ir, Value *value);
void gc_mark_stmt(Ir *ir, Stmt *stmt) {
	if (stmt->gc.color == GC_BLACK) return;
	gc_add_to_black(ir, (GCObject*)stmt);

	switch (stmt->kind) {
	case STMT_VAR: {
		gc_add_to_grey(ir, (GCObject*) stmt->var.expr);
	} break;
	case STMT_ASSIGN: {
		gc_add_to_grey(ir, (GCObject*)stmt->assign.left);
		gc_add_to_grey(ir, (GCObject*)stmt->assign.right);
	} break;
	case STMT_RETURN: {
		if (stmt->ret.expr) {
			gc_add_to_grey(ir, (GCObject*)stmt->ret.expr);
		}
	} break;
	case STMT_CALL: {
		gc_add_to_grey(ir, (GCObject*)stmt->call.expr);
		if (stmt->call.args.size > 0) {
			Value *arg;
			for_array(stmt->call.args, arg) {
				gc_add_to_grey(ir, (GCObject*)arg);
			}
		}
	} break;
	case STMT_METHOD_CALL: {
		gc_add_to_grey(ir, (GCObject*)stmt->method_call.expr);
		if (stmt->method_call.args.size > 0) {
			Value *arg;
			for_array(stmt->method_call.args, arg) {
				gc_add_to_grey(ir, (GCObject*)arg);
			}
		}
	} break;
	case STMT_IF: {
		gc_add_to_grey(ir, (GCObject*)stmt->_if.cond);
		gc_add_to_grey(ir, (GCObject*)stmt->_if.if_block);
		if (stmt->_if.else_block) {
			gc_add_to_grey(ir, (GCObject*)stmt->_if.else_block);
		}
	} break;
	case STMT_WHILE: {
		gc_add_to_grey(ir, (GCObject*)stmt->_while.cond);
		gc_add_to_grey(ir, (GCObject*)stmt->_while.block);
	} break;
	case STMT_BLOCK: {
		Stmt *st;
		if (stmt->block.stmts.size > 0) {
			for_array(stmt->block.stmts, st) {
				gc_add_to_grey(ir, (GCObject*)st);
			}
		}
	} break;
	case STMT_INCDEC: {
		gc_add_to_grey(ir, (GCObject*)stmt->incdec.expr);
	} break;
	default: {
		assert(!"How did we get here?");
	} break;
	}
}

void gc_mark_value(Ir *ir, Value *v) {
	if (v->gc.color == GC_BLACK) return;
	gc_add_to_black(ir, (GCObject*)v);

	switch (v->kind) {
	case VALUE_BINOP: {
		gc_add_to_grey(ir, (GCObject*)v->binary.lhs);
		gc_add_to_grey(ir, (GCObject*)v->binary.rhs);
	} break;
	case VALUE_UNARY: {
		gc_add_to_grey(ir, (GCObject*)v->unary.v);
	} break;
	case VALUE_INDEX: {
		gc_add_to_grey(ir, (GCObject*)v->index.expr);
		gc_add_to_grey(ir, (GCObject*)v->index.index);
	} break;
	case VALUE_CALL: {
		gc_add_to_grey(ir, (GCObject*)v->call.expr);
		if (v->call.args.size > 0) {
			Value *arg;
			for_array(v->call.args, arg) {
				gc_add_to_grey(ir, (GCObject*)arg);
			}
		}
	} break;
	case VALUE_FIELD: {
		gc_add_to_grey(ir, (GCObject*)v->field.expr);
	} break;
	case VALUE_METHOD_CALL: {
		gc_add_to_grey(ir, (GCObject*)v->method_call.expr);
		if (v->method_call.args.size > 0) {
			Value *arg;
			for_array(v->method_call.args, arg) {
				gc_add_to_grey(ir, (GCObject*)arg);
			}
		}
	} break;
	case VALUE_FUNCTION: {
		Function *f = &v->func;
		switch (f->kind) {
		case FUNCTION_NORMAL: {
			Stmt *stmt;
			if (f->normal.stmts.size > 0) {
				for_array(f->normal.stmts, stmt) {
					gc_add_to_grey(ir, (GCObject*)stmt);
				}
			}
		} break;
		case FUNCTION_NATIVE: {

		} break;
		case FUNCTION_FFI: {
			assert(!"Nope!");
		}
		}
	} break;
	case VALUE_TABLE: {
		for (size_t i = 0; i < v->table.map.cap; i++) {
			MapEntry *e = &v->table.map.entries[i];
			if (e->hash) {
				gc_add_to_grey(ir, (GCObject*)e->val);
			}
		}
	} break;
	case VALUE_INCDEC: {
		gc_add_to_grey(ir, (GCObject*)v->incdec.expr);
	} break;
	}
}

void gc_mark_scope(Ir *ir, Scope *scope) {
	if (scope->gc.color == GC_BLACK) return;
	gc_add_to_black(ir, (GCObject*)scope);

	for (size_t i = 0; i < scope->symbols.cap; i++) {
		MapEntry *e = &scope->symbols.entries[i];

		if (e->hash) {
			Value *v = e->val;
			gc_add_to_grey(ir, (GCObject*)v);
		}


	}
	if (scope->parent) {
		gc_add_to_grey(ir, (GCObject*)scope->parent);
	}
}

void gc_free_value(Ir *ir, Value *v) {
	switch (v->kind) {
	case VALUE_STRING: {
		//TODO: Replace with string_free
		free(v->string.str.str);
	} break;
	case VALUE_TABLE: {
		map_free(&v->table.map);
	} break;
	case VALUE_NAME: {
		free(v->name.name.str);
	} break;
	case VALUE_FIELD: {
		free(v->field.name.str);
	} break;
	case VALUE_METHOD_CALL: {
		free(v->method_call.name.str);
		array_free(v->method_call.args);
	} break;
	case VALUE_CALL: {
		array_free(v->call.args);
	} break;
	case VALUE_TABLE_CONSTANT: {
		array_free(v->table_constant.entries);
	} break;
	case VALUE_FUNCTION: {
		free(v->func.name.str);
		switch (v->func.kind) {
		case FUNCTION_NORMAL: {
			if (v->func.normal.arg_names.size > 0) {
				String *str;
				for_array_ref(v->func.normal.arg_names, str) {
					free(str->str);
				}
				array_free(v->func.normal.arg_names);
			}
			array_free(v->func.normal.stmts);
		} break;
		}
	} break;
	}
	free_value(ir, v);
}

void gc_free_stmt(Ir *ir, Stmt *stmt) {
	switch(stmt->kind) {
	case STMT_VAR: {
		free(stmt->var.name.str);
	} break;
	case STMT_CALL: {
		array_free(stmt->call.args);
	} break;
	case STMT_METHOD_CALL: {
		array_free(stmt->method_call.args);
		free(stmt->method_call.name.str);
	} break;
	case STMT_BLOCK: {
		array_free(stmt->block.stmts);
	} break;
	}

	free_stmt(ir, stmt);
}

void gc_free_scope(Ir *ir, Scope *scope) {
	if (scope->symbols.entries) {
		map_free(&scope->symbols);
	}
	free_scope(ir, scope);
}

void gc_do_greys(Ir *ir) {
	if (!ir->do_gc) return;
	// Just do them all for now

	int work = 5;
	GCObject **obj_list = &ir->grey_list;
	while (*obj_list && work > 0) {
		GCObject *obj = *obj_list;
		*obj_list = obj->next;

		// Mark obj black and all references grey
		switch (obj->gc_kind) {
		case GC_VALUE: {
			gc_mark_value(ir, (Value*) obj);
		} break;
		case GC_STMT: {
			gc_mark_stmt(ir, (Stmt*) obj);
		} break;
		case GC_SCOPE: {
			gc_mark_scope(ir, (Scope*) obj);
		} break;
		default: {
			assert(!"Invalid gc_kind case");
		}
		}

		work--;
	}

	if (ir->grey_list == 0) {
		{
			GCObject *obj = ir->white_list;
			while (obj) {
				GCObject *unreached = obj;
				gc_remove_from_list(ir, obj);
				obj = ir->white_list;

				switch (unreached->gc_kind) {
				case GC_VALUE: {
					gc_free_value(ir, (Value*)unreached);
				} break;
				case GC_STMT: {
					gc_free_stmt(ir, (Stmt*)unreached);
				} break;
				case GC_SCOPE: {
					gc_free_scope(ir, (Scope*)unreached);
				} break;
				default: {
					assert(!"Invalid gc_kind");
				}
				}
			}
		}

		{
			GCObject *obj = ir->black_list;
			while (obj) {
				GCObject *o = obj;
				gc_remove_from_list(ir, o);
				o->color = GC_WHITE;
				o->next = ir->white_list;
				if (o->next) {
					o->next->prev = o;
				}
				o->prev = 0;
				ir->white_list = o;
				obj = ir->black_list;
			}
		}

		gc_mark(ir);
	}
}

#if 0
void gc_mark(Value *v);
void gc_mark_stmt(Stmt *stmt) {
	if (stmt->gc_marked) return;
	stmt->gc_marked = true;
	
	switch (stmt->kind) {
	case STMT_VAR: {
		gc_mark(stmt->var.expr);
	} break;
	case STMT_ASSIGN: {
		gc_mark(stmt->assign.left);
		gc_mark(stmt->assign.right);
	} break;
	case STMT_RETURN: {
		if (stmt->ret.expr) {
			gc_mark(stmt->ret.expr);
		}
	} break;
	case STMT_CALL: {
		gc_mark(stmt->call.expr);
		if (stmt->call.args.size > 0) {
			Value *arg;
			for_array(stmt->call.args, arg) {
				gc_mark(arg);
			}
		}
	} break;
	case STMT_METHOD_CALL: {
		gc_mark(stmt->method_call.expr);
		if (stmt->method_call.args.size > 0) {
			Value *arg;
			for_array(stmt->method_call.args, arg) {
				gc_mark(arg);
			}
		}
	} break;
	case STMT_IF: {
		gc_mark(stmt->_if.cond);
		gc_mark_stmt(stmt->_if.if_block);
		if (stmt->_if.else_block) {
			gc_mark_stmt(stmt->_if.else_block);
		}
	} break;
	case STMT_WHILE: {
		gc_mark(stmt->_while.cond);
		gc_mark_stmt(stmt->_while.block);
	} break;
	case STMT_BLOCK: {
		Stmt *st;
		if (stmt->block.stmts.size > 0) {
			for_array(stmt->block.stmts, st) {
				gc_mark_stmt(st);
			}
		}
	} break;
	case STMT_INCDEC: {
		gc_mark(stmt->incdec.expr);
	} break;
	default: {
		assert(!"How did we get here?");
	} break;
	}
}

void gc_mark_scope(Scope *scope) {
	if (scope->gc_marked) return;
	scope->gc_marked = true;

	for (size_t i = 0; i < scope->symbols.cap; i++) {
		MapEntry *e = &scope->symbols.entries[i];
		if (e->hash) {
			gc_mark((Value*)e->val);
		}
	}
	if (scope->parent) {
		gc_mark_scope(scope->parent);
	}
}

void gc_mark(Value *v) {
	if (v->gc_marked) return;
	v->gc_marked = true;

	switch (v->kind) {
	case VALUE_BINOP: {
		gc_mark(v->binary.lhs);
		gc_mark(v->binary.rhs);
	} break;
	case VALUE_UNARY: {
		gc_mark(v->unary.v);
	} break;
	case VALUE_INDEX: {
		gc_mark(v->index.expr);
		gc_mark(v->index.index);
	} break;
	case VALUE_CALL: {
		gc_mark(v->call.expr);
		if (v->call.args.size > 0) {
			Value *arg;
			for_array(v->call.args, arg) {
				gc_mark(arg);
			}
		}
	} break;
	case VALUE_FIELD: {
		gc_mark(v->field.expr);
	} break;
	case VALUE_METHOD_CALL: {
		gc_mark(v->method_call.expr);
		if (v->method_call.args.size > 0) {
			Value *arg;
			for_array(v->method_call.args, arg) {
				gc_mark(arg);
			}
		}
	} break;
	case VALUE_FUNCTION: {
		Function *f = &v->func;
		switch (f->kind) {
		case FUNCTION_NORMAL: {
			Stmt *stmt;
			if (f->normal.stmts.size > 0) {
				for_array(f->normal.stmts, stmt) {
					gc_mark_stmt(stmt);
				}
			}
		} break;
		case FUNCTION_NATIVE: {

		} break;
		case FUNCTION_FFI: {
			assert(!"Nope!");
		}
		}
	} break;
	case VALUE_TABLE: {
		for (size_t i = 0; i < v->table.map.cap; i++) {
			MapEntry *e = &v->table.map.entries[i];
			if (e->hash) {
				gc_mark((Value*)e->val);
			}
		}
	} break;
	case VALUE_INCDEC: {
		gc_mark(v->incdec.expr);
	} break;
	}
}

void gc_mark_all(Ir *ir) {
	if (ir->global_scope && ir->global_scope->symbols.cap > 0) {
		for (size_t i = 0; i < ir->global_scope->symbols.cap; i++) {
			MapEntry *e = &ir->global_scope->symbols.entries[i];
			if (e->hash) {
				gc_mark((Value*)e->val);
			}
		}
	}
	if (ir->file_scope && ir->file_scope->symbols.cap > 0) {
		for (size_t i = 0; i < ir->file_scope->symbols.cap; i++) {
			MapEntry *e = &ir->file_scope->symbols.entries[i];
			if (e->hash) {
				gc_mark((Value*)e->val);
			}
		}
	}
	if (ir->scope_stack.size > 0) {
		Scope *scope;
		for_array(ir->scope_stack, scope) {
			gc_mark_scope(scope);
		}
	}
}

void gc_sweep(Ir *ir) {
	{
		Value **v = &ir->first_value;
		while (*v) {
			if (!(*v)->gc_marked) {
				Value *unreached = *v;
				*v = unreached->next;

				switch (unreached->kind) {
				case VALUE_STRING: {
					//TODO: Replace with string_free
					free(unreached->string.str.str);
				} break;
				case VALUE_TABLE: {
					map_free(&unreached->table.map);
				} break;
				case VALUE_NAME: {
					free(unreached->name.name.str);
				} break;
				case VALUE_FIELD: {
					free(unreached->field.name.str);
				} break;
				case VALUE_METHOD_CALL: {
					free(unreached->method_call.name.str);
					array_free(unreached->method_call.args);
				} break;
				case VALUE_CALL: {
					array_free(unreached->call.args);
				} break;
				case VALUE_TABLE_CONSTANT: {
					array_free(unreached->table_constant.entries);
				} break;
				case VALUE_FUNCTION: {
					free(unreached->func.name.str);
					switch (unreached->func.kind) {
					case FUNCTION_NORMAL: {
						if (unreached->func.normal.arg_names.size > 0) {
							String *str;
							for_array_ref(unreached->func.normal.arg_names, str) {
								free(str->str);
							}
							array_free(unreached->func.normal.arg_names);
						}
						array_free(unreached->func.normal.stmts);
					} break;
					}
				} break;
				}
				free_value(ir, unreached);
				ir->allocated_values--;
			}
			else {
				(*v)->gc_marked = false;
				v = &(*v)->next;
			}
		}
	}

	{
		Scope **scope = &ir->first_scope;
		while (*scope) {
			if (!(*scope)->gc_marked) {
				Scope *unreached = *scope;
				*scope = unreached->next;
				map_free(&unreached->symbols);
				free_scope(ir, unreached);
				ir->allocated_values--;
			}
			else {
				(*scope)->gc_marked = false;
				scope = &(*scope)->next;
			}
		}
	}

	{
		Stmt **stmt = &ir->first_stmt;
		while (*stmt) {
			if (!(*stmt)->gc_marked) {
				Stmt *unreached = *stmt;
				*stmt = unreached->next;
				
				switch(unreached->kind) {
				case STMT_VAR: {
					free(unreached->var.name.str);
				} break;
				case STMT_CALL: {
					array_free(unreached->call.args);
				} break;
				case STMT_METHOD_CALL: {
					array_free(unreached->method_call.args);
					free(unreached->method_call.name.str);
				} break;
				case STMT_BLOCK: {
					array_free(unreached->block.stmts);
				} break;
				}

				free_stmt(ir, unreached);
				ir->allocated_values--;
			}
			else {
				(*stmt)->gc_marked = false;
				stmt = &(*stmt)->next;
			}
		}
	}

	if (0) {
		size_t free_buckets = 0;
		size_t old_buckets = 0;
		Bucket *b = ir->value_pool.free_buckets;
		while (b) {
			free_buckets++;
			b = b->next;
		}

		b = ir->scope_pool.free_buckets;
		while (b) {
			free_buckets++;
			b = b->next;
		}

		b = ir->stmt_pool.free_buckets;
		while (b) {
			free_buckets++;
			b = b->next;
		}

		b = ir->value_pool.old_buckets;
		while (b) {
			old_buckets++;
			b = b->next;
		}

		b = ir->scope_pool.old_buckets;
		while (b) {
			old_buckets++;
			b = b->next;
		}

		b = ir->stmt_pool.old_buckets;
		while (b) {
			old_buckets++;
			b = b->next;
		}

		printf("Old buckets: %d, free buckets: %d\n", (int) old_buckets, (int) free_buckets);
	}
	/*size_t total_free = ir->value_pool.free_count + ir->scope_pool.free_count + ir->stmt_pool.free_count;
	if (ir->allocated_values < total_free) {
		pool_free_freelist(&ir->value_pool);
		pool_free_freelist(&ir->scope_pool);
		pool_free_freelist(&ir->stmt_pool);
	}*/
	/*if (ir->value_pool.buckets > 4096) {
		pool_clear_buckets(&ir->value_pool);
	}
	if (ir->scope_pool.buckets > 128) {
		pool_clear_buckets(&ir->scope_pool);
	}
	if (ir->stmt_pool.buckets > 128) {
		pool_clear_buckets(&ir->stmt_pool);
	}*/
	/*pool_clear_buckets(&ir->value_pool);
	pool_clear_buckets(&ir->scope_pool);
	pool_clear_buckets(&ir->stmt_pool);*/
}

void do_actual_gc(Ir *ir) {
	//TODO: Split between couting values,scopes and stmts
	//TODO: Add pool allocators for values,scopes and stms and have the gc reuse freed pools.
	int values_before_gc = ir->allocated_values;

	gc_mark_all(ir);
	gc_sweep(ir);

	ir->max_allocated_values = ir->allocated_values * 2;
	//printf("Values before gc: %d\n", values_before_gc);
	//printf("Values after  gc: %d\n", ir->allocated_values);
}

void do_gc(Ir *ir) {
	if (ir->allocated_values < ir->max_allocated_values) return;
	if (!ir->do_gc) return;

	do_actual_gc(ir);
}

void force_gc(Ir *ir) {
	do_actual_gc(ir);
}
#endif

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

Value* make_native_function(Ir *ir, String name, Value* (*func)(Ir *ir, ValueArray args)) {
	Value *v = alloc_value(ir);
	v->kind = VALUE_FUNCTION;
	v->func.kind = FUNCTION_NATIVE;
	v->func.native.function = func;
	v->func.name = make_string_copy(name);
	return v;
}

Stmt* alloc_stmt(Ir *ir, SourceLoc loc) {
	Stmt *stmt = pool_alloc(&ir->stmt_pool);

	stmt->gc.gc_kind = GC_STMT;
	stmt->gc.color = GC_GREY;
	stmt->gc.next = ir->grey_list;
	stmt->gc.prev = 0;
	ir->grey_list = (GCObject*)stmt;

	stmt->loc = loc;
	return stmt;
}

void free_stmt(Ir *ir, Stmt *stmt) {
	pool_release(&ir->stmt_pool, stmt);
}

Stmt* convert_node_to_stmt(Ir *ir, Node *n) {
	switch (n->kind) {
	case NODE_VAR: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_VAR;
		stmt->var.name = make_string_copy(n->var.name);
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
	case NODE_METHOD_CALL: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_METHOD_CALL;
		stmt->method_call.expr = expr_to_value(ir, n->method_call.expr);
		stmt->method_call.name = make_string_copy(n->method_call.name);
		if (n->method_call.args.size > 0) {
			Node *arg;
			for_array(n->method_call.args, arg) {
				array_add(stmt->method_call.args, expr_to_value(ir, arg));
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
	case NODE_INCDEC: {
		Stmt *stmt = alloc_stmt(ir, n->loc);
		stmt->kind = STMT_ASSIGN;
		stmt->assign.left = expr_to_value(ir, n->incdec.expr);

		Value *binop = alloc_value(ir);
		binop->kind = VALUE_BINOP;
		if (n->incdec.op == TOKEN_INCREMENT) {
			binop->binary.op = TOKEN_PLUS;
		} else if (n->incdec.op == TOKEN_DECREMENT) {
			binop->binary.op = TOKEN_MINUS;
		}
		else {
			assert(!"Invalid incdec op");
		}
		binop->binary.lhs = expr_to_value(ir, n->incdec.expr);
		binop->binary.rhs = make_number_value(ir, 1);
		stmt->assign.right = binop;

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

void import_gfx(Ir *ir); // In gfx.c
void ir_use_library(Ir *ir, String name) {
	if (strings_match(string("gfx"), name)) {
		import_gfx(ir);
	}
	else {
		ir_error(ir, "Unknown library name after use: %.*s", (int)name.len, name.str);
	}
}

void ir_import_file(Ir *ir, String path, String as);
void convert_top_levels_to_ir(Ir *ir, Scope *scope, NodeArray stmts) {
	Node *n;
	for_array(stmts, n) {
		ir->loc = n->loc;
		switch (n->kind) {
		case NODE_IMPORT: {
			ir_import_file(ir, n->import.name, n->import.as);
		} break;
		case NODE_USE: {
			ir_use_library(ir, n->use.name);
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
			f->name = make_string_copy(n->func.name);
			f->loc = n->loc;
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

void ir_import_file(Ir *ir, String path, String as) {
	Parser p;
	memset(&p, 0, sizeof(Parser));
	init_parser(&p, path);
	NodeArray stmts = parse(&p);

	assert(as.str == 0 && as.len == 0); //TODO: Implement namespace
	//TODO: Handle recursive imports
	//TODO: Give every file its own scope, that way, another file cant access our variables etc.
	convert_top_levels_to_ir(ir, ir->file_scope, stmts);
}

void gc_mark(Ir *ir);
void init_ir(Ir *ir, NodeArray stmts) {
	pool_init(&ir->value_pool, sizeof(Value), 4096);
	pool_init(&ir->scope_pool, sizeof(Scope), 128);
	pool_init(&ir->stmt_pool, sizeof(Stmt), 128);
	
	ir->do_gc = false;
	ir->max_allocated_values = 1024;
	ir->allocated_values = 0;
	
	ir->white_list = 0;
	ir->grey_list = 0;
	ir->black_list = 0;

	ir->global_scope = make_scope(ir, 0);
	ir->file_scope = make_scope(ir, ir->global_scope);
	convert_top_levels_to_ir(ir, ir->file_scope, stmts);

	add_globals(ir);

	// printf("sizeof(Value): %d\n", (int)sizeof(Value));

	ir->do_gc = true;

	gc_mark(ir);
}

void do_assign(Ir *ir, Scope *scope, Value *lhs, Value *rhs) {
	if (lhs->kind != VALUE_NAME && lhs->kind != VALUE_FIELD && lhs->kind != VALUE_INDEX) {
		// Error not supported assignemtn or something else?
		lhs = eval_value(ir, scope, lhs);
	}
	if (lhs->kind != VALUE_NAME && lhs->kind != VALUE_FIELD && lhs->kind != VALUE_INDEX) {
		ir_error(ir, "Cannot assign to left hand");
	}

	switch (lhs->kind) {
	case VALUE_NAME: {
		rhs = eval_value(ir, scope, rhs);
		scope_set(ir, scope, lhs->name.name, rhs);
	} break;
	case VALUE_FIELD: {
		Value *expr = eval_value(ir, scope, lhs->field.expr);
		rhs = eval_value(ir, scope, rhs);
		table_put_name(ir, expr, lhs->field.name, rhs);
	} break;
	case VALUE_INDEX: {
		Value *expr = eval_value(ir, scope, lhs->index.expr);
		Value *index = eval_value(ir, scope, lhs->index.index);
		rhs = eval_value(ir, scope, rhs);
		table_put(ir, expr, index, rhs);
	} break;
	default: {
		ir_error(ir, "Cannot assign to left hand");
	} break;
	}
}

// True if we had a return,break,continue, etc
bool eval_stmt(Ir *ir, Scope *scope, Stmt *stmt, Value **return_value) {
	ir->loc = stmt->loc;
	//do_gc(ir);
	gc_do_greys(ir);
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
		do_assign(ir, scope, stmt->assign.left, stmt->assign.right);
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
		call_function(ir, func, args, false);
		array_free(args);
	} break;
	case STMT_METHOD_CALL: {
		Value *table = eval_value(ir, scope, stmt->method_call.expr);
		if (!istable(table)) {
			ir_error(ir, "':' operator only works with tables as lvalues");
		}

		//TODO: Give error if value from table is null
		Value *func = table_get_name(ir, table, stmt->method_call.name);
		if (!func) {
			ir_error(ir, "Table does not contain any value called: %.*s", (int)stmt->method_call.name.len, stmt->method_call.name.str);
		}
		if (!isfunction(func)) {
			ir_error(ir, "Right hand side of ':' operator is not a function");
		}

		ValueArray args = { 0 };
		array_add(args, table);
		if (stmt->method_call.args.size > 0) {
			Value *arg;
			for_array(stmt->method_call.args, arg) {
				Value *v = eval_value(ir, scope, arg);
				array_add(args, v);
			}
		}

		call_function(ir, func, args, true);
		array_free(args);
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
			Scope *block_scope = push_scope(ir, scope);
			Stmt *block_stmt;
			for_array(stmt->block.stmts, block_stmt) {
				bool returned = eval_stmt(ir, block_scope, block_stmt, return_value);
				if (returned) {
					pop_scope(ir);
					return returned;
				}
			}
			pop_scope(ir);
		}
	} break;
	default: {
		assert(!"Unhandled stmt!");
		exit(1);
	} break;
	}

	return false;
}

Value* eval_function(Ir *ir, Function func, ValueArray args, bool is_method_call) {
	// Check that we recieved the right amount of args
	// Register args with appropiate names
	// Eval all stmts
	Value *return_value = null_value;

	if (func.normal.arg_names.size != args.size) {
		if (is_method_call) {
			ir_error(ir, "Argument count mismatch! Wanted %d got %d. This function was called as a method which means the left hand side of ':' is passed as the first argument.", (int)func.normal.arg_names.size, (int)args.size);
		}
		else {
			ir_error(ir, "Argument count mismatch! Wanted %d got %d.", (int)func.normal.arg_names.size, (int)args.size);
		}
	}

	if (func.normal.stmts.size > 0) {
		Scope *scope = push_scope(ir, ir->file_scope);
		for (int i = 0; i < args.size; i++) {
			scope_add(ir, scope, func.normal.arg_names.data[i], args.data[i]);
		}
		Stmt *stmt;
		for_array(func.normal.stmts, stmt) {
			bool returned = eval_stmt(ir, scope, stmt, &return_value);
			if (returned) {
				pop_scope(ir);
				return return_value;
			}
		}

		pop_scope(ir);
	}

	return return_value;
}

Value* call_function(Ir *ir, Value *func_value, ValueArray args, bool is_method_call) {
	assert(func_value->kind == VALUE_FUNCTION);

	Value *return_value = null_value;

	Function func = func_value->func;
	StackCall call = { 0 };
	call.loc = func.loc;
	call.kind = func.kind;
	call.name = make_string_copy(func.name);
	push_call(ir, call);
	switch (func.kind) {
	case FUNCTION_NORMAL: {
		return_value = eval_function(ir, func, args, is_method_call);
	} break;
	case FUNCTION_NATIVE: {
		assert(func.native.function);
		return_value = (*func.native.function)(ir, args);
	} break;
	case FUNCTION_FFI: {
		assert(!"Not implemented");
	} break;
	}

	pop_call(ir);
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
	case TOKEN_NOT: {
		res->number.value = !rhs->number.value;
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
		else {
			ir_error(ir, "Operator '%s' only work with numbers and strings.", token_kind_to_string(op));
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
		if (!isfunction(func)) {
			ir_error(ir, "Tried to call non-function value");
		}
		ValueArray args = { 0 };
		if (v->call.args.size > 0) {
			Value *arg;
			for_array(v->call.args, arg) {
				Value *v = eval_value(ir, scope, arg);
				array_add(args, v);
			}
		}
		Value *ret = eval_value(ir, scope, call_function(ir, func, args, false));
		array_free(args);
		return ret;
	} break;
	case VALUE_METHOD_CALL: {
		Value *table = eval_value(ir, scope, v->method_call.expr);
		if (!istable(table)) {
			ir_error(ir, "':' operator only works with tables as lvalues");
		}

		Value *func = table_get_name(ir, table, v->method_call.name); // Should return null for non existing values
		if (!func) {
			ir_error(ir, "Table does not contain any value called: %.*s", (int)v->method_call.name.len, v->method_call.name.str);
		}
		if (!isfunction(func)) {
			ir_error(ir, "Right hand side of ':' operator is not a function");
		}

		ValueArray args = { 0 };
		array_add(args, table);
		if (v->method_call.args.size > 0) {
			Value *arg;
			for_array(v->method_call.args, arg) {
				Value *v = eval_value(ir, scope, arg);
				array_add(args, v);
			}
		}

		Value *result = call_function(ir, func, args, true);
		array_free(args);
		return eval_value(ir, scope, result);
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
	case VALUE_INCDEC: {
		Value *lhs = v->incdec.expr;
		if (lhs->kind != VALUE_NAME && lhs->kind != VALUE_FIELD && lhs->kind != VALUE_INDEX) {
			// Error not supported assignemtn or something else?
			lhs = eval_value(ir, scope, v->incdec.expr);
		}
		if (lhs->kind != VALUE_NAME && lhs->kind != VALUE_FIELD && lhs->kind != VALUE_INDEX) {
			ir_error(ir, "Cannot assign to left hand");
		}

		Value *lhs_value = eval_value(ir, scope, lhs);
		Value *to_assign = 0;
		if (v->incdec.op == TOKEN_INCREMENT) {
			to_assign = eval_binop(ir, scope, TOKEN_PLUS, lhs_value, make_number_value(ir, 1));
		}
		else if (v->incdec.op == TOKEN_INCREMENT) {
			to_assign = eval_binop(ir, scope, TOKEN_MINUS, lhs_value, make_number_value(ir, 1));
		}
		else {
			assert(!"Invalid incdec op");
		}

		Value *result = 0;
		if (v->incdec.post) {
			result = eval_value(ir, scope, lhs);
		}
		else {
			result = eval_value(ir, scope, to_assign);
		}

		do_assign(ir, scope, lhs, to_assign);

		return result;
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
		v->string.str = make_string_copy(n->string.string);
		return v;
	} break;
	case NODE_NAME: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_NAME;
		v->name.name = make_string_copy(n->name.name);
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
		v->field.name = make_string_copy(n->field.name);
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
	case NODE_METHOD_CALL: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_METHOD_CALL;
		v->method_call.expr = expr_to_value(ir, n->method_call.expr);
		v->method_call.name = make_string_copy(n->method_call.name);
		if (n->method_call.args.size > 0) {
			Node *arg;
			for_array(n->method_call.args, arg) {
				array_add(v->method_call.args, expr_to_value(ir, arg));
			}
		}
		return v;
	} break;
	case NODE_ANON_FUNC: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_FUNCTION;
		v->func.kind = FUNCTION_NORMAL;
		v->func.name = make_string_slow("<anonymous func>");
		v->func.loc = n->loc;

		if (n->anon_func.args.size > 0) {
			String *str;
			for_array_ref(n->anon_func.args, str) {
				array_add(v->func.normal.arg_names, make_string_slow_len(str->str, str->len));
			}
		}
		v->func.normal.stmts = convert_nodes_to_stmts(ir, n->anon_func.block->block.stmts);
		return v;
	} break;
	case NODE_INCDEC: {
		Value *v = alloc_value(ir);
		v->kind = VALUE_INCDEC;
		v->incdec.expr = expr_to_value(ir, n->incdec.expr);
		v->incdec.op = n->incdec.op;
		v->incdec.post = n->incdec.post;
		return v;
	} break;
	default: {
		assert(!"Node to value conversion not added");
		exit(1);
	} break;
	}
}

Value* ir_run(Ir *ir, size_t argc, char **argv) {
	ValueArray args = { 0 };
	Value *arg_table = alloc_value(ir);
	arg_table->kind = VALUE_TABLE;
	array_add(args, arg_table);

	if (argc > 0) {
		for (size_t i = 0; i < argc; i++) {
			if (argv[i]) {
				table_put(ir, arg_table, make_number_value(ir, (double)i), make_string_value(ir, make_string_slow(argv[i])));
			}
		}
	}
	Value *main_func = scope_get(ir, ir->file_scope, string("main"));
	return call_function(ir, main_func, args, false);
}
