#ifndef LIBEASYELF_HPP
#define LIBEASYELF_HPP

#ifdef _MSC_VER
  #define _SCL_SECURE_NO_WARNINGS
  #define ELFIO_NO_INTTYPES
#endif

#include <iostream>
#include <map>

#include <windows.h>

#include "reader.hpp"
#include "dump.hpp"

using namespace ELFIO;

// That's how I want the API to look, change later in source.
#define cSection section
#define getFileOffset get_offset

class cSymbol;
class cRelocation;

class cELF {
public:
	elfio reader;
	bool loaded;
	std::map<Elf_Half, cSymbol *> symbols;
	std::map<Elf_Half, cRelocation *> relocations;
	unsigned char *image;
	int image_length;
	
	cELF(char *filename);
	void dump();
	void dumpSymbols();
	void dumpRelocations();
	void loadSections();
	void loadSymbolsOfSection(section *sec);
	void loadRelocationsOfSection(section *sec);
	section *getSectionByName(char *name);
	cSymbol *getSymbolByName(char *name);
	cSymbol *getSymbolByID(Elf_Word id);
	int getIdOfSymbol(char *name);
	int importSymbol(char *name, void *value);
	int importSymbol(int text_fileoffset, char *name, void *value);
	int getProcAddress(char *name);
	section *getSectionByInternalID(Elf_Half id);
}; // class cELF

class cSymbol {
public:
	int id;
	cELF *elf;

	std::string   name;
	Elf64_Addr    value;
	Elf_Xword     size;
	unsigned char bind;
	unsigned char type;
	Elf_Half      sect;
	unsigned char other;

	cSymbol(int id_, cELF *elf_) : id(id_), elf(elf_) {
	}

	void dump() {
		std::ios_base::fmtflags original_flags = std::cout.flags();
		printf("cSymbol::dump() ");
		if ( elf->reader.get_class() == ELFCLASS32 ) { // Output for 32-bit
			std::cout << "[" 
				<< "id=" << DUMP_DEC_FORMAT(  5 ) << id
				<< "] "
				<< "value=" << DUMP_HEX_FORMAT(  8 ) << value                   << " "
				<< "size=" << DUMP_HEX_FORMAT(  8 ) << size                    << " "
				<< "type=" << DUMP_STR_FORMAT(  7 ) << dump::str_symbol_type( type ) << " "
				<< "bind=" << DUMP_STR_FORMAT(  8 ) << dump::str_symbol_bind( bind ) << " "
				<< "sect=" << DUMP_DEC_FORMAT(  5 ) << sect                 << " "
				<< "name=" << DUMP_STR_FORMAT(  1 ) << name                    << " "
				<< std::endl;
		} else {                           // Output for 64-bit
			std::cout << "[" 
				<< DUMP_DEC_FORMAT(  5 ) << id
				<< "] "
				<< DUMP_HEX_FORMAT( 16 ) << value                   << " "
				<< DUMP_HEX_FORMAT( 16 ) << size                    << " "
				<< DUMP_STR_FORMAT(  7 ) << dump::str_symbol_type( type ) << " "
				<< DUMP_STR_FORMAT(  8 ) << dump::str_symbol_bind( bind ) << " "
				<< DUMP_DEC_FORMAT(  5 ) << sect                 << " "
				<< std::endl
				<< "        "
				<< DUMP_STR_FORMAT(  1 ) << name                    << " "
				<< std::endl;
		}
		std::cout.flags(original_flags);
	}
	
	Elf64_Addr getSegmentOffset() {
		return value;
	}
	
	Elf64_Addr getFileOffset() {
		section *section_of_symbol = elf->getSectionByInternalID(sect);
		section *section_of_data = elf->getSectionByName((char *) ".data");
		Elf64_Addr offsetForDataSection = section_of_data->getFileOffset();
		Elf64_Addr offsetInData = offsetForDataSection + getSegmentOffset();
		Elf32_Addr offsetForActualSymbol = *(int *)( elf->image + offsetInData );
		//printf("sect id=%d offsetForDataSection=%llp .data fileoffset=%llp offsetInData=%llp offsetForActualSymbol=%p\n", sect, offsetForDataSection, section_of_data->getFileOffset(), offsetInData, offsetForActualSymbol);
		return section_of_symbol->getFileOffset() + offsetForActualSymbol;
	}
	
	
	Elf64_Addr getAbsoluteFileOffset() {
		return (Elf64_Addr)elf->image + getFileOffset();
	}
}; // class cSymbol

class cRelocation {
public:
	int id;
	cELF *elf;
	
	Elf64_Addr offset;
	Elf_Word sym;
	Elf_Word type;
	Elf_Sxword addend;

	cRelocation(int id_, cELF *elf_) : id(id_), elf(elf_) {

	}

	void dump() {
		cSymbol *sym_ = elf->getSymbolByID(sym);
		printf("cRelocation::dump() id=%3d offset=%llp symbol=%3d symname=%s type=%3d addend=%3d\n", id, offset, sym, sym_->name.c_str(), type, addend);
		//std::cout << "id=" << id << " offset=" << offset << " symbol=" << symbol << " type=" << type << " addend=" << addend << std::endl;
	}
}; // class cRelocation




	
cELF::cELF(char *filename) {
	loaded = reader.load(filename);
	loadSections();
	
	// load whole image into memory and make it executable
	if ( ! file_get_contents(filename, &image, &image_length)) {
		printf("file_get_contents failed..");
		exit(1);
	}
	printf("length=%d o = %s\n\n", image_length, image);
	memory_rwx(image, image_length);
}

void cELF::dump() {
	printf("cELF::dump()\n");
	dumpSymbols();
	dumpRelocations();

	printf("fooooo\n");
	dump::header         ( std::cout, reader );
	dump::section_headers( std::cout, reader );
	printf("Segment Headers: \n");
	dump::segment_headers( std::cout, reader );
	dump::symbol_tables  ( std::cout, reader );
	dump::notes          ( std::cout, reader );
	dump::dynamic_tags   ( std::cout, reader );
	dump::section_datas  ( std::cout, reader );
	dump::segment_datas  ( std::cout, reader );
	printf("fooooo\n");
	
	printf("header->get_segments_num()=%d\n", reader.header->get_segments_num());
}

void cELF::dumpSymbols() {
	if (reader.get_class() == ELFCLASS32) { // Output for 32-bit
		printf("[  Nr ] Value    Size     Type    Bind      Sect Name\n");
	} else {                                    // Output for 64-bit
		printf("[  Nr ] Value            Size             Type    Bind      Sect\n        Name\n");
	}
	for (std::map<Elf_Half, cSymbol *>::iterator i=symbols.begin(); i != symbols.end(); i++) {
		i->second->dump();
	}
	printf("\n");
}
void cELF::dumpRelocations() {
	if (reader.get_class() == ELFCLASS32) { // Output for 32-bit
		printf("[  Nr ] Value    Size     Type    Bind      Sect Name\n");
	} else {                                    // Output for 64-bit
		printf("[  Nr ] Value            Size             Type    Bind      Sect\n        Name\n");
	}
	for (std::map<Elf_Half, cRelocation *>::iterator i=relocations.begin(); i != relocations.end(); i++) {
		i->second->dump();
	}
	printf("\n");
}

void cELF::loadSections() {
	printf("cELF::loadSections()\n");
	Elf_Half n = reader.sections.size();
	for (Elf_Half i = 0; i < n; ++i) {
		section *sec = reader.sections[i];
		Elf_Word type = sec->get_type();
		printf("Section i=%2d name=%-12s type=%-12s\n", i, sec->get_name().c_str(), dump::str_section_type(sec->get_type()).c_str());
		if (type == SHT_SYMTAB || type == SHT_DYNSYM)
			loadSymbolsOfSection(sec);
		if (type == SHT_REL && sec->get_name() == ".rel.text")
			loadRelocationsOfSection(sec);
	}
}

void cELF::loadSymbolsOfSection(section *sec) {
	symbol_section_accessor ssa(reader, sec);
	Elf_Half len = ssa.get_symbols_num();
	printf("cELF::loadSymbols() name=%s len=%d\n", sec->get_name().c_str(), len);
	for (Elf_Half i = 0; i < len; i++) {
		cSymbol *sym = new cSymbol(i, this);
		ssa.get_symbol(i, sym->name, sym->value, sym->size, sym->bind, sym->type, sym->sect, sym->other);
		symbols[i] = sym;
	}
}
void cELF::loadRelocationsOfSection(section *sec) {
	relocation_section_accessor rsa = relocation_section_accessor(reader, sec);
	Elf_Half len = rsa.get_entries_num();
	printf("cELF::loadRelocations() name=%s len=%d\n", sec->get_name().c_str(), len);
	for (Elf_Half i = 0; i < len; i++) {
		cRelocation *rel = new cRelocation(i, this);
		rsa.get_entry(i, rel->offset, rel->sym, rel->type, rel->addend);
		relocations[i] = rel;
	}
}

section *cELF::getSectionByName(char *name) {
	Elf_Half n = reader.sections.size();
	for (Elf_Half i = 0; i < n; ++i) {
		section *sec = reader.sections[i];
		if ( ! strcmp(sec->get_name().c_str(), name))
			return sec;
	}
	return NULL;
}

cSymbol *cELF::getSymbolByName(char *name) {
	for (std::map<Elf_Half, cSymbol *>::iterator i=symbols.begin(); i != symbols.end(); i++) {
		if ( ! strcmp(i->second->name.c_str(), name))
			return i->second;
	}
	Debug::LogError("getSymbolByName(name=%s): Symbol name \"%s\" doesn't exist!", name, name);
	return NULL;
}

cSymbol *cELF::getSymbolByID(Elf_Word id) {
	// gosh, i should use a goddammit vector instead of list, cba now
	Elf_Word idx = 0;
	for (std::map<Elf_Half, cSymbol *>::iterator i=symbols.begin(); i != symbols.end(); i++) {
		if (idx == id)
			return i->second;
		idx++;
	}
	return NULL;
}

int cELF::getIdOfSymbol(char *name) {
	cSymbol *sym = getSymbolByName(name);
	if ( ! sym) {
		Debug::LogError("getIdOfSymbol(name=%s): Could not get ID for symbol \"%s\"!", name, name);
		return -1;
	}
	return sym->id;
}

int cELF::importSymbol(char *name, void *value) {
	cSection *text = getSectionByName((char *)".text");
	return importSymbol((int)text->getFileOffset(), name, value);
}

int cELF::importSymbol(int text_fileoffset, char *name, void *value) {
	std::string tmp = string_format("elf->importSymbol(text_fileoffset=%p, name=%-20s, value=%p)", text_fileoffset, name, value);
	
	int id = getIdOfSymbol(name);
	
	tmp += string_format(" getIdOfSymbol=%d;\n", id);

	if (id == -1) {
		printf(tmp.c_str());
		Debug::printLastErrors(/*depth=*/1);
		Debug::cleanLastErrors();
		return 0;
	}

	printf("%s", tmp.c_str());

	for (std::map<Elf_Half, cRelocation *>::iterator i=relocations.begin(); i != relocations.end(); i++) {
		if ((int)i->second->sym == id) {
			int relocation_offset = i->second->offset;
			
			std::string tmp = string_format("    symbol-id=%d relocation_offset=%p Relocation value=%p total offset=%p symbol_type=%s",
				id,
				relocation_offset,
				*(int *)(image + text_fileoffset + relocation_offset),
				text_fileoffset + relocation_offset,
				dump::str_symbol_type(i->second->type).c_str()
			);

			if (i->second->type != STT_FUNC) {
				tmp += " >>>import symbol normally<<<\n";
				*(int *)(image + text_fileoffset + relocation_offset) = (int)value;
			} else {
				int final_addr = (int)(image + text_fileoffset + relocation_offset);
				tmp += string_format(" >>>import symbol as FUNC<<< image: %p final: %p\n", image, final_addr);
				cracking_hook_call(final_addr - 1, (int)value);
			}
			
			printf("%s", tmp.c_str());
		}
	}
	return 1;
}

int cELF::getProcAddress(char *name) {
	cSection *text = getSectionByName((char *)".text");
	unsigned char *textptr = image + text->getFileOffset();
	cSymbol *sym = getSymbolByName(name);
	int procaddress = (int)textptr + sym->value;
	printf("getProcAddress(name=%-20s): text_fileoffset=%llp symbol=%p symbol->value=%llp procaddress=%p\n", name, text->getFileOffset(), sym, sym->value, procaddress);
	return procaddress;
}

section *cELF::getSectionByInternalID(Elf_Half id) {
	Elf_Half n = reader.sections.size();
	Elf_Half idx = 0; // only count the sections with flags (WAX), as seeable with dumper.exe
	for ( Elf_Half i = 0; i < n; i++ ) {
		section *sec = reader.sections[i];
		if (sec->get_flags() == 0)
			continue;
		if ( idx == id)
			return sec;
		idx++;
	}
	return NULL;
}

#endif