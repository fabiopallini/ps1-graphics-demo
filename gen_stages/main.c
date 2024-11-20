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
	stageData->planes_len = 0;
	stageData->spawns_len = 0;
	stageData->zones_len = 0;
}

void set_plane(StageData *stageData, int x, int y, int z, int w, int h, int d){
	PlaneData *p = &stageData->planes[stageData->planes_len++];
	p->x = x; p->y = y; p->z = z;
	p->w = w; p->h = h; p->d = d;
}

void set_spawn(StageData *stageData, int x, int y, int z, short rx, short ry, short rz) {
	SpawnData *s = &stageData->spawns[stageData->spawns_len++];
	s->x = x; s->y = y; s->z = z;
	s->rx = rx; s->ry = ry; s->rz = rz;
}

void set_zone(StageData *stageData, int x, int y, int z, int w, int h, int d, int stage_id, int spawn_id) {
	ZoneData *zone = &stageData->zones[stageData->zones_len++];
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
		fwrite(&stageData[i], sizeof(StageData), 1, file);
		for(int j = 0; j < stageData[i].npc.talk_pages; j++){
			//printf("talk_chars[%d]: %s\n", j, stageData[i].npc.talk_chars[j]);
			fwrite(stageData[i].npc.talk_chars[j], sizeof(char), BALLOON_MAX_CHARS, file);
		}
        }
	fclose(file);

	// TEST READ
	file = fopen(FILE_NAME, "rb");
	if (file == NULL) {
		perror("error: can't read file");
		exit(EXIT_FAILURE);
	}
	for(int i = 0; i < array_size; i++){
		StageData data;
		fread(&data, sizeof(StageData), 1, file);
		printf("tim_0: %s\n", data.tims[0]);
		//fseek(file, data.npc.talk_pages * BALLOON_MAX_CHARS, SEEK_CUR); 
		data.npc.talk_chars = malloc(data.npc.talk_pages * sizeof(char *));
		for (int j = 0; j < data.npc.talk_pages; j++) {
			data.npc.talk_chars[j] = malloc(BALLOON_MAX_CHARS * sizeof(char));
			fread(data.npc.talk_chars[j], sizeof(char), BALLOON_MAX_CHARS, file);
			printf("npc talk_chars[%d]: %s\n", j, data.npc.talk_chars[j]);
		}
		for (int j = 0; j < data.npc.talk_pages; j++)
			free(data.npc.talk_chars[j]);
		free(data.npc.talk_chars);
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
		printf("Errore nel parsing del JSON\n");
		free(json_data);
		return;
	}

	if (!cJSON_IsArray(json_array)) {
		printf("Il JSON letto non è un array\n");
		cJSON_Delete(json_array);
		free(json_data);
		return;
	}

	array_size = cJSON_GetArraySize(json_array);
	int index = 0;
	int byte_address = 0;
	int stages_byte_addr[array_size];
	printf("Dimensione dell'array: %d\n", array_size);

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

		/*cJSON *npc = cJSON_GetObjectItemCaseSensitive(jStage, "npc");
		if(cJSON_IsArray(npc)) {
			int npc_size = cJSON_GetArraySize(npc);
			cJSON *talk_chars = NULL;
			int i = 0;
			cJSON_ArrayForEach(talk_chars, npc){
				char *line = cJSON_GetStringValue(talk_chars);
				if(line != NULL){
					s->npc.talk_pages = npc_size;
					memcpy(&s->npc.talk_chars[i++], line, strlen(line));
				}
			}
		}*/

		cJSON *npcs = cJSON_GetObjectItemCaseSensitive(jStage, "npcs");
		if(cJSON_IsArray(npcs)) {
			cJSON *npc_obj = NULL;
			cJSON_ArrayForEach(npc_obj, npcs)
			{
				cJSON *npc = cJSON_GetObjectItemCaseSensitive(npc_obj, "npc");
				int npc_size = cJSON_GetArraySize(npc);
				s->npc.talk_chars = (char **) malloc(npc_size * sizeof(char *));
				cJSON *talk_page = NULL;
				int i = 0;
				cJSON_ArrayForEach(talk_page, npc){
					char *page = cJSON_GetStringValue(talk_page);
					if(page!= NULL){
						s->npc.talk_pages = npc_size;
						s->npc.talk_chars[i] = (char *) malloc(BALLOON_MAX_CHARS * sizeof(char));
						strncpy(s->npc.talk_chars[i], page, strlen(page));
						//strncpy(s->npc.talk_chars[i], page, BALLOON_MAX_CHARS);
						//printf("talk chars %s\n", s->npc.talk_chars[i]);
						i++;
					}
				}
			}
		}
		printf("stageData %d size %d\n", index, sizeof(StageData) + (s->npc.talk_pages*BALLOON_MAX_CHARS));
		byte_address += sizeof(StageData) + (s->npc.talk_pages*BALLOON_MAX_CHARS);
		stages_byte_addr[index] = byte_address;
		printf("stage byte address %d\n", stages_byte_addr[index]);
		index++;
	}

	write_stages_h(stages_byte_addr, array_size);

	cJSON_Delete(json_array);
	free(json_data);
	write_stages_bin(stageData, array_size);
	for(int i = 0; i < array_size; i++){
		for(int n = 0; n < stageData[i].npc.talk_pages; n++)
			free(stageData[i].npc.talk_chars[n]);
		free(stageData[i].npc.talk_chars);
	}
	free(stageData);
	printf("sizeof(StageData) %d\n", sizeof(StageData));
}

