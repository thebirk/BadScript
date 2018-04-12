#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

#include "hashmap.h"

#include "common.c"
#include "timings.c"
#include "lexer.c"
#include "parser.c"
#include "ir.c"
#include "runtime.c"

int main(int argc, char **argv) {
	Timings t = {0};
	timings_init(&t, make_string_slow("total time"));

	//lexer_test();
	timings_start_section(&t, make_string_slow("lexer"));
	Parser p = (Parser){0};
	memset(&p, 0, sizeof(Parser));
	if (argc > 1) {
		init_parser(&p, argv[1]);
	}
	else {
		char *path = "minimal.bs";
		printf("Parsing: %s\n", path);
		init_parser(&p, path);
	}
	timings_start_section(&t, make_string_slow("parser"));
	NodeArray stmts = parse(&p);

	timings_start_section(&t, make_string_slow("ir"));
	Ir ir = { 0 };
	memset(&ir, 0, sizeof(Ir));
	init_ir(&ir, stmts);

	timings_start_section(&t, make_string_slow("ir run"));
	Value *return_value = ir_run(&ir);

	printf("\n");
	timings_print_all(&t, TimingUnit_Millisecond);
	return 0;
}