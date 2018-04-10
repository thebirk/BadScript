typedef enum NodeKind {
	NODE_NUMBER,
	NODE_NAME,
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
} NodeKind;

typedef struct Node Node;
struct Node {
	int kind;
	SourceLoc loc;
	union {
		struct {
			double value;
		} number;
		struct {
			String name;
		} name;
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
			Array(Node*) args;
		} call;
		struct {
			Node *expr;
		} ret;
		struct {
			Array(String) args;
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
			Array(Node*) stmts;
		} block;
		struct {
			Array(Node*) index; // If null we have {expr, expr, expr.. } otherwise we have {index = expr, index = expr, ...}, index should probably be a number or string
			Array(Node*) exprs; // possibly split into another tagged union with default,named
		} table;
	};
};