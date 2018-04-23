#ifndef BS_PARSER_H
#define BS_PARSER_H

#include "common.h"
#include "lexer.h"

typedef enum NodeKind {
	NODE_UNKNOWN = 0,
	NODE_NUMBER,
	NODE_NAME,
	NODE_STRING,
	NODE_BINOP,
	NODE_UNARY,
	NODE_FIELD,
	NODE_CALL,
	NODE_METHOD_CALL,
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
	NODE_USE,
	NODE_ANON_FUNC,
	NODE_INCDEC,
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
			String name;
			NodeArray args;
		} method_call;
		struct {
			Node *expr;
		} ret;
		struct {
			String name;
			StringArray args;
			Node *block;
		} func;
		struct {
			StringArray args;
			Node *block;
		} anon_func;
		struct {
			String name;
			Node *expr;
		} var;
		struct { int unused; } _continue;
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
		struct { int unsued; } _null;
		struct {
			String name;
			String as;
		} import;
		struct {
			String name;
		} use;
		struct {
			TokenKind op;
			bool post;
			Node *expr;
		} incdec;
	};
};

typedef Array(Node) NodeMemory;
typedef struct Parser {
	Lexer lexer;
	Token current_token;
	int token_offset;
	NodeMemory nodes;
} Parser;

void init_parser(Parser *p, String path);
void init_parser_from_string(Parser *p, char *str);

NodeArray parse(Parser *p);

#endif /* BS_PARSER_H */