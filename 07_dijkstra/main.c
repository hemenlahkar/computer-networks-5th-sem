#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"

static void load_wiki(Graph *g) {
	graph_init(g, 6);
	const char *n[] = {"A", "B", "C", "D", "E", "F"};
	for (int i = 0; i < 6; i++) graph_set_name(g, i, n[i]);
	graph_add_edge(g, 0, 1,  7);
	graph_add_edge(g, 0, 2,  9);
	graph_add_edge(g, 0, 5, 14);
	graph_add_edge(g, 1, 2, 10);
	graph_add_edge(g, 1, 3, 15);
	graph_add_edge(g, 2, 3, 11);
	graph_add_edge(g, 2, 5,  2);
	graph_add_edge(g, 3, 4,  6);
	graph_add_edge(g, 4, 5,  9);
}

static void load_ring(Graph *g) {
	graph_init(g, 5);
	for (int i = 0; i < 5; i++) {
		char nm[NAME_LEN];
		snprintf(nm, sizeof nm, "R%d", i);
		graph_set_name(g, i, nm);
	}
	graph_add_edge(g, 0, 1, 1);
	graph_add_edge(g, 1, 2, 2);
	graph_add_edge(g, 2, 3, 3);
	graph_add_edge(g, 3, 4, 4);
	graph_add_edge(g, 4, 0, 5);
}

static void load_line(Graph *g) {
	graph_init(g, 5);
	for (int i = 0; i < 5; i++) {
		char nm[NAME_LEN];
		snprintf(nm, sizeof nm, "N%d", i);
		graph_set_name(g, i, nm);
	}
	for (int i = 0; i < 4; i++) graph_add_edge(g, i, i + 1, i + 1);
}

static void load_mesh(Graph *g) {
	graph_init(g, 4);
	const char *n[] = {"R0", "R1", "R2", "R3"};
	for (int i = 0; i < 4; i++) graph_set_name(g, i, n[i]);
	graph_add_edge(g, 0, 1, 1);
	graph_add_edge(g, 0, 2, 4);
	graph_add_edge(g, 0, 3, 5);
	graph_add_edge(g, 1, 2, 2);
	graph_add_edge(g, 1, 3, 3);
	graph_add_edge(g, 2, 3, 1);
}

static void load_star(Graph *g) {
	graph_init(g, 5);
	graph_set_name(g, 0, "HUB");
	for (int i = 1; i < 5; i++) {
		char nm[NAME_LEN];
		snprintf(nm, sizeof nm, "S%d", i);
		graph_set_name(g, i, nm);
	}
	for (int i = 1; i < 5; i++) graph_add_edge(g, 0, i, i);
}

static int load_topology(Graph *g, const char *name) {
	if (strcmp(name, "wiki") == 0) { load_wiki(g); return 0; }
	if (strcmp(name, "ring") == 0) { load_ring(g); return 0; }
	if (strcmp(name, "line") == 0) { load_line(g); return 0; }
	if (strcmp(name, "mesh") == 0) { load_mesh(g); return 0; }
	if (strcmp(name, "star") == 0) { load_star(g); return 0; }
	return -1;
}

static void usage(const char *prog) {
	fprintf(stderr,
	    "Usage: %s [options]\n"
	    "\n"
	    "Dijkstra shortest-path routing simulation.\n"
	    "\n"
	    "Options:\n"
	    "  -t <name>   built-in topology: wiki, ring, line, mesh, star\n"
	    "              (default: wiki)\n"
	    "  -f <file>   load topology from file (overrides -t)\n"
	    "  -s <node>   source node (name or index); default = first node\n"
	    "  -a          run all-pairs (one Dijkstra per source)\n"
	    "  -v          verbose step-by-step trace\n"
	    "  -h          show this help\n"
	    "\n"
	    "Topology file format:\n"
	    "  nodes N\n"
	    "  names A B C ...          (N names)\n"
	    "  edge U V W\n"
	    "  edge U V W\n"
	    "  ...\n", prog);
}

int main(int argc, char **argv) {
	const char *topo     = "wiki";
	const char *file     = NULL;
	const char *src_name = NULL;
	int all_pairs = 0, verbose = 0;

	int opt;
	while ((opt = getopt(argc, argv, "t:f:s:avh")) != -1) {
		switch (opt) {
			case 't': topo      = optarg; break;
			case 'f': file      = optarg; break;
			case 's': src_name  = optarg; break;
			case 'a': all_pairs = 1;      break;
			case 'v': verbose   = 1;      break;
			case 'h': usage(argv[0]); return 0;
			default:  usage(argv[0]); return 1;
		}
	}

	Graph g;
	memset(&g, 0, sizeof g);

	if (file) {
		int rc = graph_load_file(&g, file);
		if (rc != 0) {
			fprintf(stderr,
			        "Failed to load topology '%s' (err %d).\n",
			        file, rc);
			return 1;
		}
	} else if (load_topology(&g, topo) != 0) {
		fprintf(stderr, "Unknown topology '%s'. Try -h.\n", topo);
		return 1;
	}

	printf("==================================================\n");
	printf("   DIJKSTRA SHORTEST-PATH ROUTING SIMULATION\n");
	printf("==================================================\n");
	graph_print_topology(&g);

	int src_idx = 0;
	if (src_name) {
		src_idx = graph_find_node(&g, src_name);
		if (src_idx < 0) src_idx = atoi(src_name);
		if (src_idx < 0 || src_idx >= g.n) {
			fprintf(stderr, "Invalid source '%s'.\n", src_name);
			return 1;
		}
	}

	if (all_pairs) {
		for (int s = 0; s < g.n; s++) {
			SPT spt;
			dijkstra_run(&g, s, &spt, verbose);
			spt_print_routing_table(&g, &spt);
			spt_print_tree(&g, &spt);
		}
	} else {
		SPT spt;
		dijkstra_run(&g, src_idx, &spt, verbose);
		spt_print_routing_table(&g, &spt);
		spt_print_tree(&g, &spt);
	}

	return 0;
}
