#include "char.h"

void char_animation_init()
{

}

void char_animation_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz))
{
	if(c->animationNode != NULL)
	{
		Node *animationNode = c->animationNode;
		while(animationNode != NULL)
		{
			MeshAnimation *meshAnimation = (MeshAnimation*)animationNode->data;
			int i = 0;
			Node *node = meshAnimation->meshNode;

			meshAnimation->timer++;
			if(meshAnimation->timer >= 5){
				meshAnimation->timer = 0;
				meshAnimation->current_frame++;
				if(meshAnimation->current_frame >= meshAnimation->frames)
					meshAnimation->current_frame = 0;
			}

			while(node != NULL){
				if(i == meshAnimation->current_frame)
					drawMesh((Mesh*)node->data, _otz);
				node = node->next;
				i++;
			}
			
			animationNode = animationNode->next;
		}
	}
}
