#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>

#define FILE_NAME "STAGES.BIN"
#define N_STAGES 5 

typedef struct PlaneData {
	int x, y, z;
	int w, h, d;
} PlaneData;

typedef struct SpawnData {
	int x, y, z;
	short rx, ry, rz; 
} SpawnData;

typedef struct ZoneData {
	int x, y, z;
	int w, h, d;
	int stage_id;
	int spawn_id;
} ZoneData;

typedef struct StageData {
	char tims[2][10];
	int cam_x, cam_y, cam_z;
	short cam_rx, cam_ry, cam_rz;
	PlaneData planes[5];
	SpawnData spawns[5];
	ZoneData zones[5];
	unsigned char planes_len;
	unsigned char spawns_len;
	unsigned char zones_len;
} StageData;

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

int main() {
	StageData *stage = gen_stages();

	FILE *file = fopen(FILE_NAME, "wb");
	if (file == NULL) {
		perror("error: can't write file");
		exit(EXIT_FAILURE);
	}
	for(int i = 0; i < N_STAGES; i++)
		fwrite(&stage[i], sizeof(StageData), 1, file);
	fclose(file);

	file = fopen(FILE_NAME, "rb");
	if (file == NULL) {
		perror("error: can't read file");
		exit(EXIT_FAILURE);
	}
	for(int i = 0; i < N_STAGES; i++){
		StageData stage;
		fread(&stage, sizeof(StageData), 1, file);
		printf("tim0: %s\n", stage.tims[0]);
		//printf("planes_len: %d\n", stage.planes_len);
	}
	fclose(file);

	free(stage);

	//printf("sizeof short: %d\n", sizeof(short));
	//printf("sizeof int: %d\n", sizeof(int));
	
	parse_json();

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
    // Legge il contenuto del file JSON
    char *json_data = read_json_file("stages.json");
    if (json_data == NULL) {
        return;
    }

    // Parse il contenuto JSON
    cJSON *json = cJSON_Parse(json_data);
    if (json == NULL) {
        printf("Errore nel parsing del JSON\n");
        free(json_data);
        return;
    }

    // Estrai i dati dal JSON
    cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    cJSON *age = cJSON_GetObjectItemCaseSensitive(json, "age");
    cJSON *is_student = cJSON_GetObjectItemCaseSensitive(json, "is_student");

    if (cJSON_IsString(name) && (name->valuestring != NULL)) {
        printf("Nome: %s\n", name->valuestring);
    }

    if (cJSON_IsNumber(age)) {
        printf("Età: %d\n", age->valueint);
    }

    if (cJSON_IsBool(is_student)) {
        printf("Studente: %s\n", cJSON_IsTrue(is_student) ? "sì" : "no");
    }

    // Libera la memoria
    cJSON_Delete(json);
    free(json_data);
}

