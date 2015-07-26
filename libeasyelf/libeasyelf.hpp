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

class cSymbol {
public:
	int id;
	elfio *reader;

	std::string   name;
	Elf64_Addr    value;
	Elf_Xword     size;
	unsigned char bind;
	unsigned char type;
	Elf_Half      section;
	unsigned char other;

	cSymbol(int id_, elfio *reader_) : id(id_), reader(reader_) {
	}

	void dump() {
		std::ios_base::fmtflags original_flags = std::cout.flags();
		printf("cSymbol::dump() ");
		if ( reader->get_class() == ELFCLASS32 ) { // Output for 32-bit
			std::cout << "[" 
				<< DUMP_DEC_FORMAT(  5 ) << id
				<< "] "
				<< DUMP_HEX_FORMAT(  8 ) << value                   << " "
				<< DUMP_HEX_FORMAT(  8 ) << size                    << " "
				<< DUMP_STR_FORMAT(  7 ) << dump::str_symbol_type( type ) << " "
				<< DUMP_STR_FORMAT(  8 ) << dump::str_symbol_bind( bind ) << " "
				<< DUMP_DEC_FORMAT(  5 ) << section                 << " "
				<< DUMP_STR_FORMAT(  1 ) << name                    << " "
				<< std::endl;
		} else {                           // Output for 64-bit
			std::cout << "[" 
				<< DUMP_DEC_FORMAT(  5 ) << id
				<< "] "
				<< DUMP_HEX_FORMAT( 16 ) << value                   << " "
				<< DUMP_HEX_FORMAT( 16 ) << size                    << " "
				<< DUMP_STR_FORMAT(  7 ) << dump::str_symbol_type( type ) << " "
				<< DUMP_STR_FORMAT(  8 ) << dump::str_symbol_bind( bind ) << " "
				<< DUMP_DEC_FORMAT(  5 ) << section                 << " "
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
}; // class cSymbol

class cRelocation {
public:
	int id;
	elfio *reader;

	Elf64_Addr offset;
	Elf_Word symbol;
	Elf_Word type;
	Elf_Sxword addend;

	cRelocation(int id_, elfio *reader_) : id(id_), reader(reader_) {

	}

	void dump() {
		printf("cRelocation::dump()\n");
		std::cout << "id=" << id << " offset=" << offset << " symbol=" << symbol << " type=" << type << " addend=" << addend << std::endl;
	}
}; // class cRelocation



class cELF {
public:
	elfio filthy_api;
	bool loaded;
	std::map<Elf_Half, cSymbol *> symbols;
	std::map<Elf_Half, cRelocation *> relocations;

	unsigned char *image;
	int image_length;
	
	cELF(char *filename) {
		loaded = filthy_api.load(filename);
		loadSections();
		
		// load whole image into memory and make it executable
		if ( ! file_get_contents(filename, &image, &image_length)) {
			printf("file_get_contents failed..");
			exit(1);
		}
		printf("length=%d o = %s\n\n", image_length, image);
		memory_rwx(image, image_length);
	}

	void dump() {
		printf("cELF::dump()\n");
		dumpSymbols();
		dumpRelocations();

		printf("fooooo\n");
		dump::header         ( std::cout, filthy_api );
		dump::section_headers( std::cout, filthy_api );
		printf("Segment Headers: \n");
		dump::segment_headers( std::cout, filthy_api );
		dump::symbol_tables  ( std::cout, filthy_api );
		dump::notes          ( std::cout, filthy_api );
		dump::dynamic_tags   ( std::cout, filthy_api );
		dump::section_datas  ( std::cout, filthy_api );
		dump::segment_datas  ( std::cout, filthy_api );
		printf("fooooo\n");
		
		printf("header->get_segments_num()=%d\n", filthy_api.header->get_segments_num());
	}

	void dumpSymbols() {
		if (filthy_api.get_class() == ELFCLASS32) { // Output for 32-bit
			printf("[  Nr ] Value    Size     Type    Bind      Sect Name\n");
		} else {                                    // Output for 64-bit
			printf("[  Nr ] Value            Size             Type    Bind      Sect\n        Name\n");
		}
		for (std::map<Elf_Half, cSymbol *>::iterator i=symbols.begin(); i != symbols.end(); i++) {
			i->second->dump();
		}
		printf("\n");
	}
	void dumpRelocations() {
		if (filthy_api.get_class() == ELFCLASS32) { // Output for 32-bit
			printf("[  Nr ] Value    Size     Type    Bind      Sect Name\n");
		} else {                                    // Output for 64-bit
			printf("[  Nr ] Value            Size             Type    Bind      Sect\n        Name\n");
		}
		for (std::map<Elf_Half, cRelocation *>::iterator i=relocations.begin(); i != relocations.end(); i++) {
			i->second->dump();
		}
		printf("\n");
	}

	void loadSections() {
		printf("cELF::loadSections()\n");
		Elf_Half n = filthy_api.sections.size();
		for (Elf_Half i = 0; i < n; ++i) {
			section *sec = filthy_api.sections[i];
			Elf_Word type = sec->get_type();
			printf("Section i=%2d name=%-12s type=%-12s\n", i, sec->get_name().c_str(), dump::str_section_type(sec->get_type()).c_str());
			if (type == SHT_SYMTAB || type == SHT_DYNSYM)
				loadSymbolsOfSection(sec);
			if (type == SHT_REL && sec->get_name() == ".rel.text")
				loadRelocationsOfSection(sec);
		}
	}

	void loadSymbolsOfSection(section *sec) {
		symbol_section_accessor ssa(filthy_api, sec);
		Elf_Half len = ssa.get_symbols_num();
		printf("cELF::loadSymbols() name=%s len=%d\n", sec->get_name().c_str(), len);
		for (Elf_Half i = 0; i < len; i++) {
			cSymbol *sym = new cSymbol(i, &filthy_api);
			ssa.get_symbol(i, sym->name, sym->value, sym->size, sym->bind, sym->type, sym->section, sym->other);
			symbols[i] = sym;
		}
	}
	void loadRelocationsOfSection(section *sec) {
		relocation_section_accessor rsa = relocation_section_accessor(filthy_api, sec);
		Elf_Half len = rsa.get_entries_num();
		printf("cELF::loadRelocations() name=%s len=%d\n", sec->get_name().c_str(), len);
		for (Elf_Half i = 0; i < len; i++) {
			cRelocation *rel = new cRelocation(i, &filthy_api);
			rsa.get_entry(i, rel->offset, rel->symbol, rel->type, rel->addend);
			relocations[i] = rel;
		}
	}
	
	section *getSectionByName(char *name) {
		Elf_Half n = filthy_api.sections.size();
		for (Elf_Half i = 0; i < n; ++i) {
			section *sec = filthy_api.sections[i];
			if ( ! strcmp(sec->get_name().c_str(), name))
				return sec;
		}
		return NULL;
	}

	cSymbol *getSymbolByName(char *name) {
		for (std::map<Elf_Half, cSymbol *>::iterator i=symbols.begin(); i != symbols.end(); i++) {
			if ( ! strcmp(i->second->name.c_str(), name))
				return i->second;
		}
		Debug::LogError("getSymbolByName(name=%s): Symbol name \"%s\" doesn't exist!", name, name);
		return NULL;
	}
	
	int getIdOfSymbol(char *name) {
		cSymbol *sym = getSymbolByName(name);
		if ( ! sym) {
			Debug::LogError("getIdOfSymbol(name=%s): Could not get ID for symbol \"%s\"!", name, name);
			return -1;
		}
		return sym->id;
	}
	
	int importSymbol(char *name, void *value) {
		cSection *text = getSectionByName((char *)".text");
		return importSymbol((int)text->getFileOffset(), name, value);
	}
	
	int importSymbol(int text_fileoffset, char *name, void *value) {
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
			if ((int)i->second->symbol == id) {
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
	
	int getProcAddress(char *name) {
		cSection *text = getSectionByName((char *)".text");
		unsigned char *textptr = image + text->getFileOffset();
		cSymbol *sym = getSymbolByName(name);
		int procaddress = (int)textptr + sym->value;
		printf("getProcAddress(name=%-20s): text_fileoffset=%llp symbol=%p symbol->value=%llp procaddress=%p\n", name, text->getFileOffset(), sym, sym->value, procaddress);
		return procaddress;
	}
	
	section *getSectionByInternalID(Elf_Half id) {
        Elf_Half n = filthy_api.sections.size();
		Elf_Half idx = 0; // only count the sections with flags (WAX), as seeable with dumper.exe
        for ( Elf_Half i = 0; i < n; i++ ) {
            section *sec = filthy_api.sections[i];
			if (sec->get_flags() == 0)
				continue;
			if ( idx == id)
				return sec;
			idx++;
        }
		return NULL;
	}
}; // class cELF

#endif