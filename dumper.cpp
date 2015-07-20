#include <stdio.h>

#include "libeasyelf/libeasyelf.hpp"

int main( int argc, char** argv ) {
	char *file1 = (char *)"testlib_b.elf";
	char *file2 = (char *)"cod2_lnxded";
	cELF *elf = new cELF(file1);
	if (!elf->loaded) {
		printf("Coulnd't load ELF file!");
		exit(1);
	}
	
	elf->dump();	
	
	section *text = elf->getSectionByName((char *)".text");
	printf(".text section: %x file_offset=%x", text, text->get_offset());
	
	getchar();
    return 0;
}
