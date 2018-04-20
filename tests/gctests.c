#include <stdio.h>
#include <stdlib.h>

int main() {
	double a = 0;
	while (1) {
		a = a + 1;
		printf("a: %f\n", a);
		
		double *t = (double*)malloc(sizeof(double)*100000);
		double i = 0;
		while (i < 100000) {
			t[(size_t)i] = i;
			i = i + 1;
		}
		free(t);
	}
}