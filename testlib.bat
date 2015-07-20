gcc -m32 -c testlib.c -o testlib.o
objcopy.exe --input-target=pe-i386 --output-target=elf32-i386 testlib.o testlib.elf
pause