#include "shooter.h"
#include "utils.h"
#include <string.h>

#define SHOOTER_PLATFORMS 7
#define SHOOTER_ENEMIES 6
#define SHOOTER_SHOTS 10
#define WORLD_END 5200
#define PLATFORM_TILE_WIDTH 256
#define PLATFORM_MAX_TILES 10
#define CAMERA_BASE_Y 360
#define CAMERA_BASE_Z 1050
#define CAMERA_BASE_RX 185

/* Smaller OT values are drawn later and therefore appear in front. */
#define SHOOTER_OT_BODY 8
#define SHOOTER_OT_SHOT 6
#define SHOOTER_OT_SPECIAL 3

#define NR_FRAME_W 64
#define NR_FRAME_H 64
#define NR_ENEMY_V 128
#define NR_EFFECT_V 192
#define NR_ENEMY_IDLE 0
#define NR_ENEMY_FIRE 1
#define NR_PROJECTILE_FRAME 0
#define NR_BURST_FRAME 2
#define HERO_FRAME_W 32
#define HERO_GROUND_OFFSET 58
#define HERO_IDLE_ROW 192
#define HERO_RUN_SPEED 7
#define HERO_JUMP_SPEED 23
#define ENEMY_WAKE_DISTANCE 600
#define ENEMY_BASE_Y (20 - HERO_GROUND_OFFSET + 2)
#define RUN_FRAMES 8
#define RUN_FRAME_TICKS 4
#define EXPLOSION_FRAMES 6
#define EXPLOSION_FRAME_TICKS 4
#define EXPLOSION_TICKS (EXPLOSION_FRAMES * EXPLOSION_FRAME_TICKS)
#ifdef PAL
#define GAME_OVER_TICKS (3 * 50)
#else
#define GAME_OVER_TICKS (3 * 60)
#endif
#define SHOT_INTERVAL 12
#define SHOT_LIFETIME 36
#define HIT_RADIUS_X 38
#define HIT_RADIUS_Y 42

typedef struct ShooterActor {
	Sprite body;
	long vx, vy;
	int hp;
	int timer;
	u_char active;
} ShooterActor;

typedef struct ShooterPlatform {
	Mesh mesh[PLATFORM_MAX_TILES];
	long x, y, w, thickness;
	int tile_count;
} ShooterPlatform;

static ShooterActor hero;
static ShooterActor enemies[SHOOTER_ENEMIES];
static ShooterActor shots[SHOOTER_SHOTS];
static ShooterActor explosions[SHOOTER_ENEMIES];
static ShooterPlatform platforms[SHOOTER_PLATFORMS];
static int special_timer;
static int spawn_timer;
static int score;
static int grounded;
static int shot_cursor;
static int hero_fire_timer;
static int run_anim_timer;
static int hero_direction;
static int aim_up;
static int shot_cooldown;
static int hero_frame;
static int hero_row;
static int game_over;
static long game_over_start;
static u_char shooter_loaded;
static u_short tpage_nr1;
static u_short tpage_hero;
static u_short tpage_explosion;
static u_short tpage_bg1;
static u_short tpage_tex1;
static Sprite shooter_background;
static Sprite burst;
static Sprite muzzle_flash;

static char *cuboid_vertices(void) {
	return "v 0.000000 0.000000 0.000000\n"
		"v 1.000000 0.000000 0.000000\n"
		"v 0.000000 0.000000 -1.000000\n"
		"v 1.000000 0.000000 -1.000000\n"
		"v 0.000000 1.000000 0.000000\n"
		"v 1.000000 1.000000 0.000000\n"
		"v 0.000000 1.000000 -1.000000\n"
		"v 1.000000 1.000000 -1.000000\n"
		"vt 0.000000 1.000000\n"
		"vt 0.250000 1.000000\n"
		"vt 0.250000 0.750000\n"
		"vt 0.000000 0.750000\n"
		"vt 0.000000 0.750000\n"
		"vt 0.250000 0.750000\n"
		"vt 0.250000 0.500000\n"
		"vt 0.000000 0.500000\n"
		"vt 0.000000 0.500000\n"
		"vt 0.250000 0.500000\n"
		"vt 0.250000 0.250000\n"
		"vt 0.000000 0.250000\n"
		"f 1/1 2/2 4/3 3/4\n"
		"f 5/5 6/6 8/7 7/8\n"
		"f 1/5 2/6 6/7 5/8\n"
		"f 3/5 4/6 8/7 7/8\n"
		"f 1/9 3/10 7/11 5/12\n"
		"f 2/9 4/10 8/11 6/12\n";
}

static void platform_set_uv(Mesh *mesh, int variant) {
	int u0;
	int u1;
	u0 = (variant * 64) + 4;
	u1 = u0 + 55;

	/* Crop tile borders so adjacent cuboids read as one continuous platform. */
	setUV4(&mesh->poly.ft4[0], u0, 4, u1, 4, u0, 59, u1, 59);
	setUV4(&mesh->poly.ft4[1], u0, 68, u1, 68, u0, 123, u1, 123);
	setUV4(&mesh->poly.ft4[2], u0, 68, u1, 68, u0, 123, u1, 123);
	setUV4(&mesh->poly.ft4[3], u0, 68, u1, 68, u0, 123, u1, 123);
	setUV4(&mesh->poly.ft4[4], u0, 132, u1, 132, u0, 187, u1, 187);
	setUV4(&mesh->poly.ft4[5], u0, 132, u1, 132, u0, 187, u1, 187);
}

static void actor_init(ShooterActor *a, int w, int h, u_short tpage,
	short u, short v) {
	memset(a, 0, sizeof(ShooterActor));
	sprite_init(&a->body, w, h, tpage);
	sprite_set_uv(&a->body, u, v, NR_FRAME_W, NR_FRAME_H);
	a->active = 1;
}

static void platform_init(ShooterPlatform *p, long x, long y, long z,
	long w, long d, long thickness) {
	int i;
	long tile_x;
	long tile_w;
	p->x = x;
	p->y = y;
	p->w = w;
	p->thickness = thickness;
	p->tile_count = (w + PLATFORM_TILE_WIDTH - 1) / PLATFORM_TILE_WIDTH;
	if(p->tile_count > PLATFORM_MAX_TILES)
		p->tile_count = PLATFORM_MAX_TILES;
	for(i = 0; i < p->tile_count; i++) {
		tile_x = i * PLATFORM_TILE_WIDTH;
		tile_w = w - tile_x;
		if(tile_w > PLATFORM_TILE_WIDTH)
			tile_w = PLATFORM_TILE_WIDTH;
		if(i < p->tile_count - 1)
			tile_w++;
		mesh_init(&p->mesh[i], (u_long *)cuboid_vertices(),
			tpage_tex1, 255, 255, 1);
		platform_set_uv(&p->mesh[i], i & 3);
		p->mesh[i].vertices[0].vx = 0;
		p->mesh[i].vertices[0].vy = 0;
		p->mesh[i].vertices[0].vz = 0;
		p->mesh[i].vertices[1].vx = tile_w;
		p->mesh[i].vertices[1].vy = 0;
		p->mesh[i].vertices[1].vz = 0;
		p->mesh[i].vertices[2].vx = 0;
		p->mesh[i].vertices[2].vy = 0;
		p->mesh[i].vertices[2].vz = -d;
		p->mesh[i].vertices[3].vx = tile_w;
		p->mesh[i].vertices[3].vy = 0;
		p->mesh[i].vertices[3].vz = -d;
		p->mesh[i].vertices[4].vx = 0;
		p->mesh[i].vertices[4].vy = thickness;
		p->mesh[i].vertices[4].vz = 0;
		p->mesh[i].vertices[5].vx = tile_w;
		p->mesh[i].vertices[5].vy = thickness;
		p->mesh[i].vertices[5].vz = 0;
		p->mesh[i].vertices[6].vx = 0;
		p->mesh[i].vertices[6].vy = thickness;
		p->mesh[i].vertices[6].vz = -d;
		p->mesh[i].vertices[7].vx = tile_w;
		p->mesh[i].vertices[7].vy = thickness;
		p->mesh[i].vertices[7].vz = -d;
		p->mesh[i].pos.vx = x + tile_x;
		p->mesh[i].pos.vy = y;
		p->mesh[i].pos.vz = z;
	}
}

static long ground_at(long x, long previous_y, long next_y) {
	int i;
	long best = 30000;
	for(i = 0; i < SHOOTER_PLATFORMS; i++) {
		ShooterPlatform *p = &platforms[i];
		if(x >= p->x && x <= p->x + p->w &&
			previous_y <= p->y - HERO_GROUND_OFFSET &&
			next_y >= p->y - HERO_GROUND_OFFSET && p->y - HERO_GROUND_OFFSET < best)
			best = p->y - HERO_GROUND_OFFSET;
	}
	return best;
}

/* Muzzle anchors match the three complete aiming poses in PLAYER. */
static void muzzle_position(VECTOR *pos) {
	*pos = hero.body.pos;
	if(aim_up == 0) {
		pos->vx += 25 * hero_direction;
		pos->vy += 2;
	} else if(aim_up == 1) {
		pos->vx += 27 * hero_direction;
		pos->vy -= 24;
	} else {
		pos->vx += 2 * hero_direction;
		pos->vy -= 40;
	}
	if(!grounded) pos->vy -= 8;
}

static void fire_shot(void) {
	ShooterActor *s;
	long dx, dy, along, across, best;
	int i, target;
	s = &shots[shot_cursor];
	shot_cursor = (shot_cursor + 1) % SHOOTER_SHOTS;
	s->active = 1;
	s->timer = SHOT_LIFETIME;
	muzzle_position(&s->body.pos);
	s->vx = 32 * hero_direction;
	s->vy = 0;
	if(aim_up == 1) {
		s->vx = 23 * hero_direction;
		s->vy = -23;
	} else if(aim_up == 2) {
		s->vx = 0;
		s->vy = -32;
	}
	/* Small launch correction within the chosen direction; no homing. */
	best = 601;
	target = -1;
	for(i = 0; i < SHOOTER_ENEMIES; i++) if(enemies[i].active) {
		dx = enemies[i].body.pos.vx - s->body.pos.vx;
		dy = enemies[i].body.pos.vy - s->body.pos.vy;
		along = (dx * s->vx + dy * s->vy) / 32;
		across = labs((dx * s->vy - dy * s->vx) / 32);
		if(along > 0 && along < best && across <= 28 + along / 6) {
			best = along;
			target = i;
		}
	}
	if(target >= 0) {
		dx = enemies[target].body.pos.vx - s->body.pos.vx;
		dy = enemies[target].body.pos.vy - s->body.pos.vy;
		s->vx = dx * 32 / best;
		s->vy = dy * 32 / best;
	}
	s->body.mirror_h = hero_direction < 0;
	s->body.rot.vz = -aim_up * hero_direction * 512;
	/* Crop the actual bolt instead of shrinking a mostly empty 64px cell. */
	sprite_set_uv(&s->body, 16, NR_EFFECT_V + 24, 32, 20);
}

/* Sample the entire step so fast shots cannot skip a nearby target. */
static int shot_hits(ShooterActor *s, ShooterActor *enemy) {
	int step;
	long x, y;
	for(step = 0; step <= 4; step++) {
		x = s->body.pos.vx + s->vx * step / 4;
		y = s->body.pos.vy + s->vy * step / 4;
		if(labs(x - enemy->body.pos.vx) <= HIT_RADIUS_X &&
			labs(y - enemy->body.pos.vy) <= HIT_RADIUS_Y)
			return 1;
	}
	return 0;
}

static void enemy_destroy(int index) {
	ShooterActor *enemy = &enemies[index];
	ShooterActor *effect = &explosions[index];
	if(!enemy->active) return;
	enemy->active = 0;
	effect->active = 1;
	effect->timer = EXPLOSION_TICKS;
	effect->body.pos = enemy->body.pos;
	effect->body.w = 48;
	effect->body.h = 48;
	sprite_set_uv(&effect->body, 0, 0, NR_FRAME_W, NR_FRAME_H);
	score += 100;
}

static void explosions_update(void) {
	int i, frame;
	for(i = 0; i < SHOOTER_ENEMIES; i++) if(explosions[i].active) {
		if(--explosions[i].timer <= 0) {
			explosions[i].active = 0;
			continue;
		}
		frame = (EXPLOSION_TICKS - explosions[i].timer) / EXPLOSION_FRAME_TICKS;
		sprite_set_uv(&explosions[i].body, (frame % 3) * NR_FRAME_W,
			(frame / 3) * NR_FRAME_H, NR_FRAME_W, NR_FRAME_H);
	}
}

static void game_over_begin(void) {
	game_over = 1;
	game_over_start = VSync(-1);
	hero_fire_timer = 0;
	spawn_timer = 0;
	special_timer = 0;
}

void shooter_load(void) {
	int i, j;
	u_long *buffer_nr1;
	u_long *buffer_hero;
	u_long *buffer_explosion;
	u_long *buffer_bg1;
	u_long *buffer_tex1;
	scene_free();
	if(shooter_loaded) {
		for(i = 0; i < SHOOTER_PLATFORMS; i++) {
			for(j = 0; j < platforms[i].tile_count; j++)
				mesh_free(&platforms[i].mesh[j]);
		}
	}
	if(!shooter_loaded) {
		cd_read_file("SHOOTER\\NR1.TIM", &buffer_nr1);
		cd_read_file("SHOOTER\\PLAYER.TIM", &buffer_hero);
		cd_read_file("SHOOTER\\FX1.TIM", &buffer_explosion);
		cd_read_file("SHOOTER\\BG1.TIM", &buffer_bg1);
		cd_read_file("SHOOTER\\TEX1.TIM", &buffer_tex1);
		tpage_nr1 = loadToVRAM(buffer_nr1);
		tpage_hero = loadToVRAM(buffer_hero);
		tpage_explosion = loadToVRAM(buffer_explosion);
		tpage_bg1 = loadToVRAM(buffer_bg1);
		tpage_tex1 = loadToVRAM(buffer_tex1);
		free3(buffer_nr1);
		free3(buffer_hero);
		free3(buffer_explosion);
		free3(buffer_bg1);
		free3(buffer_tex1);
	}
	sprite_init(&shooter_background, SCREEN_WIDTH, SCREEN_HEIGHT, tpage_bg1);
	sprite_set_uv(&shooter_background, 0, 0, 256, 256);
	actor_init(&hero, 28, 64, tpage_hero, 0, HERO_IDLE_ROW);
	sprite_set_uv(&hero.body, 0, HERO_IDLE_ROW, HERO_FRAME_W, NR_FRAME_H);
	hero.hp = 5;
	hero.body.pos.vx = 100;
	hero.body.pos.vy = 20 - HERO_GROUND_OFFSET;
	hero.body.pos.vz = 0;

	for(i = 0; i < SHOOTER_ENEMIES; i++) {
		actor_init(&enemies[i], 24, 24, tpage_nr1,
			NR_ENEMY_IDLE * NR_FRAME_W, NR_ENEMY_V);
		enemies[i].body.pos.vx = 720 + i * 720;
		enemies[i].body.pos.vy = ENEMY_BASE_Y;
		enemies[i].body.pos.vz = 0;
		enemies[i].hp = 2;
		actor_init(&explosions[i], 48, 48, tpage_explosion, 0, 0);
		explosions[i].active = 0;
	}
	for(i = 0; i < SHOOTER_SHOTS; i++) {
		actor_init(&shots[i], 12, 6, tpage_nr1,
			NR_PROJECTILE_FRAME * NR_FRAME_W, NR_EFFECT_V);
		shots[i].active = 0;
	}
	sprite_init(&muzzle_flash, 7, 5, tpage_nr1);
	sprite_set_uv(&muzzle_flash, 16, NR_EFFECT_V + 24, 32, 20);
	sprite_init(&burst, 70, 70, tpage_nr1);
	sprite_set_uv(&burst, NR_BURST_FRAME * NR_FRAME_W,
		NR_EFFECT_V, NR_FRAME_W, NR_FRAME_H);

	platform_init(&platforms[0], -500, 20, 170, 1800, 340, 260);
	platform_init(&platforms[1], 1400, 20, 170, 900, 340, 260);
	platform_init(&platforms[2], 2450, 20, 170, 1000, 340, 260);
	platform_init(&platforms[3], 3600, 20, 170, 2100, 340, 260);
	platform_init(&platforms[4], 850, -180, 110, 360, 220, 50);
	platform_init(&platforms[5], 2050, -135, 80, 300, 180, 45);
	platform_init(&platforms[6], 3260, -180, 130, 420, 240, 50);

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
	aim_up = 0;
	shot_cooldown = 0;
	hero_frame = 0;
	hero_row = HERO_IDLE_ROW;
	game_over = 0;
	game_over_start = 0;
	shooter_loaded = 1;
}

void shooter_update(void) {
	int i, j;
	long floor_y;
	long previous_y = hero.body.pos.vy;
	long target_camera_y;
	explosions_update();
	if(game_over) {
		/* Use vertical blanks, so the delay survives changes in frame rate. */
		if(VSync(-1) - game_over_start >= GAME_OVER_TICKS)
			shooter_load();
		return;
	}
	if(hero.hp <= 0 || hero.body.pos.vy > 500) {
		game_over_begin();
		return;
	}

	hero.vx = 0;
	if(pad & PADLleft) {
		hero.vx = -HERO_RUN_SPEED;
		hero_direction = -1;
		hero.body.mirror_h = 1;
	}
	if(pad & PADLright) {
		hero.vx = HERO_RUN_SPEED;
		hero_direction = 1;
		hero.body.mirror_h = 0;
	}
	if((pad & PADLsquare) && !(opad & PADLsquare) && grounded) {
		hero.vy = -HERO_JUMP_SPEED;
		grounded = 0;
	}
	aim_up = 0;
	if(pad & PADLup) {
		aim_up = 2;
		if(hero.vx != 0) aim_up = 1;
	}
	if(shot_cooldown > 0) shot_cooldown--;
	if((pad & PADLcircle) && !(opad & PADLcircle) && special_timer == 0) special_timer = 75;

	hero.body.pos.vx += hero.vx;
	if(hero.body.pos.vx < 0) hero.body.pos.vx = 0;
	if(hero.body.pos.vx > WORLD_END) hero.body.pos.vx = WORLD_END;
	hero.vy += 1;
	if(hero.vy > 20) hero.vy = 20;
	hero.body.pos.vy += hero.vy;
	floor_y = ground_at(hero.body.pos.vx, previous_y, hero.body.pos.vy);
	if(floor_y != 30000) {
		hero.body.pos.vy = floor_y;
		hero.vy = 0;
		grounded = 1;
	} else grounded = 0;
	if(hero.body.pos.vy > 500) {
		game_over_begin();
		return;
	}
	if(hero_fire_timer > 0) hero_fire_timer--;
	/* Fire after movement so the muzzle stays attached to the current pose. */
	if((pad & PADLcross) && shot_cooldown == 0) {
		fire_shot();
		hero_fire_timer = SHOT_INTERVAL;
		shot_cooldown = SHOT_INTERVAL;
	}
	/* Eight consecutive poses keep their phase while firing or changing aim. */
	hero_frame = aim_up;
	hero_row = HERO_IDLE_ROW;
	if(grounded && hero.vx != 0) {
		hero_frame = run_anim_timer / RUN_FRAME_TICKS;
		hero_row = aim_up * NR_FRAME_H;
		run_anim_timer = (run_anim_timer + 1) % (RUN_FRAME_TICKS * RUN_FRAMES);
	} else {
		run_anim_timer = 0;
		if(!grounded) hero_frame = 3 + aim_up;
	}
	sprite_set_uv(&hero.body, hero_frame * HERO_FRAME_W,
		hero_row, HERO_FRAME_W, NR_FRAME_H);

	for(i = 0; i < SHOOTER_SHOTS; i++) if(shots[i].active) {
		for(j = 0; j < SHOOTER_ENEMIES; j++) {
			if(enemies[j].active && shot_hits(&shots[i], &enemies[j])) {
				shots[i].active = 0;
				enemies[j].timer = 6;
				if(--enemies[j].hp <= 0) enemy_destroy(j);
				break;
			}
		}
		shots[i].body.pos.vx += shots[i].vx;
		shots[i].body.pos.vy += shots[i].vy;
		if(--shots[i].timer <= 0) shots[i].active = 0;
	}

	for(i = 0; i < SHOOTER_ENEMIES; i++) if(enemies[i].active) {
		if(enemies[i].timer > 0) enemies[i].timer--;
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
		/* Distant enemies wait instead of collecting around the player. */
		if(labs(enemies[i].body.pos.vx - hero.body.pos.vx) < ENEMY_WAKE_DISTANCE &&
			enemies[i].timer == 0) {
			if(enemies[i].body.pos.vx > hero.body.pos.vx) enemies[i].body.pos.vx--;
			else if(enemies[i].body.pos.vx < hero.body.pos.vx) enemies[i].body.pos.vx++;
		}
		if(labs(enemies[i].body.pos.vx - hero.body.pos.vx) < 45 &&
			labs(enemies[i].body.pos.vy - hero.body.pos.vy) < 65 && spawn_timer == 0) {
			hero.hp--;
			spawn_timer = 75;
		}
		if(special_timer > 44 && labs(enemies[i].body.pos.vx - hero.body.pos.vx) < 340) {
			enemy_destroy(i);
		}
	}
	if(hero.hp <= 0) {
		game_over_begin();
		return;
	}
	if(spawn_timer > 0) spawn_timer--;
	if(special_timer > 0) special_timer--;

	camera.pos.vx += ((-hero.body.pos.vx + 120) - camera.pos.vx) / 8;
	if(camera.pos.vx > 0) camera.pos.vx = 0;
	target_camera_y = CAMERA_BASE_Y -
		(hero.body.pos.vy + HERO_GROUND_OFFSET);
	camera.pos.vy += (target_camera_y - camera.pos.vy) / 8;
	if(special_timer > 0) {
		camera.rot.vy = special_timer - 38;
		camera.rot.vz = ((special_timer & 7) - 4) / 2;
		camera.pos.vz = 1020;
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
	char hud[64];
	if(game_over) {
		/* Same BIOS font and empty scene as the main menu. */
		FntPrint("\n\n\n\n\n\n\n\n\n\n\n\n");
		FntPrint("               GAME OVER\n\n");
		FntPrint("             RESTARTING...\n");
		return;
	}
	drawSprite(&shooter_background, OTSIZE - 1);
	/* Keep real 3D depth for clipping; actors use fixed foreground layers. */
	for(i = 0; i < SHOOTER_PLATFORMS; i++) {
		for(j = 0; j < platforms[i].tile_count; j++) {
			Mesh *tile = &platforms[i].mesh[j];
			if(tile->pos.vx + PLATFORM_TILE_WIDTH >= hero.body.pos.vx - 800 &&
				tile->pos.vx <= hero.body.pos.vx + 900)
				drawMesh(tile, 0);
		}
	}
	if(spawn_timer == 0 || (spawn_timer & 4) == 0)
		actor_draw(&hero);
	for(i = 0; i < SHOOTER_ENEMIES; i++) {
		if(enemies[i].timer > 0 && (enemies[i].timer & 1)) continue;
		actor_draw(&enemies[i]);
	}
	for(i = 0; i < SHOOTER_ENEMIES; i++) {
		if(explosions[i].active)
			drawSprite3D(&explosions[i].body, SHOOTER_OT_SHOT);
	}
	for(i = 0; i < SHOOTER_SHOTS; i++) {
		if(shots[i].active)
			drawSprite3D(&shots[i].body, SHOOTER_OT_SHOT);
	}

	if(!game_over && hero_fire_timer > SHOT_INTERVAL - 3) {
		muzzle_position(&muzzle_flash.pos);
		muzzle_flash.mirror_h = hero_direction < 0;
		muzzle_flash.rot.vz = -aim_up * hero_direction * 512;
		sprite_set_uv(&muzzle_flash, 16, NR_EFFECT_V + 24, 32, 20);
		drawSprite3D(&muzzle_flash, SHOOTER_OT_SHOT);
	}
	if(special_timer > 0) {
		burst.pos = hero.body.pos;
		burst.w = 55 + (special_timer & 15) * 3;
		burst.h = burst.w;
		drawSprite3D(&burst, SHOOTER_OT_SPECIAL);
	}
	/* The RPG font atlas at (960, 0) overlaps FX1; use the BIOS font. */
	sprintf(hud, "HP %d   SCORE %d", hero.hp, score);
	FntPrint("%s\n", hud);
	for(i = 1; i <= 29; i++) {
		if(i == 9 && special_timer > 0)
			FntPrint("             PHOTON BURST!");
		if(i == 13 && hero.body.pos.vx >= WORLD_END)
			FntPrint("           MISSION COMPLETE");
		if(i == 27)
			FntPrint("HOLD X FIRE  UP / UP+SIDE AIM");
		if(i == 29)
			FntPrint("SQUARE JUMP  CIRCLE SPECIAL");
		FntPrint("\n");
	}
}
