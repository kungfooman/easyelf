gcc -m32 -c testlib_a.c -o testlib_a.o
gcc -m32 -c testlib_b.c -o testlib_b.o
objcopy.exe --input-target=pe-i386 --output-target=elf32-i386 testlib_a.o testlib_a.elf
objcopy.exe --input-target=pe-i386 --output-target=elf32-i386 testlib_b.o testlib_b.elf
pause