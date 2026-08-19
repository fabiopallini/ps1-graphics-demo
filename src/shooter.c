#include "shooter.h"
#include "utils.h"
#include <string.h>

#define SHOOTER_PLATFORMS 7
#define SHOOTER_ENEMIES 6
#define SHOOTER_SHOTS 10
#define WORLD_END 5200
#define PLATFORM_TILE_WIDTH 256
#define PLATFORM_MAX_TILES 10
#define CAMERA_BASE_Y 300
#define CAMERA_BASE_Z 1050
#define CAMERA_BASE_RX 185

/* Smaller OT values are drawn later and therefore appear in front. */
#define SHOOTER_OT_BODY 8
#define SHOOTER_OT_SHOT 6
#define SHOOTER_OT_SPECIAL 3

#define NR_FRAME_W 64
#define NR_FRAME_H 64
#define NR_RANGER_V 8
#define NR_ENEMY_V 96
#define NR_RANGER_IDLE 0
#define NR_RANGER_RUN 1
#define NR_RANGER_FIRE 2
#define NR_RANGER_JUMP 3
#define NR_ENEMY_IDLE 0
#define NR_ENEMY_FIRE 1
#define NR_PROJECTILE_FRAME 2
#define NR_BURST_FRAME 3

typedef struct ShooterActor {
	Sprite body;
	long vx, vy;
	int hp;
	u_char active;
} ShooterActor;

typedef struct ShooterPlatform {
	Mesh mesh[PLATFORM_MAX_TILES];
	long x, y, w;
	int tile_count;
} ShooterPlatform;

static ShooterActor hero;
static ShooterActor enemies[SHOOTER_ENEMIES];
static ShooterActor shots[SHOOTER_SHOTS];
static ShooterPlatform platforms[SHOOTER_PLATFORMS];
static int special_timer;
static int spawn_timer;
static int score;
static int grounded;
static int shot_cursor;
static int hero_fire_timer;
static int run_anim_timer;
static int hero_direction;
static u_char shooter_loaded;
static u_short tpage_nr1;
static Sprite burst;

static void actor_init(ShooterActor *a, int w, int h, u_short tpage,
	short u, short v) {
	memset(a, 0, sizeof(ShooterActor));
	sprite_init(&a->body, w, h, tpage);
	sprite_set_uv(&a->body, u, v, NR_FRAME_W, NR_FRAME_H);
	a->active = 1;
}

static void platform_init(ShooterPlatform *p, long x, long y, long z, long w, long d, u_char shade) {
	int i;
	long tile_x;
	long tile_w;
	p->x = x;
	p->y = y;
	p->w = w;
	p->tile_count = (w + PLATFORM_TILE_WIDTH - 1) / PLATFORM_TILE_WIDTH;
	if(p->tile_count > PLATFORM_MAX_TILES)
		p->tile_count = PLATFORM_MAX_TILES;
	for(i = 0; i < p->tile_count; i++) {
		tile_x = i * PLATFORM_TILE_WIDTH;
		tile_w = w - tile_x;
		if(tile_w > PLATFORM_TILE_WIDTH)
			tile_w = PLATFORM_TILE_WIDTH;
		mesh_init(&p->mesh[i], (u_long *)plane_vertices(), 0, 0, 0, 1);
		p->mesh[i].vertices[1].vx = tile_w;
		p->mesh[i].vertices[3].vx = tile_w;
		p->mesh[i].vertices[0].vz = -d;
		p->mesh[i].vertices[1].vz = -d;
		p->mesh[i].pos.vx = x + tile_x;
		p->mesh[i].pos.vy = y;
		p->mesh[i].pos.vz = z;
		mesh_set_rgb(&p->mesh[i], 20, shade, shade + 35, 0);
	}
}

static long ground_at(long x, long previous_y, long next_y) {
	int i;
	long best = 30000;
	for(i = 0; i < SHOOTER_PLATFORMS; i++) {
		ShooterPlatform *p = &platforms[i];
		if(x >= p->x && x <= p->x + p->w && previous_y <= p->y - 46 && next_y >= p->y - 46 && p->y < best)
			best = p->y - 46;
	}
	return best;
}

static void fire_shot(void) {
	ShooterActor *s = &shots[shot_cursor++ % SHOOTER_SHOTS];
	s->active = 1;
	s->body.pos = hero.body.pos;
	s->body.pos.vx += 48 * hero_direction;
	s->body.pos.vy -= 8;
	s->vx = 32 * hero_direction;
	if(hero_direction < 0)
		s->body.mirror_h = 1;
	else
		s->body.mirror_h = 0;
	sprite_set_uv(&s->body, NR_PROJECTILE_FRAME * NR_FRAME_W,
		NR_ENEMY_V, NR_FRAME_W, NR_FRAME_H);
}

void shooter_load(void) {
	int i, j;
	u_long *buffer_nr1;
	scene_free();
	if(shooter_loaded) {
		for(i = 0; i < SHOOTER_PLATFORMS; i++) {
			for(j = 0; j < platforms[i].tile_count; j++)
				mesh_free(&platforms[i].mesh[j]);
		}
	}
	if(!shooter_loaded) {
		cd_read_file("SHOOTER\\NR1.TIM", &buffer_nr1);
		tpage_nr1 = loadToVRAM(buffer_nr1);
		free3(buffer_nr1);
	}
	actor_init(&hero, 24, 42, tpage_nr1,
		NR_RANGER_IDLE * NR_FRAME_W, NR_RANGER_V);
	hero.hp = 5;
	hero.body.pos.vx = 100;
	hero.body.pos.vy = -46;
	hero.body.pos.vz = 0;

	for(i = 0; i < SHOOTER_ENEMIES; i++) {
		actor_init(&enemies[i], 24, 24, tpage_nr1,
			NR_ENEMY_IDLE * NR_FRAME_W, NR_ENEMY_V);
		enemies[i].body.pos.vx = 720 + i * 720;
		enemies[i].body.pos.vy = -90 - (i & 1) * 70;
		enemies[i].body.pos.vz = 0;
		enemies[i].hp = 2;
	}
	for(i = 0; i < SHOOTER_SHOTS; i++) {
		actor_init(&shots[i], 12, 6, tpage_nr1,
			NR_PROJECTILE_FRAME * NR_FRAME_W, NR_ENEMY_V);
		shots[i].active = 0;
	}
	sprite_init(&burst, 70, 70, tpage_nr1);
	sprite_set_uv(&burst, NR_BURST_FRAME * NR_FRAME_W,
		NR_ENEMY_V, NR_FRAME_W, NR_FRAME_H);

	platform_init(&platforms[0], -500, 0, 170, 1800, 340, 65);
	platform_init(&platforms[1], 1400, 0, 170, 900, 340, 75);
	platform_init(&platforms[2], 2450, 0, 170, 1000, 340, 85);
	platform_init(&platforms[3], 3600, 0, 170, 2100, 340, 95);
	platform_init(&platforms[4], 850, -180, 110, 360, 220, 110);
	platform_init(&platforms[5], 2050, -135, 80, 300, 180, 120);
	platform_init(&platforms[6], 3260, -210, 130, 420, 240, 130);

	camera.pos.vx = 0;
	camera.pos.vy = CAMERA_BASE_Y;
	camera.pos.vz = CAMERA_BASE_Z;
	camera.rot.vx = CAMERA_BASE_RX;
	camera.rot.vy = 0;
	camera.rot.vz = 0;
	special_timer = 0;
	spawn_timer = 0;
	score = 0;
	grounded = 1;
	shot_cursor = 0;
	hero_fire_timer = 0;
	run_anim_timer = 0;
	hero_direction = 1;
	shooter_loaded = 1;
}

void shooter_update(void) {
	int i, j;
	long floor_y;
	long previous_y = hero.body.pos.vy;
	long target_camera_y;
	if(hero.hp <= 0 || hero.body.pos.vy > 500) {
		shooter_load();
		return;
	}

	hero.vx = 0;
	if(pad & PADLleft) {
		hero.vx = -12;
		hero_direction = -1;
		hero.body.mirror_h = 1;
	}
	if(pad & PADLright) {
		hero.vx = 12;
		hero_direction = 1;
		hero.body.mirror_h = 0;
	}
	if((pad & PADLcross) && !(opad & PADLcross) && grounded) {
		hero.vy = -31;
		grounded = 0;
	}
	if((pad & PADLsquare) && !(opad & PADLsquare)) {
		fire_shot();
		hero_fire_timer = 8;
	}
	if((pad & PADLtriangle) && !(opad & PADLtriangle) && special_timer == 0) special_timer = 75;

	hero.body.pos.vx += hero.vx;
	if(hero.body.pos.vx < 0) hero.body.pos.vx = 0;
	if(hero.body.pos.vx > WORLD_END) hero.body.pos.vx = WORLD_END;
	hero.vy += 2;
	if(hero.vy > 28) hero.vy = 28;
	hero.body.pos.vy += hero.vy;
	floor_y = ground_at(hero.body.pos.vx, previous_y, hero.body.pos.vy);
	if(floor_y != 30000) {
		hero.body.pos.vy = floor_y;
		hero.vy = 0;
		grounded = 1;
	} else grounded = 0;
	if(!grounded) {
		sprite_set_uv(&hero.body, NR_RANGER_JUMP * NR_FRAME_W,
			NR_RANGER_V, NR_FRAME_W, NR_FRAME_H);
	} else if(hero_fire_timer > 0) {
		sprite_set_uv(&hero.body, NR_RANGER_FIRE * NR_FRAME_W,
			NR_RANGER_V, NR_FRAME_W, NR_FRAME_H);
	} else if(hero.vx != 0) {
		sprite_set_uv(&hero.body, NR_RANGER_RUN * NR_FRAME_W,
			NR_RANGER_V, NR_FRAME_W, NR_FRAME_H);
		run_anim_timer++;
		if(run_anim_timer >= 8)
			run_anim_timer = 0;
	} else {
		sprite_set_uv(&hero.body, NR_RANGER_IDLE * NR_FRAME_W,
			NR_RANGER_V, NR_FRAME_W, NR_FRAME_H);
		run_anim_timer = 0;
	}
	if(hero_fire_timer > 0)
		hero_fire_timer--;

	for(i = 0; i < SHOOTER_SHOTS; i++) if(shots[i].active) {
		shots[i].body.pos.vx += shots[i].vx;
		if(labs(shots[i].body.pos.vx - hero.body.pos.vx) > 900)
			shots[i].active = 0;
		for(j = 0; j < SHOOTER_ENEMIES; j++) if(enemies[j].active &&
			labs(shots[i].body.pos.vx - enemies[j].body.pos.vx) < 42 &&
			labs(shots[i].body.pos.vy - enemies[j].body.pos.vy) < 45) {
			shots[i].active = 0;
			if(--enemies[j].hp <= 0) { enemies[j].active = 0; score += 100; }
		}
	}

	for(i = 0; i < SHOOTER_ENEMIES; i++) if(enemies[i].active) {
		if(enemies[i].body.pos.vx > hero.body.pos.vx)
			enemies[i].body.mirror_h = 1;
		else
			enemies[i].body.mirror_h = 0;
		if(labs(enemies[i].body.pos.vx - hero.body.pos.vx) < 350) {
			sprite_set_uv(&enemies[i].body, NR_ENEMY_FIRE * NR_FRAME_W,
				NR_ENEMY_V, NR_FRAME_W, NR_FRAME_H);
		} else {
			sprite_set_uv(&enemies[i].body, NR_ENEMY_IDLE * NR_FRAME_W,
				NR_ENEMY_V, NR_FRAME_W, NR_FRAME_H);
		}
		if(enemies[i].body.pos.vx > hero.body.pos.vx) enemies[i].body.pos.vx -= 3;
		else enemies[i].body.pos.vx += 2;
		if(labs(enemies[i].body.pos.vx - hero.body.pos.vx) < 45 &&
			labs(enemies[i].body.pos.vy - hero.body.pos.vy) < 65 && spawn_timer == 0) {
			hero.hp--;
			spawn_timer = 55;
		}
		if(special_timer > 44 && labs(enemies[i].body.pos.vx - hero.body.pos.vx) < 340) {
			enemies[i].active = 0;
			score += 100;
		}
	}
	if(spawn_timer > 0) spawn_timer--;
	if(special_timer > 0) special_timer--;

	camera.pos.vx += ((-hero.body.pos.vx + 120) - camera.pos.vx) / 8;
	if(camera.pos.vx > 0) camera.pos.vx = 0;
	target_camera_y = CAMERA_BASE_Y - (hero.body.pos.vy + 46);
	camera.pos.vy += (target_camera_y - camera.pos.vy) / 8;
	if(special_timer > 0) {
		camera.rot.vy = (special_timer - 38) * 3;
		camera.rot.vz = (special_timer & 7) - 4;
		camera.pos.vz = 980;
	} else {
		camera.rot.vy -= camera.rot.vy / 4;
		camera.rot.vz = 0;
		camera.pos.vz += (CAMERA_BASE_Z - camera.pos.vz) / 5;
	}
}

static void actor_draw(ShooterActor *a) {
	if(!a->active) return;
	drawSprite3D(&a->body, SHOOTER_OT_BODY);
}

void shooter_draw(void) {
	int i, j;
	long hero_draw_y;
	char hud[64];
	/* Keep real 3D depth for clipping; actors use fixed foreground layers. */
	for(i = 0; i < SHOOTER_PLATFORMS; i++) {
		for(j = 0; j < platforms[i].tile_count; j++) {
			Mesh *tile = &platforms[i].mesh[j];
			if(tile->pos.vx + PLATFORM_TILE_WIDTH >= hero.body.pos.vx - 800 &&
				tile->pos.vx <= hero.body.pos.vx + 900)
				drawMesh(tile, 0);
		}
	}
	hero_draw_y = hero.body.pos.vy;
	if(grounded && hero.vx != 0 && run_anim_timer >= 4)
		hero.body.pos.vy -= 2;
	actor_draw(&hero);
	hero.body.pos.vy = hero_draw_y;
	for(i = 0; i < SHOOTER_ENEMIES; i++) actor_draw(&enemies[i]);
	for(i = 0; i < SHOOTER_SHOTS; i++) {
		if(shots[i].active)
			drawSprite3D(&shots[i].body, SHOOTER_OT_SHOT);
	}

	if(special_timer > 0) {
		burst.pos = hero.body.pos;
		burst.w = 55 + (special_timer & 15) * 3;
		burst.h = burst.w;
		drawSprite3D(&burst, SHOOTER_OT_SPECIAL);
		drawFont("PHOTON BURST!", 112, 72, 0);
	}
	sprintf(hud, "HP %d   SCORE %d", hero.hp, score);
	drawFont(hud, 8, 8, 0);
	drawFont("X JUMP  SQUARE FIRE  TRIANGLE SPECIAL", 15, 237, 0);
	if(hero.body.pos.vx >= WORLD_END) drawFont("MISSION COMPLETE", 105, 105, 0);
}
