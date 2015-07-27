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

int debug_printf(char *msg) {
	return printf("debug_printf: first arg=%p text=%s\n", msg, msg);
}
int debug_puts(char *msg) {
	return printf("debug_puts: arg=%p text=%s\n", msg, msg);
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
	elf_a->importSymbol((int)text->getFileOffset(), (char *)"_printf", (void *)printf);
	elf_a->importSymbol((int)text->getFileOffset(), (char *)"_puts", (void *)puts);
	elf_a->importSymbol((int)text->getFileOffset(), (char *)"_vprintf", (void *)vprintf);
	
	cSymbol *d1 = elf_a->getSymbolByName((char *)"_someData1");
	cSymbol *d2 = elf_a->getSymbolByName((char *)"_someData2");
	cSymbol *d3 = elf_a->getSymbolByName((char *)"_someData3");
	printf("someData1 symbol segment offset: %llp symbol->getFileOffset()=%llp text=%s\n", d1->getSegmentOffset(), d1->getFileOffset(), d1->getAbsoluteFileOffset() );
	printf("someData2 symbol segment offset: %llp symbol->getFileOffset()=%llp text=%s\n", d2->getSegmentOffset(), d2->getFileOffset(), d2->getAbsoluteFileOffset() );
	printf("someData3 symbol segment offset: %llp symbol->getFileOffset()=%llp text=%s\n", d3->getSegmentOffset(), d3->getFileOffset(), d3->getAbsoluteFileOffset() );
	
	printf("should be .text : file_offset=%p\n", elf_a->getSectionByInternalID(0)->getFileOffset());
	printf("should be .data : file_offset=%p\n", elf_a->getSectionByInternalID(1)->getFileOffset());
	printf("should be .bss  : file_offset=%p\n", elf_a->getSectionByInternalID(2)->getFileOffset());
	printf("should be .rdata: file_offset=%p\n", elf_a->getSectionByInternalID(3)->getFileOffset());
	
	// get the function pointers
	int (*add)(int a, int b);
	int (*mul)(int a, int b);
	int (*printsomething)(char *msg, ...);
	void (*printglobals)();
	*(int *)&add = elf_a->getProcAddress((char *)"_add");
	*(int *)&mul = elf_a->getProcAddress((char *)"_mul");
	*(int *)&printsomething = elf_a->getProcAddress((char *)"_printsomething");
	*(int *)&printglobals = elf_a->getProcAddress((char *)"_printglobals");

	// call it
	printf("add=%p mul=%p\n", add, mul);
	printf("lib.c add(2,2) = %d\n", add(2,2));
	printf("lib.c mul(3,3) = %d\n", mul(3,3));

	printsomething((char *)"haiiiii! 123=%d\n", 123);

	elf_a->relocateTextForRdata();
	elf_a->relocateTextForData();
	printglobals();
	printglobals();
	printglobals();
	
	printf("End!\n");
	
	#if 0
	cELF *elf_b = new cELF((char *)"testlib_b.elf");
	
	int (*addmul)(int a, int b, int factor);
	
	elf_b->importSymbol((char *)"_add", (void *)add);
	elf_b->importSymbol((char *)"_mul", (void *)mul);
	*(int *)&addmul = elf_b->getProcAddress((char *)"_addmul");
	printf("addmul: %p\n", addmul);
	printf("lib.c addmul(3,3, 2) = %d\n", addmul(3, 3, 2));
	
	printf("Address of printf=%p real printf=%p\n", GetProcAddress(NULL, "printf"), printf);
	#endif
	
	return 0;
}