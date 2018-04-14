typedef enum NodeKind {
	NODE_UNKNOWN = 0,
	NODE_NUMBER,
	NODE_NAME,
	NODE_STRING,
	NODE_BINOP,
	NODE_UNARY,
	NODE_FIELD,
	NODE_CALL,
	NODE_RETURN,
	NODE_FUNC,
	NODE_VAR,
	NODE_CONTINUE,
	NODE_BREAK,
	NODE_FOR,
	NODE_WHILE,
	NODE_IF,
	NODE_INDEX,
	NODE_ASSIGN,
	NODE_BLOCK,
	NODE_TABLE,
	NODE_NULL,
	NODE_IMPORT,
} NodeKind;

typedef enum TableEntryKind {
	ENTRY_NORMAL, // { expr, expr, expr }
	ENTRY_KEY,    // { expr = expr, expr = expr }
	ENTRY_INDEX,  // { [expr] = expr, [expr] = expr}
} TableEntryKind;

typedef struct Node Node;
typedef struct TableEntry {
	TableEntryKind kind;
	Node *expr;
	union {
		Node *key;
		Node *index;
	};
} TableEntry;
typedef Array(TableEntry) TableEntryArray;

typedef struct Node Node;
typedef Array(Node*)  NodeArray;
struct Node {
	NodeKind kind;
	SourceLoc loc;
	union {
		struct {
			double value;
		} number;
		struct {
			String name;
		} name;
		struct {
			String string;
		} string;
		struct {
			TokenKind op;
			Node *lhs;
			Node *rhs;
		} binary;
		struct {
			TokenKind op;
			Node *rhs;
		} unary;
		struct {
			Node *expr;
			String name;
		} field;
		struct {
			Node *expr;
			NodeArray args;
		} call;
		struct {
			Node *expr;
		} ret;
		struct {
			String name;
			StringArray args;
			Node *block;
		} func;
		struct {
			String name;
			Node *expr;
		} var;
		struct { int unused;  } _continue;
		struct { int unused; } _break;
		struct {
			Node *init;
			Node *cond;
			Node *next;
			Node *block;
		} _for;
		struct {
			Node *cond;
			Node *block;
		} _while;
		struct {
			Node *cond;
			Node *block;
			Node *else_block; // Can be another if node
		} _if;
		struct {
			Node *expr;
			Node *index;
		} index;
		struct {
			Node *left;
			Node *right;
		} assign;
		struct {
			NodeArray stmts;
		} block;
		struct {
			TableEntryArray entries;
		} table;
		struct { int unsued;  } _null;
		struct {
			String name;
		} import;
	};
};

typedef Array(Node) NodeMemory;
typedef struct Parser {
	Lexer lexer;
	Token current_token;
	int token_offset;
	NodeMemory nodes;
} Parser;

void init_parser_common(Parser *p) {
	lex(&p->lexer);
	p->token_offset = 0;
	p->current_token = p->lexer.tokens.data[p->token_offset];
	array_init(p->nodes, 32);
}

void init_parser(Parser *p, String path) {
	init_lexer(&p->lexer, path);
	init_parser_common(p);
}

void init_parser_from_string(Parser *p, char *str) {
	init_lexer_from_string(&p->lexer, str);
	init_parser_common(p);
}

Node* alloc_node(Parser *p) {
	//TODO: Implement arena allocator
	return calloc(1, sizeof(Node));
	//Node n = { 0 };
	//array_add(p->nodes, n);
	//return (Node*)&p->nodes.data[p->nodes.size-1];
}

Node* make_number(Parser *p, Token number) {
	assert(number.kind == TOKEN_NUMBER);
	Node *n = alloc_node(p);

	n->loc = number.loc;

	n->kind = NODE_NUMBER;
	n->number.value = number.number_value;

	return n;
}

Node* make_name(Parser *p, Token name) {
	assert(name.kind == TOKEN_IDENT);
	Node *n = alloc_node(p);

	n->loc = name.loc;

	n->kind = NODE_NAME;
	n->name.name = name.lexeme;

	return n;
}

Node* make_string(Parser *p, Token str) {
	assert(str.kind == TOKEN_STRING);
	Node *n = alloc_node(p);

	n->loc = str.loc;

	n->kind = NODE_STRING;
	n->string.string = str.lexeme;
	
	return n;
}

Node* make_null(Parser *p, Token t) {
	assert(t.kind = TOKEN_NULL);
	Node *n = alloc_node(p);

	n->loc = t.loc;

	n->kind = NODE_NULL;
	
	return n;
}

Node* make_binary(Parser *p, Token op, Node *lhs, Node *rhs) {
	assert(lhs);
	assert(rhs);
	Node *n = alloc_node(p);

	n->loc = op.loc;

	n->kind = NODE_BINOP;
	n->binary.lhs = lhs;
	n->binary.rhs = rhs;
	n->binary.op = op.kind;

	return n;
}

Node* make_unary(Parser *p, Token op, Node *rhs) {
	assert(op.kind == TOKEN_PLUS || op.kind == TOKEN_MINUS);
	assert(rhs);
	Node *n = alloc_node(p);

	n->loc = op.loc;

	n->kind = NODE_UNARY;
	n->unary.op = op.kind;
	n->unary.rhs = rhs;

	return n;
}

Node* make_index(Parser *p, Token op, Node *expr, Node *index_expr) {
	assert(expr);
	assert(index_expr);
	Node *n = alloc_node(p);

	n->loc = op.loc;

	n->kind = NODE_INDEX;
	n->index.expr = expr;
	n->index.index = index_expr;

	return n;
}

Node* make_field(Parser *p, Token field, Node *expr) {
	assert(expr);
	Node *n = alloc_node(p);

	n->loc = field.loc;

	n->kind = NODE_FIELD;
	n->field.expr = expr;
	n->field.name = field.lexeme;

	return n;
}

Node* make_call(Parser *p, Token lpar, Node *expr, NodeArray args) {
	Node *n = alloc_node(p);
	
	n->loc = lpar.loc;

	n->kind = NODE_CALL;
	n->call.expr = expr;
	n->call.args = args;

	return n;
}

Node* make_var(Parser *p, Token var, String name, Node *init) {
	Node *n = alloc_node(p);

	n->loc = var.loc;

	n->kind = NODE_VAR;
	n->var.name = name;
	n->var.expr = init;

	return n;
}

Node* make_if(Parser *p, Token _if, Node *cond, Node *block, Node *else_block) {
	assert(cond);
	assert(block);
	Node *n = alloc_node(p);

	n->loc = _if.loc;

	n->kind = NODE_IF;
	n->_if.cond = cond;
	n->_if.block = block;
	n->_if.else_block = else_block;

	return n;
}

Node* make_block(Parser *p, Token t, NodeArray stmts) {
	Node *n = alloc_node(p);
	
	n->loc = t.loc;

	n->kind = NODE_BLOCK;
	n->block.stmts = stmts;

	return n;
}

Node* make_assign(Parser *p, Token op, Node *expr, Node *value) {
	assert(expr);
	assert(value);
	assert(op.kind == TOKEN_EQUAL);
	Node *n = alloc_node(p);

	n->loc = op.loc;

	n->kind = NODE_ASSIGN;
	n->assign.left = expr;
	n->assign.right = value;

	return n;
}

Node* make_return(Parser *p, Token ret, Node *expr) {
	Node *n = alloc_node(p);
	
	n->loc = ret.loc;

	n->kind = NODE_RETURN;
	n->ret.expr = expr;

	return n;
}

Node* make_continue(Parser *p, Token cont) {
	Node *n = alloc_node(p);

	n->loc = cont.loc;

	n->kind = NODE_CONTINUE;

	return n;
}

Node* make_break(Parser *p, Token cont) {
	Node *n = alloc_node(p);

	n->loc = cont.loc;

	n->kind = NODE_BREAK;

	return n;
}

Node* make_while(Parser *p, Token _while, Node *cond, Node *block) {
	assert(cond);
	assert(block);
	Node *n = alloc_node(p);

	n->loc = _while.loc;

	n->kind = NODE_WHILE;
	n->_while.cond = cond;
	n->_while.block = block;

	return n;
}

Node* make_func(Parser *p, Token func, String name, StringArray args, Node *block) {
	assert(block && block->kind == NODE_BLOCK);
	Node *n = alloc_node(p);

	n->loc = func.loc;

	n->kind = NODE_FUNC;
	n->func.name = name;
	n->func.args = args;
	n->func.block = block;

	return n;
}

Node* make_table(Parser *p, Token t, TableEntryArray entries) {
	Node *n = alloc_node(p);

	n->loc = t.loc;

	n->kind = NODE_TABLE;
	n->table.entries = entries;

	return n;
}

#ifdef _WIN32
__declspec(noreturn)
#else
__attribute__((noreturn))
#endif
void parser_error(Parser *parser, char *format, ...) {
	printf("%s(%lld, %lld): ", parser->lexer.file.str, parser->lexer.line, parser->lexer.offset);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
	assert(!"parser_error"); // Assert while developing to allow for better debugging
	//exit(1);
}

Token next_token(Parser *p) {
	p->token_offset++;
	p->current_token = p->lexer.tokens.data[p->token_offset];
	return p->current_token;
}

// Check current
bool is_token(Parser *p, TokenKind kind) {
	if (p->current_token.kind == kind) {
		return true;
	}
	return false;
}

// Check current and eat token if true
bool match_token(Parser *p, TokenKind kind) {
	if (p->current_token.kind == kind) {
		next_token(p);
		return true;
	}
	return false;
}

void expect(Parser *p, TokenKind kind) {
	if (match_token(p, kind)) {
		return;
	}
	parser_error(p, "Expected '%s' but got '%s'!", token_kind_to_string(kind), token_kind_to_string(p->current_token.kind));
}

#define IncompletePath() do { \
	assert(!"IncompletePath"); \
	return 0; \
} while (0);

Node* parse_expr(Parser *p);

Node* expr_operand(Parser *p) {
	Token t = p->current_token;
	if (match_token(p, TOKEN_NUMBER)) {
		return make_number(p, t);
	}
	else if (match_token(p, TOKEN_IDENT)) {
		return make_name(p, t);
	}
	else if (match_token(p, TOKEN_STRING)) {
		return make_string(p, t);
	}
	else if (match_token(p, TOKEN_NULL)) {
		return make_null(p, t);
	}
	else if (match_token(p, TOKEN_LEFTPAR)) {
		// ( 
		Node *expr = parse_expr(p);
		expect(p, TOKEN_RIGHTPAR);
		return expr;
	}
	else if (match_token(p, TOKEN_LEFTBRACE)) {
		// {
		TableEntryArray entries = { 0 };

		if (match_token(p, TOKEN_RIGHTBRACE)) {
			return make_table(p, t, entries);
		}

		do {
			if (is_token(p, TOKEN_RIGHTBRACE)) {
				// We allow one trailing comma
				break;
			}

			if (match_token(p, TOKEN_LEFTBRACKET)) {
				Node *index = parse_expr(p);
				expect(p, TOKEN_RIGHTBRACKET);

				expect(p, TOKEN_EQUAL);

				Node *value = parse_expr(p);
				TableEntry entry = (TableEntry) { .kind = ENTRY_INDEX, .key = index, .expr = value};
				array_add(entries, entry);
			}
			else {
				Node *expr = parse_expr(p);

				if (match_token(p, TOKEN_EQUAL)) {
					Node *value = parse_expr(p);
					TableEntry entry = (TableEntry) { .kind = ENTRY_KEY, .key = expr, .expr = value };
					array_add(entries, entry);
				}
				else {
					TableEntry entry = (TableEntry) { .kind = ENTRY_NORMAL, .key = 0, .expr = expr };
					array_add(entries, entry);
				}
			}
		} while (!is_token(p, TOKEN_RIGHTBRACE) && match_token(p, TOKEN_COMMA));
		if (!match_token(p, TOKEN_RIGHTBRACE)) {
			//expect(p, TOKEN_RIGHTBRACE);
			parser_error(p, "Expected ',' or '}' but got '%s'!", token_kind_to_string(p->current_token.kind));
		}

		return make_table(p, t, entries);
	}
	else {
		parser_error(p, "Unexpected token: %s", token_kind_to_string(p->current_token.kind));
	}
}

Node* expr_base(Parser *p) {
	Node *expr = expr_operand(p);

	while (is_token(p, TOKEN_LEFTPAR) || is_token(p, TOKEN_LEFTBRACKET) || is_token(p, TOKEN_DOT)) {
		Token op = p->current_token; // Used to set the location of the nodes

		if (match_token(p, TOKEN_LEFTBRACKET)) {
			// expr[
			Node *index_expr = parse_expr(p);
			expect(p, TOKEN_RIGHTBRACKET);
			expr = make_index(p, op, expr, index_expr);
		}
		else if (match_token(p, TOKEN_LEFTPAR)) {
			// expr(
			NodeArray args = { 0 };
			if (match_token(p, TOKEN_RIGHTPAR)) {
				// expr()
				expr = make_call(p, op, expr, args);
			}
			else {
				// expr(...
				do {
					Node *arg_expr = parse_expr(p);
					array_add(args, arg_expr);
				} while (match_token(p, TOKEN_COMMA));
				expect(p, TOKEN_RIGHTPAR);

				expr = make_call(p, op, expr, args);
			}
		}
		else if (match_token(p, TOKEN_DOT)) {
			// expr.t
			Token t = p->current_token;
			expect(p, TOKEN_IDENT);
			expr = make_field(p, t, expr);
		}
		else {
			assert(!"Unsynced while cond and ifs");
		}
	}

	return expr;
}

Node* expr_unary(Parser *p) {
	bool is_unary = false;
	Token op = p->current_token;
	if (is_token(p, TOKEN_PLUS) || is_token(p, TOKEN_MINUS)) {
		is_unary = true;
		next_token(p);
	}
	Node *rhs = expr_base(p);

	if (is_unary) {
		rhs = make_unary(p, op, rhs);
	}

	return rhs;
}

Node* expr_mul(Parser *p) {
	Node *lhs = expr_unary(p);

	while (is_token(p, TOKEN_ASTERISK) ||
		   is_token(p, TOKEN_SLASH) ||
		   is_token(p, TOKEN_MOD)
	){
		Token op = p->current_token;
		next_token(p);
		Node *rhs = expr_unary(p);

		lhs = make_binary(p, op, lhs, rhs);
	}

	return lhs;
}

Node* expr_plus(Parser *p) {
	Node *lhs = expr_mul(p);

	while (is_token(p, TOKEN_PLUS) ||
		   is_token(p, TOKEN_MINUS)
	) {
		Token op = p->current_token;
		next_token(p);
		Node *rhs = expr_mul(p);

		lhs = make_binary(p, op, lhs, rhs);
	}

	return lhs;
}

Node* expr_equality(Parser *p) {
	Node *lhs = expr_plus(p);

	while (
		is_token(p, TOKEN_EQUALS) ||
	    is_token(p, TOKEN_NE) ||
		is_token(p, TOKEN_GT) ||
		is_token(p, TOKEN_GTE) ||
		is_token(p, TOKEN_LT) ||
		is_token(p, TOKEN_LTE)
	) {
		Token op = p->current_token;
		next_token(p);
		Node *rhs = expr_plus(p);

		lhs = make_binary(p, op, lhs, rhs);
	}

	return lhs;
}

Node* expr_land(Parser *p) {
	Node *lhs = expr_equality(p);

	while (is_token(p, TOKEN_LAND)) {
		Token op = p->current_token;
		next_token(p);
		Node *rhs = expr_equality(p);

		lhs = make_binary(p, op, lhs, rhs);
	}

	return lhs;
}

Node* expr_lor(Parser *p) {
	Node *lhs = expr_land(p);

	while (is_token(p, TOKEN_LOR)) {
		Token op = p->current_token;
		next_token(p);
		Node *rhs = expr_land(p);

		lhs = make_binary(p, op, lhs, rhs);
	}

	return lhs;
}

// GO-like precedence : https://kuree.gitbooks.io/the-go-programming-language-report/content/31/text.html
Node* parse_expr(Parser *p) {
	return expr_lor(p);
}

Node* parse_var(Parser *p) {
	Token var = p->current_token;
	if (match_token(p, TOKEN_VAR)) {
		Token name = p->current_token;
		expect(p, TOKEN_IDENT);

		Node *expr = 0;
		if (match_token(p, TOKEN_EQUAL)) {
			expr = parse_expr(p);
		}
		expect(p, TOKEN_SEMICOLON);

		return make_var(p, var, name.lexeme, expr);
	}
	else {
		assert(!"parse_var was called but TOKEN_VAR was not the current token!");
		exit(1);
	}
}

Node* parse_return(Parser *p) {
	Token ret = p->current_token;
	if (match_token(p, TOKEN_RETURN)) {
		Node *expr = 0;
		if (!is_token(p, TOKEN_SEMICOLON)) {
			expr = parse_expr(p);
		}
		expect(p, TOKEN_SEMICOLON);

		return make_return(p, ret, expr);
	}
	else {
		assert(!"parse_return was called but TOKEN_RETURN was not the current token!");
		exit(1);
	}
}

Node* parse_continue(Parser *p) {
	Token cont = p->current_token;
	if (match_token(p, TOKEN_CONTINUE)) {
		expect(p, TOKEN_SEMICOLON);
		return make_continue(p, cont);
	}
	else {
		assert(!"parse_continue was called but TOKEN_CONTINUE was not the current token!");
		exit(1);
	}
}

Node* parse_break(Parser *p) {
	Token cont = p->current_token;
	if (match_token(p, TOKEN_BREAK)) {
		expect(p, TOKEN_SEMICOLON);
		return make_break(p, cont);
	}
	else {
		assert(!"parse_break was called but TOKEN_BREAK was not the current token!");
		exit(1);
	}
}

Node* parse_while(Parser *p);
Node* parse_if(Parser *p);
Node* parse_block(Parser *p) {
	Token lbrace = p->current_token;
	if (match_token(p, TOKEN_LEFTBRACE)) {
		NodeArray stmts = {0};
		//TODO: for
		do {
			Node *stmt = 0;
			if (is_token(p, TOKEN_IF)) {
				stmt = parse_if(p);
			}
			else if (is_token(p, TOKEN_VAR)) {
				stmt = parse_var(p);
			}
			else if (is_token(p, TOKEN_LEFTBRACE)) {
				stmt = parse_block(p);
			}
			else if (is_token(p, TOKEN_RETURN)) {
				stmt = parse_return(p);
			}
			else if (is_token(p, TOKEN_CONTINUE)) {
				stmt = parse_continue(p);
			}
			else if (is_token(p, TOKEN_BREAK)) {
				stmt = parse_break(p);
			}
			else if (is_token(p, TOKEN_WHILE)) {
				stmt = parse_while(p);
			}
			else if (match_token(p, TOKEN_SEMICOLON)) {
				continue;
			}
			else if (is_token(p, TOKEN_RIGHTBRACE)) {
				break;
			}
			else {
				Node *expr = parse_expr(p);

				Token op = p->current_token;
				if (match_token(p, TOKEN_EQUAL)) {
					Node *value = parse_expr(p);
					expect(p, TOKEN_SEMICOLON);
					stmt = make_assign(p, op, expr, value);
				}
				else {
					if (expr->kind == NODE_CALL) {
						expect(p, TOKEN_SEMICOLON);
						stmt = expr;
					}
					else {
						parser_error(p, "Unexpected expression in block!");
					}
				}
			}

			array_add(stmts, stmt);
		} while (!is_token(p, TOKEN_RIGHTBRACE));
		expect(p, TOKEN_RIGHTBRACE);
		
		return make_block(p, lbrace, stmts);
	}
	else {
		parser_error(p, "Expected '%s' got '%s'", token_kind_to_string(TOKEN_LEFTBRACE), token_kind_to_string(p->current_token.kind));
	}
}

Node* parse_while(Parser *p) {
	Token _while = p->current_token;
	if (match_token(p, TOKEN_WHILE)) {
		Node *cond = parse_expr(p);
		Node *block = parse_block(p);
		return make_while(p, _while, cond, block);
	} else {
		assert(!"parse_while was called but TOKEN_WHILE was not the current token!");
		exit(1);
	}
}

Node* parse_if(Parser *p) {
	Token _if = p->current_token;
	if (match_token(p, TOKEN_IF)) {
		Node *cond = parse_expr(p);
		Node *block = parse_block(p);
		Node *else_block = 0;

		if (match_token(p, TOKEN_ELSE)) {
			if (is_token(p, TOKEN_LEFTBRACE)) {
				else_block = parse_block(p);
			}
			else if (is_token(p, TOKEN_IF)) {
				else_block = parse_if(p);
			}
			else {
				parser_error(p, "Expected if or block after else");
			}
		}

		return make_if(p, _if, cond, block, else_block);
	}
	else {
		assert(!"parse_if was called but TOKEN_IF was not the current token!");
		exit(1);
	}
}

Node* parse_func(Parser *p) {
	Token func = p->current_token;
	if (match_token(p, TOKEN_FUNC)) {
		String name = p->current_token.lexeme;
		if (!match_token(p, TOKEN_IDENT)) {
			parser_error(p, "Expected identifier after 'func'");
		}

		expect(p, TOKEN_LEFTPAR);
		StringArray args = { 0 };

		if (!is_token(p, TOKEN_RIGHTPAR)) {
			do {
				if (is_token(p, TOKEN_IDENT)) {
					String arg = p->current_token.lexeme;
					next_token(p);
					array_add(args, arg);
				}
				else {
					parser_error(p, "Expected identifier while parsing argument list got '%s'", token_kind_to_string(p->current_token.kind));
				}
			} while (match_token(p, TOKEN_COMMA));
		}
		expect(p, TOKEN_RIGHTPAR);

		Node *block = parse_block(p);

		return make_func(p, func, name, args, block);
	}
	else {
		assert(!"parse_func was called but TOKEN_FUNC was not the current token!");
		exit(1);
	}
}

Node* parse_import(Parser *p) {
	if (match_token(p, TOKEN_IMPORT)) {
		Token name = p->current_token;
		expect(p, TOKEN_STRING);
		expect(p, TOKEN_SEMICOLON);

		Node *n = alloc_node(p);
		n->kind = NODE_IMPORT;
		n->import.name = name.lexeme;

		

		return n;
	}
	else {
		assert(!"parse_import was called but TOKEN_IMPORT was not the current token!");
		exit(1);
	}
}

NodeArray parse(Parser *p) {
	NodeArray stmts = { 0 };

	do {
		Node *stmt = 0;

		if (is_token(p, TOKEN_VAR)) {
			stmt = parse_var(p);
		}
		else if (is_token(p, TOKEN_FUNC)) {
			stmt = parse_func(p);
		}
		else if (is_token(p, TOKEN_IMPORT)) {
			stmt = parse_import(p);
		}
		else if (match_token(p, TOKEN_SEMICOLON)) {
			continue;
		}
		else {
			parser_error(p, "Unexpected token: '%s'", token_kind_to_string(p->current_token.kind));
		}

		array_add(stmts, stmt);
	} while (!is_token(p, TOKEN_EOF));

	return stmts;
}