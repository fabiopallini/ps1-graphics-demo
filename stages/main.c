#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include <data.h>

#define FILE_NAME "STAGES.BIN"
#define BALLOON_MAX_CHARS 78

int array_size;
void parse_json();

void init_stage_data(StageData *stageData, char *tim0, char *tim1, 
	int cam_x, int cam_y, int cam_z, short cam_rx, 
	short cam_ry, short cam_rz)
{
	strcpy(stageData->tims[0], tim0);
	strcpy(stageData->tims[1], tim1);
	stageData->cam_x = cam_x;		stageData->cam_y = cam_y;
	stageData->cam_z = cam_z;		stageData->cam_rx = cam_rx;
	stageData->cam_ry = cam_ry;		stageData->cam_rz = cam_rz;
	stageData->planesData_len = 0;
	stageData->spawnsData_len = 0;
	stageData->zonesData_len = 0;
	stageData->npcsData_len = 0;
}

void set_plane(StageData *stageData, int x, int y, int z, int w, int h, int d){
	PlaneData *p = &stageData->planesData[stageData->planesData_len++];
	p->x = x; p->y = y; p->z = z;
	p->w = w; p->h = h; p->d = d;
}

void set_spawn(StageData *stageData, int x, int y, int z, short rx, short ry, short rz) {
	SpawnData *s = &stageData->spawnsData[stageData->spawnsData_len++];
	s->x = x; s->y = y; s->z = z;
	s->rx = rx; s->ry = ry; s->rz = rz;
}

void set_zone(StageData *stageData, int x, int y, int z, int w, int h, int d, int stage_id, int spawn_id) {
	ZoneData *zone = &stageData->zonesData[stageData->zonesData_len++];
	zone->x = x; zone->y = y; zone->z = z;
	zone->w = w; zone->h = h; zone->d = d;
	zone->stage_id = stage_id; zone->spawn_id = spawn_id;
}

void write_stages_bin(StageData *stageData, int array_size){
	FILE *file = fopen(FILE_NAME, "wb");
	if (file == NULL) {
		perror("error: can't write file");
		exit(EXIT_FAILURE);
	}
	for(int i = 0; i < array_size; i++){
		StageData *sd = &stageData[i];
		fwrite(sd, sizeof(StageData), 1, file);
		for(int j = 0; j < sd->npcsData_len; j++)
		{
			for(int n = 0; n < sd->npcData[j].talk_pages; n++){
				printf("write talk_chars[%d]: %s len %d\n", j, 
					sd->npcData[j].talk_chars[n], strlen(sd->npcData[j].talk_chars[n]));
				char *str = sd->npcData[j].talk_chars[n];
				fwrite(str, sizeof(char), strlen(str)+1, file);
			}
		}
        }
	fclose(file);

	// TEST READ
	printf("TEST READ\n");
	file = fopen(FILE_NAME, "rb");
	if (file == NULL) {
		perror("error: can't read file");
		exit(EXIT_FAILURE);
	}
	for(int i = 0; i < array_size; i++){
		StageData sd;
		fread(&sd, sizeof(StageData), 1, file);
		printf("tims: %s %s\n", sd.tims[0], sd.tims[1]);

		for(int j = 0; j < sd.npcsData_len; j++){
			for(int n = 0; n < sd.npcData[j].talk_pages; n++){
				char str[BALLOON_MAX_CHARS];
				memset(str, 0, BALLOON_MAX_CHARS);
				int len = 0;
				char ch;
				while(fread(&ch, sizeof(char), 1, file) == 1){
					if (ch == '\0')
						break;
					if (len < BALLOON_MAX_CHARS - 1)
						str[len++] = ch;
				}
				str[len] = '\0';
				printf("npc talk_chars %s\n", str);
			}
		}
	}
	fclose(file);
}

void write_stages_h(int *bytes_addr, int size) {
	FILE *file = fopen("stages.h", "w");
	if (file == NULL) {
		perror("Errore nell'aprire il file");
		exit(EXIT_FAILURE);
	}
	fprintf(file, "unsigned int stages_byte_addr[] = {");
	for (int i = 0; i < size; i++) {
		fprintf(file, "%d", bytes_addr[i]);
		if (i < size - 1) {
			fprintf(file, ", ");
		}
	}
	fprintf(file, "};\n");
	fclose(file);
}

int main() {
	parse_json();
	//printf("sizeof short: %d\n", sizeof(short));
	//printf("sizeof int: %d\n", sizeof(int));
	return 0;
}

char *read_json_file(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		perror("Impossibile aprire il file");
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *content = malloc(length + 1);
	if (content) {
		fread(content, 1, length, file);
	}
	content[length] = '\0';

	fclose(file);
	return content;
}

void parse_json() {
	char *json_data = read_json_file("stages.json");
	if (json_data == NULL) {
		return;
	}

	cJSON *json_array = cJSON_Parse(json_data);
	if (json_array == NULL) {
		printf("JSON parse error\n");
		free(json_data);
		return;
	}

	if (!cJSON_IsArray(json_array)) {
		printf("JSON is not an array\n");
		cJSON_Delete(json_array);
		free(json_data);
		return;
	}

	array_size = cJSON_GetArraySize(json_array);
	int index = 0;
	int byte_address = 0;
	int stages_byte_addr[array_size];
	int pageLen = 0;

	StageData *stageData = malloc(array_size * sizeof(StageData));
	StageData *s;
	if (stageData == NULL) {
		printf("StageData malloc error\n");
		exit(1);
	}
	
	cJSON *jStage = NULL;

	cJSON_ArrayForEach(jStage, json_array) {
		s = &stageData[index];
		cJSON *tim_0 = cJSON_GetObjectItemCaseSensitive(jStage, "tim_0");
		cJSON *tim_1 = cJSON_GetObjectItemCaseSensitive(jStage, "tim_1");
		cJSON *cam_x = cJSON_GetObjectItemCaseSensitive(jStage, "cam_x");
		cJSON *cam_y = cJSON_GetObjectItemCaseSensitive(jStage, "cam_y");
		cJSON *cam_z = cJSON_GetObjectItemCaseSensitive(jStage, "cam_z");
		cJSON *cam_rx = cJSON_GetObjectItemCaseSensitive(jStage, "cam_rx");
		cJSON *cam_ry = cJSON_GetObjectItemCaseSensitive(jStage, "cam_ry");
		cJSON *cam_rz = cJSON_GetObjectItemCaseSensitive(jStage, "cam_rz");

		init_stage_data(s, tim_0->valuestring, tim_1->valuestring,
				cam_x->valueint, cam_y->valueint, cam_z->valueint,
				(short)cam_rx->valueint, (short)cam_ry->valueint, (short)cam_rz->valueint);

		cJSON *planes = cJSON_GetObjectItemCaseSensitive(jStage, "planes");
		if (cJSON_IsArray(planes)) {
			cJSON *plane = NULL;
			cJSON_ArrayForEach(plane, planes) 
			{
				cJSON *x = cJSON_GetObjectItemCaseSensitive(plane, "x");
				cJSON *y = cJSON_GetObjectItemCaseSensitive(plane, "y");
				cJSON *z = cJSON_GetObjectItemCaseSensitive(plane, "z");
				cJSON *w = cJSON_GetObjectItemCaseSensitive(plane, "w");
				cJSON *h = cJSON_GetObjectItemCaseSensitive(plane, "h");
				cJSON *d = cJSON_GetObjectItemCaseSensitive(plane, "d");
				set_plane(s, x->valueint,y->valueint,z->valueint,
					w->valueint,h->valueint,d->valueint);
			}
		}

		cJSON *zones = cJSON_GetObjectItemCaseSensitive(jStage, "zones");
		if (cJSON_IsArray(zones)) {
			cJSON *zone = NULL;
			cJSON_ArrayForEach(zone, zones) 
			{
				cJSON *x = cJSON_GetObjectItemCaseSensitive(zone, "x");
				cJSON *y = cJSON_GetObjectItemCaseSensitive(zone, "y");
				cJSON *z = cJSON_GetObjectItemCaseSensitive(zone, "z");
				cJSON *w = cJSON_GetObjectItemCaseSensitive(zone, "w");
				cJSON *h = cJSON_GetObjectItemCaseSensitive(zone, "h");
				cJSON *d = cJSON_GetObjectItemCaseSensitive(zone, "d");
				cJSON *stage_id = cJSON_GetObjectItemCaseSensitive(zone, "stage_id");
				cJSON *spawn_id = cJSON_GetObjectItemCaseSensitive(zone, "spawn_id");
				set_zone(s, x->valueint, y->valueint, z->valueint,
					w->valueint, h->valueint, d->valueint,
					stage_id->valueint, spawn_id->valueint);
			}
		}
		cJSON *spawns = cJSON_GetObjectItemCaseSensitive(jStage, "spawns");
		if(cJSON_IsArray(spawns)) {
			cJSON *spawn = NULL;
			cJSON_ArrayForEach(spawn, spawns) 
			{
				cJSON *x = cJSON_GetObjectItemCaseSensitive(spawn, "x");
				cJSON *y = cJSON_GetObjectItemCaseSensitive(spawn, "y");
				cJSON *z = cJSON_GetObjectItemCaseSensitive(spawn, "z");
				cJSON *rx = cJSON_GetObjectItemCaseSensitive(spawn, "rx");
				cJSON *ry = cJSON_GetObjectItemCaseSensitive(spawn, "ry");
				cJSON *rz = cJSON_GetObjectItemCaseSensitive(spawn, "rz");
				set_spawn(s, x->valueint, y->valueint, z->valueint,
					(short)rx->valueint, (short)ry->valueint, (short)rz->valueint);
			}
		}

		cJSON *npcs = cJSON_GetObjectItemCaseSensitive(jStage, "npcs");
		if(cJSON_IsArray(npcs)){
			int j = 0;
			cJSON *npc_obj = NULL;
			cJSON_ArrayForEach(npc_obj, npcs)
			{
				cJSON *npc = npc_obj->child; 
				while(npc)
				{
					// get the key name (es. "npc0", "npc1")
					const char *npc_name = npc->string;
					printf("npc name %s\n", npc_name);
					cJSON *x = cJSON_GetObjectItemCaseSensitive(npc, "x");
					cJSON *y = cJSON_GetObjectItemCaseSensitive(npc, "y");
					cJSON *z = cJSON_GetObjectItemCaseSensitive(npc, "z");
					cJSON *rx = cJSON_GetObjectItemCaseSensitive(npc, "rx");
					cJSON *ry = cJSON_GetObjectItemCaseSensitive(npc, "ry");
					cJSON *rz = cJSON_GetObjectItemCaseSensitive(npc, "rz");
					s->npcData[j].x = x->valueint;
					s->npcData[j].y = y->valueint;
					s->npcData[j].z = z->valueint;
					s->npcData[j].rx = rx->valueint;
					s->npcData[j].ry = ry->valueint;
					s->npcData[j].rz = rz->valueint;
					cJSON *talk_chars = cJSON_GetObjectItemCaseSensitive(npc, "talk_chars");
					if(talk_chars == NULL){
						printf("can't find talk_chars\n");
						return;
					}
					int talk_pages = cJSON_GetArraySize(talk_chars);
					int i = 0;
					s->npcData[j].talk_chars = (char **) malloc(talk_pages * sizeof(char *));
					cJSON *page_chars = NULL;
					cJSON_ArrayForEach(page_chars, talk_chars){
						char *page = cJSON_GetStringValue(page_chars);
						if(page!= NULL){
							size_t len = strlen(page);
							pageLen += len+1;
							s->npcData[j].talk_pages = talk_pages;
							
							s->npcData[j].talk_chars[i] = (char *) malloc((len+1) * sizeof(char));
							strncpy(s->npcData[j].talk_chars[i], page, len);
							s->npcData[j].talk_chars[i][len] = '\0';
							printf("talk chars %s\n", s->npcData[j].talk_chars[i]);
							i++;
						}
					}
					j++;
					s->npcsData_len++;
					npc = npc->next;
				}
			}
		}
		byte_address += sizeof(StageData) + pageLen;
		pageLen = 0;
		stages_byte_addr[index] = byte_address;
		//printf("stage byte address %d\n", stages_byte_addr[index]);
		index++;
	}

	write_stages_h(stages_byte_addr, array_size);

	cJSON_Delete(json_array);
	free(json_data);
	write_stages_bin(stageData, array_size);
	for(int i = 0; i < array_size; i++){
		for(int j = 0; j < stageData[i].npcsData_len; j++)
		{
			StageData *sd = &stageData[i];
			for(int n = 0; n < sd->npcData[j].talk_pages; n++)
				free(sd->npcData[j].talk_chars[n]);
			free(sd->npcData[j].talk_chars);
		}
	}
	free(stageData);
}

