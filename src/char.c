#include "char.h"
#include "utils.h"

void char_animation_init(Character *c, unsigned int frames, u_long *cd_data[], u_short tpages[])
{
	c->meshAnimation = malloc3(sizeof(MeshAnimation));
	if (c->meshAnimation == NULL) {
		printf("Error on c->meshAnimation malloc3\n");
		return;
	}
	c->meshAnimation->meshNode = NULL; 
	c->meshAnimation->frames = 3;

	c->meshAnimation->meshFrames = malloc3(2 * sizeof(Mesh));
	if (c->meshAnimation->meshFrames == NULL) {
		printf("Error on c->meshAnimation->meshFrames malloc3\n");
		return;
	}
	memset(&c->meshAnimation->meshFrames[0], 0, sizeof(Mesh));
	mesh_init(&c->meshAnimation->meshFrames[0], cd_data[11], tpages[2], 255, 150);

	/*c->meshAnimation->meshFrame = malloc3(sizeof(Mesh));
	if (c->meshAnimation->meshFrame == NULL) {
		printf("Error on c->meshAnimation->meshFrame malloc3\n");
		return;
	}
	memset(c->meshAnimation->meshFrame, 0, sizeof(Mesh));
	mesh_init(c->meshAnimation->meshFrame, cd_data[11], tpages[2], 255, 150);*/

	/*memset(&c->meshAnimation->test, 0, sizeof(Mesh));
	mesh_init(&c->meshAnimation->test, cd_data[11], tpages[2], 255, 150);*/
}

void char_animation_set(Mesh meshFrames[], MeshAnimation *meshAnim, Character *c, u_long *cd_data[], u_short tpages[])
{
	c->meshAnimation = malloc3(sizeof(MeshAnimation));
	if (c->meshAnimation == NULL) {
		printf("Error on c->meshAnimation malloc3\n");
		return;
	}
	c->meshAnimation->meshNode = NULL; 
	c->meshAnimation->frames = 3;

	mesh_init(&meshFrames[0], cd_data[11], tpages[2], 255, 150);
	mesh_init(&meshFrames[1], cd_data[12], tpages[2], 255, 150);
	mesh_init(&meshFrames[2], cd_data[13], tpages[2], 255, 150);

	node_push(&c->meshAnimation->meshNode, &meshFrames[0]);
	node_push(&c->meshAnimation->meshNode, &meshFrames[1]);
	node_push(&c->meshAnimation->meshNode, &meshFrames[2]);

	node_push(&c->animationNode, c->meshAnimation);
}

void char_animation_set2(Mesh meshFrames[], MeshAnimation *meshAnim, Character *c, u_long *cd_data[], u_short tpages[])
{
	//c->meshAnimationations[0] = malloc3(sizeof(MeshAnimation));
	mesh_init(&meshFrames[0], cd_data[11], tpages[2], 255, 150);
	mesh_init(&meshFrames[1], cd_data[12], tpages[2], 255, 150);
	mesh_init(&meshFrames[2], cd_data[13], tpages[2], 255, 150);

	node_push(&meshAnim->meshNode, &meshFrames[0]);
	node_push(&meshAnim->meshNode, &meshFrames[1]);
	node_push(&meshAnim->meshNode, &meshFrames[2]);

	meshAnim->frames = 3;
	node_push(&c->animationNode, meshAnim);
}

void char_animation_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz))
{
	/*int i = 0;
	Node *node = c->meshAnimation->meshNode;

	c->meshAnimation->timer++;
	if(c->meshAnimation->timer >= 5){
		c->meshAnimation->timer = 0;
		c->meshAnimation->current_frame++;
		if(c->meshAnimation->current_frame >= c->meshAnimation->frames)
			c->meshAnimation->current_frame = 0;
	}

	while(node != NULL){
		if(i == c->meshAnimation->current_frame)
			drawMesh((Mesh*)node->data, _otz);
		node = node->next;
		i++;
	}*/


	/*if(c->animationNode != NULL)
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
	}*/
}
