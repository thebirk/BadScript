#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "common.c"
#include "timings.c"
#include "lexer.c"
#include "parser.c"
#include "ir.c"

int main() {
	Timings t = {0};
	timings_init(&t, make_string_slow("BadScript"));

	timings_start_section(&t, make_string_slow("lexer"));
	//lexer_test();

	Parser p = (Parser){0};
	memset(&p, 0, sizeof(Parser));
	init_parser(&p, "parsing.bd");
	Node *file_block = parse(&p);

	timings_print_all(&t, TimingUnit_Millisecond);
	return 0;
}