#define EDGE_MAP_W 65535
#define EDGE_MAP_H 32

struct edge_info {

	unsigned hits : 15;
	unsigned is_free : 1;
	u16      pre_bb_id;
	u8       bonus;

};

struct edge_header {

	unsigned size1 : 8;
	unsigned size2 : 8;

};

struct edge_map {

	struct edge_header headers[EDGE_MAP_W];
	struct edge_info   trace[EDGE_MAP_W][EDGE_MAP_H];
	struct edge_info   untouch[EDGE_MAP_W][EDGE_MAP_H];

};

void debug(struct edge_map* map) {

	if (!map) {
		return;
	}

	for (int i = 0; i < EDGE_MAP_W; i++) {
		if (!map->headers[i].size1) continue;
		for (int j = 0; j < map->headers[i].size1; j++) {
			fprintf(stderr, "Trace [Edge: %04x-%02d, Pre_BB: %04x]\n", i, j, map->trace[i][j].pre_bb_id);
		}
	}

	fprintf(stderr, "\n");

	for (int i = 0; i < EDGE_MAP_W; i++) {
		if (!map->headers[i].size2) continue;
		for (int j = 0; j < map->headers[i].size2; j++) {
			fprintf(stderr, "Untouch [Edge: %04x-%02d, Pre_BB: %04x, Bonus: %d]\n", i, j, map->untouch[i][j].pre_bb_id, map->untouch[i][j].bonus);
		}
	}

}