# ----------------------------
# PlayStation 1 Psy-Q MAKEFILE
# ----------------------------

all:
	ccpsx -O3 -Xo$80010000 -Wall *.c -llibds -omain.cpe,main.sym,mem.map
	cpe2x /ce main.cpe

	..\cdrom\buildcd.exe -l -i..\cdrom\temp.img ..\cdrom\CONF.CTI
	..\cdrom\stripiso.exe s 2352 ..\cdrom\temp.img ..\cdrom\GAME.ISO
	del CDW900E.TOC
	del QSHEET.TOC
	del ..\cdrom\TEMP.IMG 
   	del mem.map
	del main.sym
	del main.exe
	del main.cpe

dev:
	ccpsx -O3 -Xo$80010000 -Wall *.c -llibds -omain.cpe,main.sym,mem.map
	cpe2x /ce main.cpe

	..\cdrom\buildcd.exe -i..\cdrom\temp.img ..\cdrom\DEVCONF.CTI
	..\cdrom\stripiso.exe s 2352 ..\cdrom\temp.img ..\cdrom\GAME.ISO
	del CDW900E.TOC
	del QSHEET.TOC
	del ..\cdrom\TEMP.IMG 
   	del mem.map
	del main.sym
	del main.exe
	del main.cpe

16bit:
	ccpsx -O3 -Xo$80010000 -Wall main.c game.c psx.c sprite.c mesh.c xa.c utils.c enemy.c ui.c char.c -llibds -omain.cpe
	cpe2x /ce main.cpe

	..\cdrom\buildcd.exe -l -i..\cdrom\temp.img ..\cdrom\CONF.CTI
	..\cdrom\stripiso.exe s 2352 ..\cdrom\temp.img ..\cdrom\GAME.ISO
	del CDW900E.TOC
	del QSHEET.TOC
	del ..\cdrom\TEMP.IMG 
   	del mem.map
	del main.sym
	del main.exe
	del main.cpe

dev16:
	ccpsx -O3 -Xo$80010000 -Wall main.c game.c psx.c sprite.c mesh.c xa.c utils.c enemy.c ui.c char.c -llibds -omain.cpe
	cpe2x /ce main.cpe

	..\cdrom\buildcd.exe -i..\cdrom\temp.img ..\cdrom\DEVCONF.CTI
	..\cdrom\stripiso.exe s 2352 ..\cdrom\temp.img ..\cdrom\GAME.ISO
	del CDW900E.TOC
	del QSHEET.TOC
	del ..\cdrom\TEMP.IMG 
   	del mem.map
	del main.sym
	del main.exe
	del main.cpe
