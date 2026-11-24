#include "dv.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void topo_init(Topology *t, int n) {
	if (n > MAX_NODES) n = MAX_NODES;
	memset(t, 0, sizeof *t);
	t->n = n;
	for (int i = 0; i < n; i++) {
		snprintf(t->names[i], NAME_LEN, "R%d", i);
		for (int j = 0; j < n; j++)
			t->cost[i][j] = (i == j) ? 0 : DV_INF;
	}
}

void topo_set_name(Topology *t, int i, const char *name) {
	strncpy(t->names[i], name, NAME_LEN - 1);
	t->names[i][NAME_LEN - 1] = '\0';
}

int topo_find_node(const Topology *t, const char *name) {
	for (int i = 0; i < t->n; i++)
		if (strcmp(t->names[i], name) == 0) return i;
	return -1;
}

void topo_add_edge(Topology *t, int u, int v, int w) {
	if (u < 0 || u >= t->n || v < 0 || v >= t->n) return;
	if (u == v) return;
	if (w < 0) w = 0;
	if (w >= DV_INF) {
		fprintf(stderr,
		        "[topo] edge cost %d >= DV_INF(%d); clamped\n",
		        w, DV_INF);
		w = DV_INF - 1;
	}
	t->cost[u][v] = w;
	t->cost[v][u] = w;
}

void topo_fail_link(Topology *t, int u, int v) {
	if (u < 0 || u >= t->n || v < 0 || v >= t->n) return;
	t->cost[u][v] = DV_INF;
	t->cost[v][u] = DV_INF;
}

int topo_load_file(Topology *t, const char *path) {
	FILE *fp = fopen(path, "r");
	if (!fp) return -1;

	char line[512];
	int  have_nodes = 0;

	while (fgets(line, sizeof line, fp)) {
		char *p = line;
		while (*p && isspace((unsigned char)*p)) p++;
		if (*p == '#' || *p == '\0') continue;

		char key[16];
		if (sscanf(p, "%15s", key) != 1) continue;

		if (strcmp(key, "nodes") == 0) {
			int n;
			if (sscanf(p + 5, "%d", &n) != 1 ||
			    n < 1 || n > MAX_NODES) {
				fclose(fp); return -2;
			}
			topo_init(t, n);
			have_nodes = 1;
		} else if (strcmp(key, "names") == 0) {
			if (!have_nodes) { fclose(fp); return -3; }
			p += 5;
			for (int i = 0; i < t->n; i++) {
				char nm[NAME_LEN];
				if (sscanf(p, "%15s", nm) != 1) {
					fclose(fp); return -4;
				}
				topo_set_name(t, i, nm);
				while (*p && !isspace((unsigned char)*p)) p++;
				while (*p &&  isspace((unsigned char)*p)) p++;
			}
		} else if (strcmp(key, "edge") == 0) {
			if (!have_nodes) { fclose(fp); return -3; }
			char un[NAME_LEN], vn[NAME_LEN];
			int  w;
			if (sscanf(p + 4, "%15s %15s %d", un, vn, &w) != 3) {
				fclose(fp); return -5;
			}
			int u = topo_find_node(t, un);
			int v = topo_find_node(t, vn);
			if (u < 0 || v < 0) { fclose(fp); return -6; }
			topo_add_edge(t, u, v, w);
		}
	}
	fclose(fp);
	return have_nodes ? 0 : -7;
}

void topo_print(const Topology *t) {
	printf("Topology: %d nodes\n  Nodes: ", t->n);
	for (int i = 0; i < t->n; i++)
		printf("%s%s", i ? " " : "", t->names[i]);
	putchar('\n');

	printf("  Links:\n");
	int m = 0;
	for (int i = 0; i < t->n; i++)
		for (int j = i + 1; j < t->n; j++)
			if (t->cost[i][j] < DV_INF) {
				printf("    %-6s --- %-6s : %d\n",
				       t->names[i], t->names[j], t->cost[i][j]);
				m++;
			}
	printf("  Active links: %d\n", m);
}

void dv_init(DVSys *sys, const Topology *t) {
	int n = t->n;
	sys->n = n;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (i == j) {
				sys->dist[i][j] = 0;
				sys->next_hop[i][j] = i;
			} else if (t->cost[i][j] < DV_INF) {
				sys->dist[i][j] = t->cost[i][j];
				sys->next_hop[i][j] = j;
			} else {
				sys->dist[i][j] = DV_INF;
				sys->next_hop[i][j] = -1;
			}
		}
	}
}

int dv_step(DVSys *sys, const Topology *t, DVMode mode, int verbose) {
	int n = sys->n;

	int old_dist[MAX_NODES][MAX_NODES];
	int old_next[MAX_NODES][MAX_NODES];
	memcpy(old_dist, sys->dist, sizeof old_dist);
	memcpy(old_next, sys->next_hop, sizeof old_next);

	int changes = 0;

	for (int v = 0; v < n; v++) {
		for (int d = 0; d < n; d++) {
			if (d == v) continue;

			int best_cost = DV_INF;
			int best_hop  = -1;

			for (int i = 0; i < n; i++) {
				if (i == v) continue;
				int link = t->cost[v][i];
				if (link >= DV_INF) continue;

				int adv = old_dist[i][d];
				if (mode != DV_MODE_BASIC &&
				    old_next[i][d] == v) {
					adv = DV_INF;
				}
				if (adv >= DV_INF) continue;

				int cand = link + adv;
				if (cand > DV_INF) cand = DV_INF;
				if (cand < best_cost) {
					best_cost = cand;
					best_hop  = i;
				}
			}

			if (best_cost != old_dist[v][d] ||
			    best_hop  != old_next[v][d]) {
				changes++;
				if (verbose) {
					const char *oh = (old_next[v][d] >= 0)
					                 ? t->names[old_next[v][d]]
					                 : "-";
					if (best_cost >= DV_INF)
						printf("  [%s->%s] unreachable "
						       "(was %d via %s)\n",
						       t->names[v], t->names[d],
						       old_dist[v][d], oh);
					else
						printf("  [%s->%s] %d via %s "
						       "(was %d via %s)\n",
						       t->names[v], t->names[d],
						       best_cost, t->names[best_hop],
						       old_dist[v][d], oh);
				}
			}
			sys->dist[v][d]     = best_cost;
			sys->next_hop[v][d] = best_hop;
		}
	}
	return changes;
}

int dv_run(DVSys *sys, const Topology *t, DVMode mode,
           int max_iters, int verbose) {
	for (int r = 1; r <= max_iters; r++) {
		int changes = dv_step(sys, t, mode, verbose);
		if (verbose)
			printf("  Round %d: %d change(s)\n", r, changes);
		if (changes == 0) return r;
	}
	return -1;
}

void dv_print_all(const DVSys *sys, const Topology *t) {
	for (int v = 0; v < sys->n; v++) {
		printf("\n  Router %s:\n", t->names[v]);
		printf("    %-6s %-6s %-10s\n", "Dest", "Cost", "Next hop");
		printf("    %-6s %-6s %-10s\n", "------", "-----", "--------");
		for (int d = 0; d < sys->n; d++) {
			if (d == v) {
				printf("    %-6s %-6d %-10s\n",
				       t->names[d], 0, t->names[v]);
			} else if (sys->dist[v][d] >= DV_INF) {
				printf("    %-6s %-6s %-10s\n",
				       t->names[d], "inf", "-");
			} else {
				printf("    %-6s %-6d %-10s\n",
				       t->names[d], sys->dist[v][d],
				       t->names[sys->next_hop[v][d]]);
			}
		}
	}
}

int dv_is_fully_reachable(const DVSys *sys) {
	for (int i = 0; i < sys->n; i++)
		for (int j = 0; j < sys->n; j++)
			if (sys->dist[i][j] >= DV_INF) return 0;
	return 1;
}
