@echo off
for %%i in (*.c) do gcc -c -s -O3 -std=gnu99 %%i -w -o obj\%%~ni.o
g++ -w -s -O3 obj\*.o -shared -o ..\bass_openmpt.dll -Wl,--add-stdcall-alias -Wl,--out-implib,libbassopenmpt.a -L.. -lbass -L. -lopenmpt
