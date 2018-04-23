#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>

#include "common.h"
#include "timings.h"
#include "parser.h"
#include "ir.h"
#include "gfx.h"
#include "runtime.h"

void print_usage(char *binary_name) {
	printf("Usage: %s [options] <script> [script arguments]\n", binary_name);
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
	size_t last_arg = 1;
	for (; last_arg < argc; last_arg++) {
		char *arg = argv[last_arg];
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
			filename = make_string_slow(arg);
			last_arg++;
			break; // We  break when we find the file as the rest of the arguments goes to the script
		}
	}
	
	if (filename.str == 0) {
		printf("No file was provided!\n");
		print_usage(argv[0]);
		exit(1);
	}
	

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

	Value *return_value = ir_run(&ir, argc-last_arg, argv+last_arg);

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