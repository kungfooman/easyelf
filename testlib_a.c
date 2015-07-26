#include <stdarg.h>


char *someData1 = "aaaaaaaa8ej12xe61276exj811111";
char *someData2 = "bbbbbbbb8ej12xe61276exj822222";

extern int test_a;
extern int test_b;

int add(int a, int b) {
	return a + b + test_a + test_b;
}

int mul(int a, int b) {
	return a * b + test_a;
}

int printsomething(char *msg, ...)
{
	va_list myargs;
	va_start(myargs, msg);
	int ret = vprintf(msg, myargs);
	va_end(myargs);
	return ret;
}