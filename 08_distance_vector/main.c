#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dv.h"

static void load_line3(Topology *t) {
	topo_init(t, 3);
	topo_set_name(t, 0, "A");
	topo_set_name(t, 1, "B");
	topo_set_name(t, 2, "C");
	topo_add_edge(t, 0, 1, 1);
	topo_add_edge(t, 1, 2, 1);
}

static void load_line4(Topology *t) {
	topo_init(t, 4);
	topo_set_name(t, 0, "A");
	topo_set_name(t, 1, "B");
	topo_set_name(t, 2, "C");
	topo_set_name(t, 3, "D");
	topo_add_edge(t, 0, 1, 1);
	topo_add_edge(t, 1, 2, 1);
	topo_add_edge(t, 2, 3, 1);
}

static void load_ring(Topology *t) {
	topo_init(t, 4);
	topo_set_name(t, 0, "A");
	topo_set_name(t, 1, "B");
	topo_set_name(t, 2, "C");
	topo_set_name(t, 3, "D");
	topo_add_edge(t, 0, 1, 1);
	topo_add_edge(t, 1, 2, 2);
	topo_add_edge(t, 2, 3, 3);
	topo_add_edge(t, 3, 0, 4);
}

static void load_mesh(Topology *t) {
	topo_init(t, 5);
	const char *names[] = { "A", "B", "C", "D", "E" };
	for (int i = 0; i < 5; i++) topo_set_name(t, i, names[i]);
	topo_add_edge(t, 0, 1, 2);
	topo_add_edge(t, 0, 2, 3);
	topo_add_edge(t, 1, 2, 1);
	topo_add_edge(t, 1, 3, 1);
	topo_add_edge(t, 2, 3, 2);
	topo_add_edge(t, 3, 4, 1);
}

static int load_builtin(Topology *t, const char *name) {
	if (!strcmp(name, "line3")) { load_line3(t); return 0; }
	if (!strcmp(name, "line4")) { load_line4(t); return 0; }
	if (!strcmp(name, "ring"))  { load_ring (t); return 0; }
	if (!strcmp(name, "mesh"))  { load_mesh (t); return 0; }
	return -1;
}

static const char *mode_name(DVMode m) {
	switch (m) {
		case DV_MODE_BASIC:  return "basic";
		case DV_MODE_SPLIT:  return "split-horizon";
		case DV_MODE_POISON: return "poison-reverse";
	}
	return "?";
}

static int resolve_node(const Topology *t, const char *s) {
	char *end;
	long v = strtol(s, &end, 10);
	if (*end == '\0' && v >= 0 && v < t->n) return (int)v;
	return topo_find_node(t, s);
}

static void usage(const char *prog) {
	fprintf(stderr,
	    "Usage: %s [options]\n"
	    "\n"
	    "Distance Vector routing simulation with count-to-infinity demo.\n"
	    "\n"
	    "Options:\n"
	    "  -t <name>   built-in topology: line3, line4, ring, mesh\n"
	    "              (default: line3)\n"
	    "  -f <file>   load topology from file (overrides -t)\n"
	    "  -m <mode>   basic | split | poison    (default: basic)\n"
	    "  -i <n>      max iterations            (default: 40)\n"
	    "  -F <u>,<v>  after initial convergence, fail link u--v\n"
	    "              (u,v are names or indices)\n"
	    "  -v          verbose: print every route change\n"
	    "  -h          show this help\n"
	    "\n"
	    "Notes:\n"
	    "  * DV_INF = %d (RIP-style unreachable).  Valid link costs are\n"
	    "    in [0, %d].\n"
	    "  * 'basic' shows count-to-infinity after link failure; 'split'\n"
	    "    and 'poison' suppress it.\n",
	    prog, DV_INF, DV_INF - 1);
}

int main(int argc, char **argv) {
	const char *topo = "line3";
	const char *file = NULL;
	DVMode      mode = DV_MODE_BASIC;
	int         max_iters = 40;
	int         verbose   = 0;
	char        fail_u[NAME_LEN] = {0}, fail_v[NAME_LEN] = {0};
	int         have_fail = 0;

	int opt;
	while ((opt = getopt(argc, argv, "t:f:m:i:F:vh")) != -1) {
		switch (opt) {
			case 't': topo      = optarg;     break;
			case 'f': file      = optarg;     break;
			case 'i': max_iters = atoi(optarg); break;
			case 'v': verbose   = 1;          break;
			case 'm':
				if      (!strcmp(optarg, "basic"))  mode = DV_MODE_BASIC;
				else if (!strcmp(optarg, "split"))  mode = DV_MODE_SPLIT;
				else if (!strcmp(optarg, "poison")) mode = DV_MODE_POISON;
				else { fprintf(stderr, "Bad mode '%s'.\n", optarg);
				       return 1; }
				break;
			case 'F': {
				char *comma = strchr(optarg, ',');
				if (!comma) {
					fprintf(stderr, "-F needs u,v\n");
					return 1;
				}
				*comma = '\0';
				strncpy(fail_u, optarg,        NAME_LEN - 1);
				strncpy(fail_v, comma + 1,     NAME_LEN - 1);
				have_fail = 1;
				break;
			}
			case 'h': usage(argv[0]); return 0;
			default:  usage(argv[0]); return 1;
		}
	}

	if (max_iters < 1) max_iters = 1;

	Topology t;
	memset(&t, 0, sizeof t);
	if (file) {
		int rc = topo_load_file(&t, file);
		if (rc) { fprintf(stderr, "Failed to load '%s' (err %d).\n",
		                   file, rc); return 1; }
	} else if (load_builtin(&t, topo) != 0) {
		fprintf(stderr, "Unknown topology '%s'. Try -h.\n", topo);
		return 1;
	}

	printf("===================================================\n");
	printf("   DISTANCE VECTOR ROUTING SIMULATION\n");
	printf("===================================================\n");
	topo_print(&t);
	printf("\nMode: %s\n", mode_name(mode));
	printf("DV_INF (unreachable cost): %d\n", DV_INF);

	DVSys sys;
	dv_init(&sys, &t);

	printf("\n---- Initial convergence ----\n");
	int rounds = dv_run(&sys, &t, mode, max_iters, verbose);
	if (rounds < 0)
		printf("  [!] did not converge within %d rounds\n", max_iters);
	else
		printf("  Converged in %d round(s).\n", rounds);

	printf("\nFinal routing tables (initial state):");
	dv_print_all(&sys, &t);

	if (have_fail) {
		int u = resolve_node(&t, fail_u);
		int v = resolve_node(&t, fail_v);
		if (u < 0 || v < 0) {
			fprintf(stderr,
			        "\n[!] unknown node in -F %s,%s\n",
			        fail_u, fail_v);
			return 1;
		}

		printf("\n---- Failing link %s -- %s ----\n",
		       t.names[u], t.names[v]);
		topo_fail_link(&t, u, v);
		topo_print(&t);

		printf("\n---- Re-convergence (mode: %s) ----\n",
		       mode_name(mode));
		rounds = dv_run(&sys, &t, mode, max_iters, verbose);

		if (rounds < 0) {
			printf("  [!] did NOT converge within %d rounds "
			       "--> count-to-infinity!\n", max_iters);
		} else if (dv_is_fully_reachable(&sys)) {
			printf("  Converged in %d round(s); "
			       "all destinations reachable.\n", rounds);
		} else {
			printf("  Converged in %d round(s); "
			       "some destinations UNREACHABLE "
			       "(count-to-infinity resolved to %d).\n",
			       rounds, DV_INF);
		}

		printf("\nFinal routing tables (after link failure):");
		dv_print_all(&sys, &t);
	}

	return 0;
}
