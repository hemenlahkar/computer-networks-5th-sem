#include "graph.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void graph_init(Graph *g, int n) {
	if (n > MAX_NODES) n = MAX_NODES;
	memset(g, 0, sizeof(*g));
	g->n = n;
	for (int i = 0; i < n; i++) {
		snprintf(g->names[i], NAME_LEN, "R%d", i);
		for (int j = 0; j < n; j++)
			g->adj[i][j] = (i == j) ? 0 : GRAPH_INF;
	}
}

void graph_set_name(Graph *g, int i, const char *name) {
	strncpy(g->names[i], name, NAME_LEN - 1);
	g->names[i][NAME_LEN - 1] = '\0';
}

int graph_find_node(const Graph *g, const char *name) {
	for (int i = 0; i < g->n; i++)
		if (strcmp(g->names[i], name) == 0) return i;
	return -1;
}

void graph_add_edge(Graph *g, int u, int v, int w) {
	if (u < 0 || u >= g->n || v < 0 || v >= g->n) return;
	g->adj[u][v] = w;
	g->adj[v][u] = w;
}

int graph_load_file(Graph *g, const char *path) {
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
			int n = 0;
			if (sscanf(p + 5, "%d", &n) != 1 ||
			    n < 1 || n > MAX_NODES) {
				fclose(fp); return -2;
			}
			graph_init(g, n);
			have_nodes = 1;
		} else if (strcmp(key, "names") == 0) {
			if (!have_nodes) { fclose(fp); return -3; }
			p += 5;
			for (int i = 0; i < g->n; i++) {
				char nm[NAME_LEN];
				if (sscanf(p, "%15s", nm) != 1) {
					fclose(fp); return -4;
				}
				graph_set_name(g, i, nm);
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
			int u = graph_find_node(g, un);
			int v = graph_find_node(g, vn);
			if (u < 0 || v < 0) { fclose(fp); return -6; }
			graph_add_edge(g, u, v, w);
		}
	}
	fclose(fp);
	return have_nodes ? 0 : -7;
}

void dijkstra_run(const Graph *g, int src, SPT *spt, int verbose) {
	int visited[MAX_NODES] = {0};

	spt->src           = src;
	spt->relaxations   = 0;
	for (int i = 0; i < g->n; i++) {
		spt->dist[i]     = GRAPH_INF;
		spt->prev[i]     = -1;
		spt->next_hop[i] = -1;
	}
	spt->dist[src]     = 0;
	spt->next_hop[src] = src;

	if (verbose) {
		printf("\n[verbose] Dijkstra from %s\n", g->names[src]);
	}

	for (int iter = 0; iter < g->n; iter++) {
		/* -------- 1. extract-min (linear scan) -------- */
		int u = -1, best = GRAPH_INF;
		for (int i = 0; i < g->n; i++) {
			if (!visited[i] && spt->dist[i] < best) {
				best = spt->dist[i];
				u    = i;
			}
		}
		if (u == -1) break;

		visited[u] = 1;

		if (verbose) {
			printf("  pick %-8s (dist=%d)\n",
			       g->names[u], spt->dist[u]);
		}

		for (int v = 0; v < g->n; v++) {
			if (v == u || visited[v])      continue;
			int w = g->adj[u][v];
			if (w == GRAPH_INF)            continue;

			int nd = spt->dist[u] + w;
			if (nd < spt->dist[v]) {
				if (verbose) {
					printf("    relax %-8s via %-8s : "
					       "%d -> %d\n",
					       g->names[v], g->names[u],
					       spt->dist[v], nd);
				}
				spt->dist[v]     = nd;
				spt->prev[v]     = u;
				spt->next_hop[v] = (u == src)
				                       ? v
				                       : spt->next_hop[u];
				spt->relaxations++;
			}
		}
	}
}

void graph_print_topology(const Graph *g) {
	printf("\nTopology: %d node(s)\n", g->n);
	printf("  Nodes: ");
	for (int i = 0; i < g->n; i++)
		printf("%s%s", i ? " " : "", g->names[i]);
	printf("\n  Links:\n");
	int m = 0;
	for (int i = 0; i < g->n; i++)
		for (int j = i + 1; j < g->n; j++)
			if (g->adj[i][j] != GRAPH_INF && g->adj[i][j] != 0) {
				printf("    %-6s --- %-6s : %d\n",
				       g->names[i], g->names[j], g->adj[i][j]);
				m++;
			}
	printf("  Total undirected links: %d\n", m);
}

static void print_path_rec(const Graph *g, const SPT *spt, int dst) {
	if (spt->prev[dst] == -1) {
		printf("%s", g->names[dst]);
		return;
	}
	print_path_rec(g, spt, spt->prev[dst]);
	printf(" -> %s", g->names[dst]);
}

void spt_print_routing_table(const Graph *g, const SPT *spt) {
	printf("\nRouting table at %s (cost / next hop / full path):\n",
	       g->names[spt->src]);
	printf("  %-8s %-6s %-10s %s\n",
	       "Dest", "Cost", "Next hop", "Path");
	printf("  -------- ------ ---------- "
	       "-------------------------------\n");

	for (int d = 0; d < g->n; d++) {
		if (d == spt->src) continue;
		if (spt->dist[d] == GRAPH_INF) {
			printf("  %-8s %-6s %-10s (unreachable)\n",
			       g->names[d], "inf", "-");
		} else {
			printf("  %-8s %-6d %-10s ",
			       g->names[d], spt->dist[d],
			       g->names[spt->next_hop[d]]);
			print_path_rec(g, spt, d);
			putchar('\n');
		}
	}
}

static void print_subtree(const Graph *g, const SPT *spt, int node,
                          const char *prefix, int is_last, int is_root) {
	if (is_root)
		printf("%s (root)\n", g->names[node]);
	else
		printf("%s%s %s (cost %d)\n",
		       prefix, is_last ? "`--" : "|--",
		       g->names[node], spt->dist[node]);

	int children[MAX_NODES];
	int n_children = 0;
	for (int i = 0; i < g->n; i++)
		if (i != spt->src && spt->prev[i] == node)
			children[n_children++] = i;

	char new_prefix[512];
	if (is_root)
		new_prefix[0] = '\0';
	else
		snprintf(new_prefix, sizeof new_prefix, "%s%s",
		         prefix, is_last ? "    " : "|   ");

	for (int k = 0; k < n_children; k++)
		print_subtree(g, spt, children[k], new_prefix,
		              k == n_children - 1, 0);
}

void spt_print_tree(const Graph *g, const SPT *spt) {
	printf("\nShortest-path tree rooted at %s:\n",
	       g->names[spt->src]);
	print_subtree(g, spt, spt->src, "", 1, 1);
	printf("  (successful relaxations: %d)\n", spt->relaxations);
}
