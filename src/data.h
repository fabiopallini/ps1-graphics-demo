#ifndef DATA_H 
#define DATA_H

typedef struct Npc {
	int talk_pages;
	char **talk_chars;
} Npc;

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
	PlaneData planes[5];
	SpawnData spawns[5];
	ZoneData zones[5];
	unsigned char planes_len;
	unsigned char spawns_len;
	unsigned char zones_len;
	Npc npc;
} StageData;

#endif
