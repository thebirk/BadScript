#ifndef BS_LEXER_H
#define BS_LEXER_H

#include "common.h"

typedef enum TokenKind {
	TOKEN_UNKNOWN = 0,
	TOKEN_NUMBER,
	TOKEN_IDENT,
	TOKEN_NULL,
	TOKEN_FUNC,
	TOKEN_RETURN,
	TOKEN_CONTINUE,
	TOKEN_BREAK,
	TOKEN_VAR,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_LEFTPAR,
	TOKEN_RIGHTPAR,
	TOKEN_LEFTBRACE,
	TOKEN_RIGHTBRACE,
	TOKEN_LEFTBRACKET,
	TOKEN_RIGHTBRACKET,
	TOKEN_EQUAL,
	TOKEN_EQUALS,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_SLASH,
	TOKEN_ASTERISK,
	TOKEN_MOD,
	TOKEN_LT,
	TOKEN_GT,
	TOKEN_LTE,
	TOKEN_GTE,
	TOKEN_NE,
	TOKEN_COMMA,
	TOKEN_SEMICOLON,
	TOKEN_COLON,
	TOKEN_FOR,
	TOKEN_WHILE,
	TOKEN_STRING,
	TOKEN_LAND,
	TOKEN_LOR,
	TOKEN_DOT,
	TOKEN_IMPORT,
	TOKEN_USE,
	TOKEN_INCREMENT,
	TOKEN_DECREMENT,
	TOKEN_TRUE,
	TOKEN_FALSE,
	TOKEN_AS,
	TOKEN_NOT,
	TOKEN_EOF,

	LAST_TOKEN_KIND,
} TokenKind;

char* token_kind_to_string(TokenKind kind);

typedef struct SourceLoc {
	String file;
	size_t line;
	size_t offset;
} SourceLoc;

typedef struct Token {
	TokenKind kind;
	String lexeme;
	SourceLoc loc;
	union {
		double number_value;
	};
} Token;

typedef struct Lexer {
	String file;
	char *data;
	size_t line;
	size_t offset;
	Array(Token) tokens;
	StringArray strings;
} Lexer;

void init_lexer(Lexer *lexer, String path);
void init_lexer_from_string(Lexer *lexer, char *str);

char* token_kind_to_string(TokenKind kind);
void lex(Lexer *lexer);

#endif /* BS_LEXER_H */