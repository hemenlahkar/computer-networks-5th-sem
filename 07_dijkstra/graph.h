#ifndef __GRAPH_H__
#define __GRAPH_H__

#define MAX_NODES  64
#define GRAPH_INF  0x3f3f3f3f
#define NAME_LEN   16

typedef struct {
	int  n;
	char names[MAX_NODES][NAME_LEN];
	int  adj[MAX_NODES][MAX_NODES];
} Graph;

typedef struct {
	int src;
	int dist[MAX_NODES];
	int prev[MAX_NODES];
	int next_hop[MAX_NODES];
	int relaxations;
} SPT;

void graph_init(Graph *g, int n);
void graph_set_name(Graph *g, int i, const char *name);
int  graph_find_node(const Graph *g, const char *name);
void graph_add_edge(Graph *g, int u, int v, int w);
int  graph_load_file(Graph *g, const char *path);

void dijkstra_run(const Graph *g, int src, SPT *spt, int verbose);

void graph_print_topology(const Graph *g);
void spt_print_routing_table(const Graph *g, const SPT *spt);
void spt_print_tree(const Graph *g, const SPT *spt);

#endif
