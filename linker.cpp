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
	
	cELF *elf = new cELF(filename);
	
	memory_rwx(image, length);

	//unsigned char *text = image + object_get_text_fileoffset(image);
	int segment_text = 0x34;
	section *text = elf->getSectionByName((char *)".text");
	printf(".text section: fileoffset=%p\n", text->getFileOffset());
	unsigned char *textptr = image + text->getFileOffset();
	int test_a = 10;
	int test_b = 100;
	//*(int *)(o + 0x58 + 0x80) = (int)&ten;
	
	// apply relocation records for .text
	*(int *)(textptr + 0x0c) = (int)&test_a;
	*(int *)(textptr + 0x13) = (int)&test_b;
	*(int *)(textptr + 0x28) = (int)&test_a;

	int (*add)(int a, int b);
	int (*mul)(int a, int b);
	*(int *)&add = (int)textptr + 0x00;
	*(int *)&mul = (int)textptr + 0x1B;
	
	
	//external_filehdr *header = (external_filehdr *)o;
	//printf("Number of symbols: %d\n", header->f_nsyms);
	//printf("Symbol table offset: %.8p\n", header->f_symptr);
	

	printf("lib.c add(2,2) = %d\n", add(2,2));
	printf("lib.c mul(3,3) = %d\n", mul(3,3));
	printf("length=%d o = %s", length, image);
	return 0;
}