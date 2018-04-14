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
	TOKEN_EOF,

	LAST_TOKEN_KIND,
} TokenKind;

char* token_kind_to_string(TokenKind kind) {
	switch (kind) {
		case TOKEN_UNKNOWN:      return "(unknown)";
		case TOKEN_NUMBER:       return "number";
		case TOKEN_IDENT:        return "identifier";
		case TOKEN_NULL:         return "null";
		case TOKEN_FUNC:         return "func";
		case TOKEN_RETURN:       return "return";
		case TOKEN_CONTINUE:     return "continue";
		case TOKEN_BREAK:        return "break";
		case TOKEN_VAR:          return "var";
		case TOKEN_IF:           return "if";
		case TOKEN_ELSE:         return "else";
		case TOKEN_LEFTPAR:      return "(";
		case TOKEN_RIGHTPAR:     return ")";
		case TOKEN_LEFTBRACE:    return "{";
		case TOKEN_RIGHTBRACE:   return "}";
		case TOKEN_LEFTBRACKET:  return "[";
		case TOKEN_RIGHTBRACKET: return "]";
		case TOKEN_EQUAL:        return "=";
		case TOKEN_EQUALS:       return "==";
		case TOKEN_PLUS:         return "+";
		case TOKEN_MINUS:        return "-";
		case TOKEN_SLASH:        return "/";
		case TOKEN_ASTERISK:     return "*";
		case TOKEN_MOD:          return "%";
		case TOKEN_LT:           return "<";
		case TOKEN_GT:           return ">";
		case TOKEN_LTE:          return "<=";
		case TOKEN_GTE:          return ">=";
		case TOKEN_NE:           return "!=";
		case TOKEN_COMMA:        return ",";
		case TOKEN_SEMICOLON:    return ";";
		case TOKEN_COLON:        return ":";
		case TOKEN_FOR:          return "for";
		case TOKEN_WHILE:        return "while";
		case TOKEN_STRING:       return "string";
		case TOKEN_LAND:         return "&&";
		case TOKEN_LOR:          return "||";
		case TOKEN_DOT:          return ".";
		case TOKEN_IMPORT:       return "import";
		case TOKEN_EOF:          return "(end of file)";

		default: return "(unimplemented TokenKind name)";
	}
}

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

#ifdef _WIN32
__declspec(noreturn)
#else
__attribute__((noreturn))
#endif
void lexer_error(Lexer *lexer, char *format, ...) {
	printf("%s(%lld, %lld): ", lexer->file.str, lexer->line, lexer->offset);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");

#ifdef _WIN32
	if (IsDebuggerPresent()) {
		assert(!"ir_error assert for dev");
	}
#endif

	exit(1);
}

void read_entire_file(Lexer *lexer, String path) {
	FILE *f = fopen(path.str, "rb");
	if (!f) {
		lexer_error(lexer, "Failed to open file '%s'!", path.str);
	}
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	rewind(f);

	char *buffer = malloc(len + 1);
	buffer[len] = 0;
	if (fread(buffer, len, 1, f) != 1) {
		lexer_error(lexer, "Failed to read file '%s'!", path.str);
	}
	fclose(f);

	lexer->data = buffer;
}

void add_token(Lexer *lexer, Token t) {
	t.loc = (SourceLoc) { lexer->file, lexer->line, lexer->offset };
	t.lexeme = make_string_slow_len(t.lexeme.str, t.lexeme.len);
	array_add(lexer->strings, t.lexeme);
	array_add(lexer->tokens, t);
}

void check_strings_for_backslashes(String path) {
	bool did_convert = false;
	for (size_t i = 0; i < path.len; i++) {
		if (path.str[i] == '\\') {
			path.str[i] = '/';
			did_convert = true;
		}
	}

	if (did_convert) {
		printf("lexer: Converted backslahes to slashes!\n\n");
	}
}

void init_lexer(Lexer *lexer, String path) {
	check_strings_for_backslashes(path);

	lexer->file = path;
	lexer->line = 1;
	lexer->offset = 1;
	read_entire_file(lexer, lexer->file);

	array_init(lexer->tokens, 128);
}

void init_lexer_from_string(Lexer *lexer, char *str) {
	lexer->file = make_string_slow("<string>");
	lexer->line = 1;
	lexer->offset = 1;

	size_t len = strlen(str);
	lexer->data = malloc(len + 1);
	lexer->data[len] = 0;
	memcpy(lexer->data, str, len);

	array_init(lexer->tokens, 128);
}

bool is_upper(char c) {
	return (c >= 'A' && c <= 'Z');
}

bool is_lower(char c) {
	return (c >= 'a' && c <= 'z');
}

bool is_alpha(char c) {
	return is_upper(c) || is_lower(c);
}

bool is_num(char c) {
	return (c >= '0') && (c <= '9');
}

bool is_alnum(char c) {
	return is_alpha(c) || is_num(c) || c == '_';
}

bool is_space(char c) {
	return (c == ' ' || c == '\t');
}

void lex(Lexer *lexer) {
	char *ptr = lexer->data;

	while (*ptr) {
		if (*ptr == '#' && lexer->line == 1 && lexer->offset == 1) {
			while (*ptr && *ptr != '\n') {
				ptr++;
			}
			lexer->line++;
			lexer->offset = 1;
			continue;
		}

		if (*ptr == '\r') {
			ptr++;
			continue;
		}

		if (*ptr == '\n') {
			ptr++;
			lexer->line++;
			lexer->offset = 0;
			continue;
		}

		if (is_space(*ptr)) {
			ptr++;
			lexer->offset++;
			continue;
		}

		if (*ptr == '/' && *(ptr + 1) == '/') {
			ptr += 2;
			while (*ptr && *ptr != '\n') {
				ptr++;
			}
			if (*ptr == '\n') ptr++; // Eat newline if we are not and the end of the file
			lexer->line++;
			lexer->offset = 1;
			continue;
		}

		if (*ptr == '/' && *(ptr + 1) == '*') {
			ptr += 2;
			int n = 1;
			while (*ptr && n > 0) {
				if (*ptr == '\n') {
					lexer->line++;
					lexer->offset = 1;
					ptr++;
					continue;
				}
				if (*ptr == '\r') {
					ptr++;
					continue;
				}

				if (*ptr == '/' && *(ptr + 1) == '*') {
					n++;
					lexer->offset += 2;
					ptr += 2;
					continue;
				}
				else if (*ptr == '*' && *(ptr + 1) == '/') {
					n--;
					lexer->offset += 2;
					ptr += 2;
					continue;
				}

				ptr++;
				lexer->offset++;
			}

			continue;
		}

		if (0) {}
#define DOUBLE_TOKEN(_first, _second, _enum) else if(*ptr == _first && *(ptr+1) == _second) { char str[3]; str[0] = _first; str[1] = _second; str[2] = 0; add_token(lexer, (Token) {.kind = _enum, .lexeme = make_string_slow(str)}); ptr += 2; lexer->offset += 2; continue; }
		DOUBLE_TOKEN('>', '=', TOKEN_GTE)
		DOUBLE_TOKEN('<', '=', TOKEN_LTE)
		DOUBLE_TOKEN('=', '=', TOKEN_EQUALS)
		DOUBLE_TOKEN('!', '=', TOKEN_NE)
		DOUBLE_TOKEN('&', '&', TOKEN_LAND)
		DOUBLE_TOKEN('|', '|', TOKEN_LOR)
#undef DOUBLE_TOKEN

		switch (*ptr) {
#define BASIC_TOKEN(_tok, _enum) case _tok: { char str[2]; str[0] = _tok; str[1] = 0; add_token(lexer, (Token) {.kind = _enum, .lexeme = make_string_slow(str)}); lexer->offset++; ptr++; continue; }
			BASIC_TOKEN('+', TOKEN_PLUS);
			BASIC_TOKEN('-', TOKEN_MINUS);
			BASIC_TOKEN('/', TOKEN_SLASH);
			BASIC_TOKEN('*', TOKEN_ASTERISK);
			BASIC_TOKEN('%', TOKEN_MOD);
			
			BASIC_TOKEN('=', TOKEN_EQUAL);
			
			BASIC_TOKEN('<', TOKEN_LT);
			BASIC_TOKEN('>', TOKEN_GT);

			BASIC_TOKEN('.', TOKEN_DOT);
			BASIC_TOKEN(',', TOKEN_COMMA);
			BASIC_TOKEN(';', TOKEN_SEMICOLON);
			BASIC_TOKEN(':', TOKEN_COLON);

			BASIC_TOKEN('(', TOKEN_LEFTPAR);
			BASIC_TOKEN(')', TOKEN_RIGHTPAR);

			BASIC_TOKEN('{', TOKEN_LEFTBRACE);
			BASIC_TOKEN('}', TOKEN_RIGHTBRACE);

			BASIC_TOKEN('[', TOKEN_LEFTBRACKET);
			BASIC_TOKEN(']', TOKEN_RIGHTBRACKET);
#undef BASIC_TOKEN
		}

		if (is_alpha(*ptr)) {
			char *start = ptr;
			while (*ptr && is_alnum(*ptr)) {
				ptr++;
			}
			add_token(lexer, (Token) { .kind = TOKEN_IDENT, .lexeme = make_string_slow_len(start, ptr - start) });
			lexer->offset += ptr - start;
			continue;
		}

		if (*ptr == '"') {
			ptr++;
			char *start = ptr;
			while (*ptr && *ptr != '"') {
				// TODO: Escaping quotes
				ptr++;
			}
			if (*ptr != '"') {
				lexer_error(lexer, "Unexpected end of file while parsing string!");
			}
			char *end = ptr;
			ptr++;

			add_token(lexer, (Token) { .kind = TOKEN_STRING, .lexeme = make_string_slow_len(start, end - start) });
			lexer->offset += ptr - start + 1;
			continue;
		}

		if (is_num(*ptr)) {
			char *start = ptr;
			bool found_dot = false;
			while (*ptr && (is_num(*ptr) || *ptr == '.')) {
				if (*ptr == '.') {
					if (found_dot) {
						lexer_error(lexer, "Unexpected '.' while parsing number!");
					}
					else {
						found_dot = true;
					}
				}
				ptr++;
			}
			double val = strtod(start, 0);
			add_token(lexer, (Token) { .kind = TOKEN_NUMBER, .lexeme = make_string_slow_len(start, ptr - start), .number_value = val });
			lexer->offset += ptr - start;
			continue;
		}

		lexer_error(lexer, "Unexpected character '%c'/0x%02X\n", *ptr, *ptr);
	}

	// Detect keywords
	Token *t;
	for_array_ref(lexer->tokens, t) {
		if (t->kind == TOKEN_IDENT) {
			if (0) {}
#define KEYWORD(_str, _enum) else if(strings_match(_str, t->lexeme)) { t->kind = _enum; continue; }
			KEYWORD(string("var"),      TOKEN_VAR)
			KEYWORD(string("func"),     TOKEN_FUNC)
			KEYWORD(string("if"),       TOKEN_IF)
			KEYWORD(string("else"),     TOKEN_ELSE)
			KEYWORD(string("for"),      TOKEN_FOR)
			KEYWORD(string("while"),    TOKEN_WHILE)
			KEYWORD(string("return"),   TOKEN_RETURN)
			KEYWORD(string("continue"), TOKEN_CONTINUE)
			KEYWORD(string("break"),    TOKEN_BREAK)
			KEYWORD(string("null"),     TOKEN_NULL)
			KEYWORD(string("import"),   TOKEN_IMPORT)
#undef KEYWORD
		}
	}

	array_add(lexer->tokens, (Token) { TOKEN_EOF });
}

void lexer_test() {
	Lexer lexer = { 0 };
	init_lexer(&lexer, string("test.bd"));
	lex(&lexer);

	Token *t;
	for_array_ref(lexer.tokens, t) {
		printf("%s: %.*s", token_kind_to_string(t->kind), (int)t->lexeme.len, t->lexeme.str);
		if (t->kind == TOKEN_NUMBER) {
			printf(" - %f", t->number_value);
		}
		printf("\n");
	}
}