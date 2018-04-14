#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

#include "common.c"
#include "timings.c"
#include "lexer.c"
#include "parser.c"
#include "ir.c"
#include "runtime.c"

void print_usage(char *binary_name) {
	printf("Usage: %s [options] <script>\n", binary_name);
	printf("\nOptions:\n");
	printf("\t-h/-help - Prints out program usage\n");
	printf("\t-timings - Prints timing information\n");
	printf("\t-silent  - Suppresses all output\n");
}

int main(int argc, char **argv) {
	bool print_timings = false;
	bool silence = false;
	char* binary_name = argv[0];

	String filename = {0};
	for (size_t i = 1; i < argc; i++) {
		char *arg = argv[i];
		if (*arg == '-') {
			char *name = arg+1;

			if (strcmp(name, "timings") == 0) {
				print_timings = true;
			}
			else if (strcmp(name, "silent") == 0) {
				silence = true;
			}
			else if (strcmp(name, "help") == 0 || strcmp(name, "h") == 0) {
				print_usage(binary_name);
				exit(0);
			}
			else {
				printf("Unknown option '%s'!\n", name);
				print_usage(binary_name);
				exit(1);
			}
		}
		else {
			if (filename.str) {
				printf("More than one file was provided\n");
				print_usage(binary_name);
				exit(1);
			}
			else {
				filename = make_string_slow(arg);
			}
		}
	}
	
#ifdef _WIN32
	if (IsDebuggerPresent()) {
		filename = make_string_slow("tests/import_test.bs");
		printf("Parsing: %s\n", filename.str);
	}
	else {
#endif
		if (filename.str == 0) {
			printf("No file was provided!\n");
			print_usage(argv[0]);
			exit(1);
		}
#ifdef _WIN32
	}
#endif
	

	if (silence) {
#if _WIN32
		freopen("nul", "w", stdout);
		freopen("nul", "w", stderr);
#else
		freopen("/dev/null", "w", stdout);
		freopen("/dev/null", "w", stderr);
#endif
	}

	Timings t = {0};
	timings_init(&t, make_string_slow("total time"));

	//lexer_test();
	timings_start_section(&t, make_string_slow("lexer"));
	Parser p = (Parser){0};
	memset(&p, 0, sizeof(Parser));
	init_parser(&p, filename);
	timings_start_section(&t, make_string_slow("parser"));
	NodeArray stmts = parse(&p);

	timings_start_section(&t, make_string_slow("ir"));
	Ir ir = { 0 };
	memset(&ir, 0, sizeof(Ir));
	init_ir(&ir, stmts);

	timings_start_section(&t, make_string_slow("ir run"));
	Value *return_value = ir_run(&ir);

	if (print_timings) {
		printf("\n");
		timings_print_all(&t, TimingUnit_Millisecond);
	}	
	
	if (return_value->kind == VALUE_NUMBER) {
		return return_value->number.value;
	}
	else {
		return 0;
	}
}