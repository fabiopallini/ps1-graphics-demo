ccpsx -O3 -Xo$80010000 -Wall main.c psx.c sprite.c mesh.c -llibds -o main.cpe
cpe2x /ce main.cpe
del main.cpe

..\mkpsxiso\mkpsxiso.exe -o ..\cdrom\game.iso -y ..\mkpsxiso\cuesheet.xml

del MAIN.EXE