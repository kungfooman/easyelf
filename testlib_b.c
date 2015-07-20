extern int add(int a, int b);
extern int mul(int a, int b);

int addmul(int a, int b, int factor) {
	int tmp = add(a, b);
	return mul(tmp, factor);
}
