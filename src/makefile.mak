# ----------------------------
# PlayStation 1 Psy-Q MAKEFILE
# ----------------------------

CFLAGS =-O3 -Xo$80010000 -Wall -pedantic
LIBS   =-llibds
DEFS   =-DDEBUG

debug:
	ccpsx $(CFLAGS) *.c $(LIBS) $(DEFS) -omain.cpe,main.sym,mem.map
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

prod:
	ccpsx $(CFLAGS) *.c $(LIBS) -omain.cpe,main.sym,mem.map
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

all: debug 

16bit:
	ccpsx $(CFLAGS) main.c game.c psx.c utils.c battle.c enemy.c $(LIBS) $(DEFS) -omain.cpe
	cpe2x /ce main.cpe

	..\cdrom\buildcd.exe -l -i..\cdrom\temp.img ..\cdrom\CONF.CTI
	..\cdrom\stripiso.exe s 2352 ..\cdrom\temp.img ..\cdrom\GAME.ISO
	del CDW900E.TOC
	del QSHEET.TOC
	del ..\cdrom\TEMP.IMG 
	del main.exe
	del main.cpe
