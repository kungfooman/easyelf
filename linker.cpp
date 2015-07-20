#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "libeasyelf.hpp"

int file_get_contents(char *filename, unsigned char **out_content, int *out_filesize) {
	FILE *file;
	int file_size;
	int read_size;
	unsigned char *content;
	file = fopen(filename, "rb");
	if (file == NULL)
		return 0;
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	*out_filesize = file_size;
	rewind(file);
	content = (unsigned char *)malloc(file_size);
	*out_content = content;
	if (content == NULL) {
		fclose(file);
		return 0;
	}
	read_size = fread(content, 1, file_size, file);
	if (file_size != read_size) {
		free(content);
		fclose(file);
		return 0;
	}
	fclose(file);
	return 1;
}

int memory_rwx(unsigned char *mem, int len) {
	DWORD old_protect;
	if( ! VirtualProtect((LPVOID)mem, len, PAGE_EXECUTE_READWRITE, &old_protect)) {
		printf("memory_rwx() failed\n");
		return 0;
	}
	return 1;
}

int main() {
	unsigned char *image;
	int length;
	
	char *filename = (char *)"testlib.elf";
	
	if ( ! file_get_contents(filename, &image, &length)) {
		printf("file_get_contents failed..");
		exit(1);
	}
	memory_rwx(image, length);
	
	cELF *elf = new cELF(filename);
	
	// get text segment
	cSection *text = elf->getSectionByName((char *)".text");
	printf(".text section: fileoffset=%p\n", text->getFileOffset());
	unsigned char *textptr = image + text->getFileOffset();
	
	// apply relocation records for .text inside our "image" (textptr points into it)
	int test_a = 10;
	int test_b = 100;
	elf->importSymbol((int)textptr, (char *)"_test_a", &test_a);
	elf->importSymbol((int)textptr, (char *)"_test_b", &test_b);
	
	// get the function pointers
	int (*add)(int a, int b);
	int (*mul)(int a, int b);
	*(int *)&add = (int)textptr + elf->getSymbolByName((char *)"_add")->value;
	*(int *)&mul = (int)textptr + elf->getSymbolByName((char *)"_mul")->value;

	// call it
	printf("lib.c add(2,2) = %d\n", add(2,2));
	printf("lib.c mul(3,3) = %d\n", mul(3,3));
	printf("length=%d o = %s", length, image);
	return 0;
}