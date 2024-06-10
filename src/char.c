#include "char.h"

void char_animation_init()
{

}

void char_animation_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz))
{
	if(c->meshNode != NULL)
	{
		int i = 0;
		Node *node = c->meshNode;
		c->animation_timer++;
		if(c->animation_timer >= 5){
			c->animation_timer = 0;
			c->animation_frame++;
			if(c->animation_frame >= 2)
				c->animation_frame = 0;
		}
		while(node != NULL){
			if(i == c->animation_frame)
				drawMesh((Mesh*)node->data, NULL);
			node = node->next;
			i++;
		}
	}
}
