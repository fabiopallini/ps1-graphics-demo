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
	unsigned short HP, HP_MAX, MP, MP_MAX, SPEED;
	unsigned char LV, STR, INT, VIT, AGI, MND; 
	unsigned int EXP;
	VECTOR map_pos, battle_pos;
	SVECTOR map_rot, battle_rot;
	u_char hitted;
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
	Mesh planes[PLANES_LEN];
	Spawn spawns[5];
	Zone zones[5];
	unsigned char planes_length;
	unsigned char spawns_length;
	unsigned char zones_length;
	unsigned char npcs_len;
	Npc npcs[5];
} Stage;

typedef struct Background {
	Sprite s0,s1;
	u_short tpages[2];
} Background;

typedef struct Selector {
	Sprite sprite;
	u_char index;
} Selector;

typedef struct Menu {
	u_char status;
	Window win_main, win_sidebar;
	Selector selector;
} Menu;

typedef enum PLANE_EDIT_STATUS {
	PLANE_NONE, // no plane to edit
	PLANE_POS, // positioning the plane
} PLANE_EDIT_STATUS;

typedef enum BATTLE_MODE {
	BATTLE_OFF,
	BATTLE_START, // camera transition view
	BATTLE_END,
	BATTLE_WAIT, // battle is running, waiting ATBs to load
	BATTLE_SELECT_TARGET,
	BATTLE_ATTACK,
	BATTLE_REPOS, // repos player at original fight position
	BATTLE_SUBMENU
} BATTLE_MODE;

typedef enum MENU_COMMANDS {
	COMMAND_ATTACK,
	/*COMMAND_MAGIC,
	COMMAND_SKILL,*/
	COMMAND_ITEM
} MENU_COMMANDS;

typedef struct Item {
	unsigned short id;
	char name[20];
} Item;

typedef struct Inventory {
	int index;
	Node *node;
} Inventory;

void npc_init(Npc *npc, u_long *cd_obj, u_short tpage, const NpcData *npcData);
void npc_free(Npc *npc);
void npc_update(Npc *npc);
void zone_init(Zone *zone, long posX, long posY, long posZ, int w, int h, int z, int stage_id, int spawn_id);
u_char plane_add(Mesh planes[], unsigned char *planes_len);
void print_planes(Mesh planes[], unsigned char planes_len);
void print_bytes(u_long *buffer, size_t size);
void background_init(Background *b);
void background_draw(Background *b, long otz, void(*draw)(Sprite *sprite, long otz));
size_t strcpy_count(char *destination, const char *source);
const u_char *plane_vertices();
unsigned int nextLevel(unsigned int current_lv);
void menu_init(Menu *menu, u_short tpage_ui);
void menu_draw(Menu menu);
void menu_set_selector_index(Menu *menu, u_char index);

#endif
