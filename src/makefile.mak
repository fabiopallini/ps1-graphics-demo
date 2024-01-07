# ----------------------------
# PlayStation 1 Psy-Q MAKEFILE
# ----------------------------
all:
	ccpsx -O3 -Xo$80010000 -Wall *.c -llibds -omain.cpe,main.sym,mem.map
	cpe2x /ce main.cpe

    ..\mkpsxiso\mkpsxiso.exe -o ..\cdrom\game.iso -y cuesheet.xml

   	del mem.map
	del main.sym
	del main.exe
	del main.cpe