#include <malloc.h>
#include <queue>
#include <set>

#define MAX_VERTEX_NUM 10000
#define MAX_LOOP_CNT 10

#define EDGE_MAP_W 65535
#define EDGE_MAP_H 32

#define AFL_BONUS_DEBUG

typedef struct ArcNode {

	int adjvex;
	struct ArcNode* nextarc;

}Edge;

typedef struct VNode {

	BasicBlock* bb;
	std::string bb_name;

	unsigned int cur_loc;

	int indegree;
	int outdegree;

	uint8_t visited;

	unsigned int bonus;
	unsigned int calledFunNum;

	Edge* firstarc;

	int loop_pre_bbs[MAX_LOOP_CNT];
	unsigned int loop_cnt;
	u8  is_loop;

}BBNode;

typedef struct ALGraph {

	BBNode list[MAX_VERTEX_NUM];

	int bb_num, edge_num;

}CFGraph;

struct edge_info {
	
	u16 hits;
	u16 pre_bb_id;

};

struct edge_map{

	struct edge_info trace[EDGE_MAP_W][EDGE_MAP_H];
	struct edge_info untouch[EDGE_MAP_W][EDGE_MAP_H];

};


void debug(CFGraph *cfg) {

	for (int i = 0; i < cfg->bb_num; i++) {

		BBNode* bb = &cfg->list[i];

		outs() << "BB: " << bb->bb_name << " (No." << i << ")\n";
		outs() << "		indegree: " << bb->indegree << "\n";
		outs() << "		loop: " << bb->loop_cnt << "\n";
		for (int j = 0; j < bb->loop_cnt; j++) {
			outs() << "			" << "No." << j << ": " << cfg->list[bb->loop_pre_bbs[j]].bb_name << "\n";
		}
		outs() << "		bonus: " << bb->bonus << "\n";
		outs() << "		CalledFunNum: " << bb->calledFunNum << "\n";
		outs() << "		Edges: " << "\n";
		Edge* tmp = bb->firstarc;
		while(tmp){
			outs() << "			" << i << " ==> " << tmp->adjvex << "\n";
			tmp = tmp->nextarc;
		}
		outs() << "************************************************************************" << "\n";

	}

}

int search_bb(CFGraph *cfg, BasicBlock* BB) {
	
	for (int i = 0; i < cfg->bb_num; i++) {
		if (cfg->list[i].bb == BB) {
			return i;
		}
	}

	return -1;  // no find

}

int insert_bb(CFGraph* cfg, BasicBlock* BB, unsigned int cur_loc) {

	int bb_idx = search_bb(cfg, BB);
	if (bb_idx != -1) // exist bb!
		return bb_idx;

	// create BBNode
	bb_idx = cfg->bb_num++;
	if (bb_idx >= MAX_VERTEX_NUM) {
		errs() << "Too many BBs?!\n";
		exit(-1);
	}

	cfg->list[bb_idx].bb = BB;
	cfg->list[bb_idx].cur_loc = cur_loc;
	cfg->list[bb_idx].indegree = 0;
	cfg->list[bb_idx].outdegree = 0;
	cfg->list[bb_idx].visited = 0;
	cfg->list[bb_idx].bonus = 0;
	cfg->list[bb_idx].firstarc = NULL;
	cfg->list[bb_idx].loop_cnt = 0;
	cfg->list[bb_idx].is_loop = 0;
	cfg->list[bb_idx].calledFunNum = 0;

	return bb_idx;

}

bool insert_edge(CFGraph* cfg, BasicBlock* BB, int bb_idx, int suc_bb_idx) {

	Edge* arc = (Edge*)malloc(sizeof(Edge));
	arc->adjvex = suc_bb_idx;
	arc->nextarc = NULL;

	Edge* t = cfg->list[bb_idx].firstarc;
	Edge* last_cur;
	cfg->edge_num++;
	cfg->list[bb_idx].outdegree++;
	cfg->list[suc_bb_idx].indegree++;

	if (!t) {
		cfg->list[bb_idx].firstarc = arc;
		return true;
	}

	while (t) {
		if (t->adjvex == suc_bb_idx) {
			free(arc);
			cfg->edge_num--;
			cfg->list[bb_idx].outdegree--;
			cfg->list[suc_bb_idx].indegree++;
			return false;
		}
		last_cur = t;
		t = t->nextarc;
	}

	last_cur->nextarc = arc;
	return true;

}

int delete_edge(CFGraph* cfg, BBNode* bb, int adjvex) {

	if (!bb || !bb->firstarc) return -1;
	
	Edge* prev = NULL;
	Edge* start = bb->firstarc;

	while (start) {

		if (start->adjvex == adjvex) {

			if (!prev) {
				bb->firstarc = start->nextarc;
			}
			else {
				prev->nextarc = start->nextarc;
			}

			free(start);
			return 1;
		}

		prev = start;
		start = start->nextarc;
	}

	return -1; // no find?

}

void dfs(CFGraph* cfg, Edge* edge, int pre_bb_idx) {

	BBNode* bb = &(cfg->list[edge->adjvex]);

	if (bb->outdegree == 0 || bb->visited == 2) {
		return;
	}

	if (bb->visited) {

		// loop detected
		bb->is_loop = 1;

		if (bb->loop_cnt >= MAX_LOOP_CNT) {
			return;
		}
		bb->loop_pre_bbs[bb->loop_cnt++] = pre_bb_idx;

		if (bb->loop_cnt >= MAX_LOOP_CNT) {
			errs() << "Too many looppppppps?!\n";
			exit(-1);
		}

		return;

	}

	if (bb->visited == 0)
		bb->visited = 1;

	for (Edge* node = bb->firstarc; node != NULL; node = node->nextarc)
		dfs(cfg, node, edge->adjvex);

	bb->visited = 2;

}

void loop_detect(CFGraph* cfg) {

	if (!cfg->edge_num) return; // no need!

	for (Edge* node = cfg->list[0].firstarc; node != NULL; node = node->nextarc) {

		cfg->list[0].visited = 0;
		dfs(cfg, node, 0);
		cfg->list[0].visited = 2;

	}

	// complete loop detect :)

}

void delete_loop(CFGraph* cfg) {

	if (!cfg->edge_num) return;

	for (int i = 0; i < cfg->bb_num; i++) {

		if (!cfg->list[i].loop_cnt) continue;

		// delete all loops!
		
		for (int j = 0; j < cfg->list[i].loop_cnt; j++) {
			
			BBNode* pre_bb = &cfg->list[cfg->list[i].loop_pre_bbs[j]];

			if (delete_edge(cfg, pre_bb, i) == -1) {
				
				errs() << "Oops?! There's something wrong when deleting loops?\n";
				exit(-1);

			}

			pre_bb->outdegree--;

		}

		cfg->edge_num -= cfg->list[i].loop_cnt;
		cfg->list[i].indegree--;
		cfg->list[i].loop_cnt = 0;

	}

}


void dom_dfs(CFGraph* cfg, Edge* edge, int pre_bb_idx, set<int>& record) {

	BBNode* bb = &(cfg->list[edge->adjvex]);

	if (bb->visited == 2) {
		return;
	}

	if (bb->visited == 0)
		bb->visited = 2;
	
	record.insert(edge->adjvex);

	for (Edge* node = bb->firstarc; node != NULL; node = node->nextarc)
		dom_dfs(cfg, node, edge->adjvex, record);

	bb->visited = 2;

}

void update_bonus(CFGraph* cfg) {

	// The key insight is that we build dominator tree,
	// the bb's bonius = bbs dominated + call function in each bbs
	
	int bb_n = cfg->bb_num;
	if (!bb_n) return;

	vector<vector<int>> res(bb_n);

	for (int i = 1; i < bb_n; i++) {
		
		for (int j = 1; j < bb_n; j++) { cfg->list[j].visited = 0; }

		set<int> cur_record;

		// delete this node
		cfg->list[i].visited = 2;
		cfg->list[0].visited = 2;

		for (Edge* node = cfg->list[0].firstarc; node != NULL; node = node->nextarc) {

			if (node->adjvex == i) continue;
			dom_dfs(cfg, node, 0, cur_record);

		}

		cfg->list[i].bonus = bb_n - 1 - cur_record.size();

		/*
		for (int j = 1; j < bb_n; j++) {
			if (i == j)  continue;
			if (cur_record.find(j) == cur_record.end())
				cfg->list[i].bonus += cfg->list[j].calledFunNum;
		}
		*/

		if (cfg->list[i].bonus > 65535) {
			cfg->list[i].bonus = 65535;
		}

		cfg->list[0].visited = 0;
		cfg->list[i].visited = 0;

	}

#ifdef AFL_BONUS_DEBUG
	// debug(cfg);
#endif // AFL_BONUS_DEBUG

}