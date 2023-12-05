#include "sprite.h"
#include "enemy.h"

int ray_collisions(Sprite *s, Enemy enemies[], long cameraX);
int ray_collision(Sprite *s1, Sprite *s2, long cameraX);
int sprite_collision(Sprite *s1, Sprite *s2);
int inCameraView(Sprite s, long cameraX);
