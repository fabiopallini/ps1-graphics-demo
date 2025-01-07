#ifndef UTILS_H
#define UTILS_H

#include "psx.h"
#include "data.h"

#define ROT_LEFT 1024 
#define ROT_RIGHT 3072 
#define ROT_UP 2048 
#define ROT_DOWN 0

#define ROT_UP_LEFT 1536
#define ROT_UP_RIGHT 2560
#define ROT_DOWN_LEFT 512
#define ROT_DOWN_RIGHT 3584

typedef struct Entity {
	Sprite sprite;
	Model model;
	unsigned int HP, HP_MAX, MP, MP_MAX, SPEED;
	unsigned char STR, INT, VIT, AGI, MND; 
	VECTOR map_pos, battle_pos;
	SVECTOR map_rot, battle_rot;
} Entity;

typedef struct Npc {
	Mesh mesh;
	BBox bbox;
	int talk_pages;
	char **talk_chars;
} Npc;

typedef struct Spawn {
	VECTOR pos;
	SVECTOR rot;
} Spawn;

typedef struct Zone {
	VECTOR pos;
	int w, h, z;
	Mesh mesh;
	int stage_id;
	int spawn_id;
} Zone;

typedef struct Stage {
	unsigned int id;
	char *tims[2];
	VECTOR camera_pos;
	SVECTOR camera_rot;
	Mesh planes[10];
	Spawn spawns[5];
	Zone zones[5];
	unsigned char planes_length;
	unsigned char spawns_length;
	unsigned char zones_length;
	unsigned char npcs_len;
	Npc npcs[5];
} Stage;
StageData stageData;

typedef struct Background {
	Sprite s0,s1;
	u_short tpages[2];
} Background;

typedef enum {
	BATTLE_OFF,
	BATTLE_START,
	BATTLE_END,
	BATTLE_WAIT,
	BATTLE_SELECT_TARGET,
	BATTLE_ATTACK,
	BATTLE_REPOS, // repos player at original fight position
	BATTLE_SUBMENU
} BATTLE_MODE;

typedef enum {
	COMMAND_ATTACK,
	/*COMMAND_MAGIC,
	COMMAND_SKILL,*/
	COMMAND_ITEM
} MENU_COMMANDS;

typedef struct {
	unsigned short id;
	char name[20];
} Item;

typedef struct {
	int index;
	Node *node;
} Inventory;

void npc_init(Npc *npc, u_long *cd_obj, u_short tpage, const NpcData *npcData);
void npc_free(Npc *npc);
void npc_update(Npc *npc);
void zone_init(Zone *zone, long posX, long posY, long posZ, int w, int h, int z, int stage_id, int spawn_id);
void print_bytes(u_long *buffer, size_t size);
void background_init(Background *b);
void background_draw(Background *b, long otz, void(*draw)(Sprite *sprite, long otz));
size_t strcpy_count(char *destination, const char *source);

#endif
