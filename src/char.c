#include "char.h"
#include "utils.h"

void char_animation_set(Mesh anim[], MeshAnimation *meshAnim, Character *c, u_long *cd_data[], u_short tpages[])
{
	mesh_init(&anim[0], cd_data[11], tpages[2], 255, 150);
	mesh_init(&anim[1], cd_data[12], tpages[2], 255, 150);
	mesh_init(&anim[2], cd_data[13], tpages[2], 255, 150);

	node_push(&meshAnim->meshNode, &anim[0]);
	node_push(&meshAnim->meshNode, &anim[1]);
	node_push(&meshAnim->meshNode, &anim[2]);

	meshAnim->frames = 3;
	node_push(&c->animationNode, meshAnim);
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
