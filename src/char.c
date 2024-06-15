#include "char.h"
#include "utils.h"
#include <string.h> 

void char_animation_init(Character *c, u_short n_animations, u_long *cd_data[], u_short tpages[])
{
	c->meshAnimations = malloc3(sizeof(MeshAnimation)  * n_animations);
	if (c->meshAnimations == NULL) {
		printf("Error on c->meshAnimation malloc3\n");
		return;
	}
}

void char_animation_set(Character *c, u_short animation_index, u_short frames, u_long *cd_data[], u_short tpages[])
{
	u_short i = 0;
	u_short n = animation_index;
	c->meshAnimations[n].frames = frames;

	c->meshAnimations[n].meshFrames = malloc3(frames * sizeof(Mesh));
	if (c->meshAnimations[n].meshFrames == NULL) {
		printf("Error on c->meshAnimation[n].meshFrames malloc3\n");
		return;
	}
	for(i = 0; i < frames; i++){
		memset(&c->meshAnimations[n].meshFrames[i], 0, sizeof(Mesh));
		mesh_init(&c->meshAnimations[n].meshFrames[i], cd_data[11+i], tpages[2], 255, 150);
	}
}

void char_animation_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz))
{
	u_short animation_to_play = 0;
	//MeshAnimation animation = c->meshAnimations[animation_to_play];

	c->meshAnimations[animation_to_play].timer++;
	if(c->meshAnimations[animation_to_play].timer >= 5){
		c->meshAnimations[animation_to_play].timer = 0;
		c->meshAnimations[animation_to_play].current_frame++;
		if(c->meshAnimations[animation_to_play].current_frame >= c->meshAnimations[animation_to_play].frames)
			c->meshAnimations[animation_to_play].current_frame = 0;
	}

	drawMesh(&c->meshAnimations[animation_to_play].meshFrames[c->meshAnimations[animation_to_play].current_frame], _otz);
}
