# ----------------------------
# PlayStation 1 Psy-Q MAKEFILE
# ----------------------------
all:
	wine ccpsx -O3 -Xo$80010000 main.c psx.c -o main.cpe
	pcsxr -nogui -psxout main.cpe
