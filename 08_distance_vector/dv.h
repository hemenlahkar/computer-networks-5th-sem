#ifndef __DV_H__
#define __DV_H__

#define MAX_NODES  32
#define NAME_LEN   16
#define DV_INF     16

typedef struct {
	int  n;
	char names[MAX_NODES][NAME_LEN];
	int  cost[MAX_NODES][MAX_NODES];
} Topology;

typedef struct {
	int n;
	int dist[MAX_NODES][MAX_NODES];
	int next_hop[MAX_NODES][MAX_NODES];
} DVSys;

typedef enum {
	DV_MODE_BASIC,
	DV_MODE_SPLIT,
	DV_MODE_POISON
} DVMode;

void topo_init(Topology *t, int n);
void topo_set_name(Topology *t, int i, const char *name);
int  topo_find_node(const Topology *t, const char *name);
void topo_add_edge(Topology *t, int u, int v, int w);
void topo_fail_link(Topology *t, int u, int v);
int  topo_load_file(Topology *t, const char *path);
void topo_print(const Topology *t);

void dv_init(DVSys *sys, const Topology *t);
int  dv_step(DVSys *sys, const Topology *t, DVMode mode, int verbose);
int  dv_run(DVSys *sys, const Topology *t, DVMode mode,
            int max_iters, int verbose);

void dv_print_all(const DVSys *sys, const Topology *t);
int  dv_is_fully_reachable(const DVSys *sys);

#endif
