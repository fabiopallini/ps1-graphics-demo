#ifndef DATA_H 
#define DATA_H

#define PLANES_LEN 10 // max planes per stage

typedef struct NpcData {
	int x, y, z;
	short rx, ry, rz; 
	int talk_pages;
	char **talk_chars;
} NpcData;

typedef struct PlaneData {
	int x, y, z;
	int w, h, d;
} PlaneData;

typedef struct SpawnData {
	int x, y, z;
	short rx, ry, rz; 
} SpawnData;

typedef struct ZoneData {
	int x, y, z;
	int w, h, d;
	int stage_id;
	int spawn_id;
} ZoneData;

typedef struct StageData {
	char tims[2][10];
	int cam_x, cam_y, cam_z;
	short cam_rx, cam_ry, cam_rz;
	PlaneData planesData[PLANES_LEN];
	SpawnData spawnsData[5];
	ZoneData zonesData[5];
	unsigned char planesData_len;
	unsigned char spawnsData_len;
	unsigned char zonesData_len;
	unsigned char npcsData_len;
	NpcData npcData[5];
} StageData;

#endif
