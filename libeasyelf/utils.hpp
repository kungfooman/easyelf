/*
Copyright (C) 2001-2015 by Serge Lamikhov-Center

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef ELFIO_UTILS_HPP
#define ELFIO_UTILS_HPP


#include <list>

#define ELFIO_GET_ACCESS( TYPE, NAME, FIELD ) \
    TYPE get_##NAME() const                   \
    {                                         \
        return (*convertor)( FIELD );         \
    }
#define ELFIO_SET_ACCESS( TYPE, NAME, FIELD ) \
    void set_##NAME( TYPE value )             \
    {                                         \
        FIELD = value;                        \
        FIELD = (*convertor)( FIELD );        \
    }
#define ELFIO_GET_SET_ACCESS( TYPE, NAME, FIELD ) \
    TYPE get_##NAME() const                       \
    {                                             \
        return (*convertor)( FIELD );             \
    }                                             \
    void set_##NAME( TYPE value )                 \
    {                                             \
        FIELD = value;                            \
        FIELD = (*convertor)( FIELD );            \
    }

#define ELFIO_GET_ACCESS_DECL( TYPE, NAME ) \
    virtual TYPE get_##NAME() const = 0

#define ELFIO_SET_ACCESS_DECL( TYPE, NAME ) \
    virtual void set_##NAME( TYPE value ) = 0

#define ELFIO_GET_SET_ACCESS_DECL( TYPE, NAME ) \
    virtual TYPE get_##NAME() const = 0;        \
    virtual void set_##NAME( TYPE value ) = 0

namespace ELFIO {

//------------------------------------------------------------------------------
class endianess_convertor {
  public:
//------------------------------------------------------------------------------
    endianess_convertor()
    {
        need_conversion = false;
    }

//------------------------------------------------------------------------------
    void
    setup( unsigned char elf_file_encoding )
    {
        need_conversion = ( elf_file_encoding != get_host_encoding() );
    }

//------------------------------------------------------------------------------
    uint64_t
    operator()( uint64_t value ) const
    {
        if ( !need_conversion ) {
            return value;
        }
        value =
            ( ( value & 0x00000000000000FFull ) << 56 ) |
            ( ( value & 0x000000000000FF00ull ) << 40 ) |
            ( ( value & 0x0000000000FF0000ull ) << 24 ) |
            ( ( value & 0x00000000FF000000ull ) <<  8 ) |
            ( ( value & 0x000000FF00000000ull ) >>  8 ) |
            ( ( value & 0x0000FF0000000000ull ) >> 24 ) |
            ( ( value & 0x00FF000000000000ull ) >> 40 ) |
            ( ( value & 0xFF00000000000000ull ) >> 56 );

        return value;
    }

//------------------------------------------------------------------------------
    int64_t
    operator()( int64_t value ) const
    {
        if ( !need_conversion ) {
            return value;
        }
        return (int64_t)(*this)( (uint64_t)value );
    }

//------------------------------------------------------------------------------
    uint32_t
    operator()( uint32_t value ) const
    {
        if ( !need_conversion ) {
            return value;
        }
        value =
            ( ( value & 0x000000FF ) << 24 ) |
            ( ( value & 0x0000FF00 ) <<  8 ) |
            ( ( value & 0x00FF0000 ) >>  8 ) |
            ( ( value & 0xFF000000 ) >> 24 );

        return value;
    }

//------------------------------------------------------------------------------
    int32_t
    operator()( int32_t value ) const
    {
        if ( !need_conversion ) {
            return value;
        }
        return (int32_t)(*this)( (uint32_t)value );
    }

//------------------------------------------------------------------------------
    uint16_t
    operator()( uint16_t value ) const
    {
        if ( !need_conversion ) {
            return value;
        }
        value =
            ( ( value & 0x00FF ) <<  8 ) |
            ( ( value & 0xFF00 ) >>  8 );

        return value;
    }

//------------------------------------------------------------------------------
    int16_t
    operator()( int16_t value ) const
    {
        if ( !need_conversion ) {
            return value;
        }
        return (int16_t)(*this)( (uint16_t)value );
    }

//------------------------------------------------------------------------------
    int8_t
    operator()( int8_t value ) const
    {
        return value;
    }

//------------------------------------------------------------------------------
    uint8_t
    operator()( uint8_t value ) const
    {
        return value;
    }

//------------------------------------------------------------------------------
  private:
//------------------------------------------------------------------------------
    unsigned char
    get_host_encoding() const
    {
        static const int tmp = 1;
        if ( 1 == *(char*)&tmp ) {
            return ELFDATA2LSB;
        }
        else {
            return ELFDATA2MSB;
        }
    }

//------------------------------------------------------------------------------
  private:
    bool need_conversion;
};


//------------------------------------------------------------------------------
inline
uint32_t
elf_hash( const unsigned char *name )
{
    uint32_t h = 0, g;
    while ( *name ) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;
        if ( g != 0 )
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

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

void cracking_hook_call(int from, int to)
{
	int relative = to - (from+5); // +5 is the position of next opcode
	memcpy((void *)(from+1), &relative, 4); // set relative address with endian
}

std::string string_format(const std::string fmt, ...) {
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}


class Debug {
	public:
	static std::list<std::string> *last_errors;
	static void LogError(const char *error, ...);
	static std::list<std::string> *getLastErrors();
	static void printLastErrors(int depth);
	static void cleanLastErrors();
};

	std::list<std::string> *Debug::last_errors = NULL;
	void Debug::LogError(const char *error, ...) {
		if ( ! last_errors)
			last_errors = new std::list<std::string>();
		va_list myargs;
		va_start(myargs, error);
		char tmp[1024];
		vsnprintf(tmp, sizeof(tmp), error, myargs);
		va_end(myargs);
		
		last_errors->push_back(tmp);
		
		//printf("!!!!!!!!!!!!ERROR!!!!!!!!!!!!!: %s\n", tmp);
		
	}
	std::list<std::string> *Debug::getLastErrors() {
		return last_errors;
	}
	void Debug::printLastErrors(int depth = 0) {
		std::string depthstr;
		for (int i=0; i<depth; i++)
			depthstr += "    ";
		if ( ! last_errors)
			return;
		int errors = last_errors->size();
		//printf("errors: %d\n", errors);
		int idx = 0;
		for (std::list<std::string>::iterator i=last_errors->begin(); i != last_errors->end(); i++) {
			printf("%serrors[%d] = %s\n", depthstr.c_str(), idx, (*i).c_str());
			idx++;
		}
	}
	void Debug::cleanLastErrors() {
		if (last_errors)
			delete last_errors;
		last_errors = NULL;
	}

} // namespace ELFIO

#endif // ELFIO_UTILS_HPP
