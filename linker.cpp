#include <stdio.h>
#include <stdlib.h>

#include "libeasyelf/libeasyelf.hpp"

int debug_add(int a, int b) {
	printf("myadd: a=%d b=%d\n", a, b);
	return a + b;
}
int debug_mul(int a, int b) {
	printf("mymul: a=%d b=%d\n", a, b);
	return a * b;
}

int main() {
	cELF *elf_a = new cELF((char *)"testlib_a.elf");
	
	// get text segment
	cSection *text = elf_a->getSectionByName((char *)".text");
	printf(".text section: fileoffset=%p\n", text->getFileOffset());
	unsigned char *textptr = elf_a->image + text->getFileOffset();
	
	// apply relocation records for .text inside our "image" (textptr points into it)
	int test_a = 10;
	int test_b = 100;
	//test_a = test_b = 0;
	elf_a->importSymbol((int)text->getFileOffset(), (char *)"_test_a", &test_a);
	elf_a->importSymbol((int)text->getFileOffset(), (char *)"_test_b", &test_b);
	
	// get the function pointers
	int (*add)(int a, int b);
	int (*mul)(int a, int b);
	*(int *)&add = (int)textptr + elf_a->getSymbolByName((char *)"_add")->value;
	*(int *)&mul = (int)textptr + elf_a->getSymbolByName((char *)"_mul")->value;

	// call it
	printf("lib.c add(2,2) = %d\n", add(2,2));
	printf("lib.c mul(3,3) = %d\n", mul(3,3));
	
	
	cELF *elf_b = new cELF((char *)"testlib_b.elf");
	
	int (*addmul)(int a, int b, int factor);
	
	elf_b->importSymbol((char *)"_add", (void *)add);
	elf_b->importSymbol((char *)"_mul", (void *)mul);
	*(int *)&addmul = elf_b->getProcAddress((char *)"_addmul");
	printf("addmul: %p\n", addmul);
	printf("lib.c addmul(3,3, 2) = %d\n", addmul(3, 3, 2));
	
	return 0;
}