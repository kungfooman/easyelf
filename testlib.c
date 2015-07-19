char *someData = "euxhx2ze78ej12xe61276exj8126ex7";

extern int test_a;
extern int test_b;

int add(int a, int b) {
	return a + b + test_a + test_b;
}

int mul(int a, int b) {
	return a * b + test_a;
}