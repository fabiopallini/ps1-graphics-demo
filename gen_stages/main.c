#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include <data.h>

#define FILE_NAME "STAGES.BIN"
#define N_STAGES 5 

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

void write_struct(const char *filename, StageData *stage, int append) {
	FILE *file;
	if(append == 0)
		file = fopen(filename, "wb");
	else
		file = fopen(filename, "ab");
	if (file == NULL) {
		perror("error: can't open file");
		exit(EXIT_FAILURE);
	}
	/*fwrite(&data->id, sizeof(int), 1, file);
	fwrite(&data->value, sizeof(int), 1, file);*/
	fwrite(stage, sizeof(StageData), 1, file);
	fclose(file);
}

void read_struct(const char *filename, StageData *stage) {
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		perror("error: can't read file");
		exit(EXIT_FAILURE);
	}
	/*fread(&data->id, sizeof(int), 1, file);
	fread(&data->value, sizeof(int), 1, file);*/
	fread(stage, sizeof(StageData), 1, file);
	fclose(file);
}

StageData *gen_stages()
{
	StageData *stage = malloc(N_STAGES * sizeof(StageData));
	StageData *s;
	if (stage == NULL) {
		printf("StageData malloc error\n");
		exit(1);
	}
	
	// hallway
	init_stage_data(&stage[0], "BK0_0.TIM", "BK0_1.TIM",
		-55, 294, 926,
		185, 5, 0
	);
	set_plane(&stage[0],
		50, 0, 0,
		10, 0, -600 
	);
	set_plane(&stage[0],
		-50, 0, -410,
		100, 0, -100
	);

	set_zone(&stage[0],
		-50, 0, -410,
		10, 0, -100,
		2, 0
	);
	set_zone(&stage[0],
		40, 0, -590,
		45, 0, -30,
		3, 0
	);

	set_spawn(&stage[0],
		55, 0, 0,
		0, 0, 0 
	);
	set_spawn(&stage[0],
		20, 0, -430,
		0, 1024*3, 0 
	);
	set_spawn(&stage[0],
		55, 0, -500,
		0, 2048, 0 
	);

	// bedroom
	init_stage_data(&stage[1], "BK1_0.TIM", "BK1_1.TIM",
		-461, 942, 2503,
		160, 195, 0
	);
	set_plane(&stage[1],
		0, 0, 0,
		230, 0, -1300
	);
	set_spawn(&stage[1],
		80, 0, -1000,
		0, 2048, 0
	);

	// bathroom 
	s = &stage[2];
	init_stage_data(s, "BK2_0.TIM", "BK2_1.TIM",
		-10, 286, 1372,
		159, -32, 0
	);

	set_plane(s, 20, 0, -800,
		20, 0, -300
	);
	set_spawn(s, 35, 0, -1000,
		0, 2048, 0
	);
	set_zone(s, 20, 0, -1090,
		20, 0, -20,
		0, 1 
	);

	// forest 
	s = &stage[3];
	init_stage_data(s, "BK3_0.TIM", "BK3_1.TIM",
		-100, 100, 2000,
		-95, 0, 0
	);
	set_plane(s, -50, 0, 0,
		250, 0, -1500
	);
	set_spawn(s, 100, 0, -1300,
		0, 2048, 0
	);
	set_spawn(s, 100, 0, -160,
		0, 0, 0
	);
	set_zone(s, -50, 0, -1490,
		250, 0, -20,
		0, 2 
	);
	set_zone(s, -50, 0, 0,
		250, 0, -20,
		4, 0 
	);

	// forest dirt road
	s = &stage[4];
	init_stage_data(s, "BK4_0.TIM", "BK4_1.TIM",
		-70, 1830, 3550,
		305, 0, 0
	);
	set_plane(s, -50, 0, 1500,
		250, 0, -3000
	);
	set_zone(s, -50, 0, -(1500-10),
		250, 0, -20,
		3, 1 
	);
	set_spawn(s, 100, 0, -1300,
		0, 2048, 0
	);

	return stage;
}

void write_stages_bin(StageData *stageData, int array_size){
	FILE *file = fopen(FILE_NAME, "wb");
	if (file == NULL) {
		perror("error: can't write file");
		exit(EXIT_FAILURE);
	}
	for(int i = 0; i < array_size; i++)
		fwrite(&stageData[i], sizeof(StageData), 1, file);
	fclose(file);

	file = fopen(FILE_NAME, "rb");
	if (file == NULL) {
		perror("error: can't read file");
		exit(EXIT_FAILURE);
	}
	for(int i = 0; i < array_size; i++){
		StageData stageData;
		fread(&stageData, sizeof(StageData), 1, file);
		printf("tim_0: %s\n", stageData.tims[0]);
		printf("npc talk: %s\n", stageData.npc.talk);
		//printf("planes_len: %d\n", stageData.planes_len);
	}
	fclose(file);
}

int main() {
	/*StageData *stageData = gen_stages();
	write_stages_bin(stageData, N_STAGES);
	free(stageData);*/
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

	int array_size = cJSON_GetArraySize(json_array);
	int index = 0;
	printf("Dimensione dell'array: %d\n\n", array_size);

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
		if (cJSON_IsArray(spawns)) {
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

		cJSON *npc = cJSON_GetObjectItemCaseSensitive(jStage, "npc");
		printf("strlen valuestring %ld", strlen(npc->valuestring));
		memcpy(&s->npc.talk, npc->valuestring, strlen(npc->valuestring));

		/*if (cJSON_IsString(name) && (name->valuestring != NULL)) {
			printf("Nome: %s\n", name->valuestring);
		if (cJSON_IsNumber(age))
			printf("Età: %d\n", age->valueint);
		if (cJSON_IsBool(is_student))
			printf("Studente: %s\n", cJSON_IsTrue(is_student) ? "sì" : "no");
		*/
		printf("\n");
		index++;
	}

	cJSON_Delete(json_array);
	free(json_data);
	write_stages_bin(stageData, array_size);
	free(stageData);
}

