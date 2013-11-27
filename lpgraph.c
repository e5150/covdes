/*
 * Copyright © 2011,2012,2013 Lars Lindqvist <lars.lindqvist at yandex.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "util.h"
/* fixar minvalens */

static void
usage(const char *prog) {
	fprintf(stdout,
		"usage: %s <-M#> -r# -k# -n# -m# [-q] [-C] [-D directory] [-o filename] [-f filename]\n"
		"	semi-mandatory arguments\n"
		"	 -r = graph uniformity\n"
		"	 -k = vertices in forbidden graph\n"
		"	 -n = vertices in input graphs\n"
		"	 -m = edges in input graphs\n"
		"	optional arguments\n"
		"    -D, output directory\n"
		"	 -o, write output to file, use `-' for stdout\n"
		"	 -f, graphs are read from given filename, rather than stdin\n"
		"	 -C, don't clobber output file\n"
		"	 -q, quiet, surppress misc output\n"
		"	input: list of edge indices with regards to K^r_n\n"
		"	output: LP constraints for expansion of input graphs to n+1 vertices\n"
		"	misc: Output directory will be choosen by:\n"
		"	      1) command line argument -D\n"
		"	      2) environment variable $GRAPH_DIR\n"
		"	      3) compile time definition OUTDIR=" OUTDIR
		"\n"
		"	      Default output filenames are `lpgraph-r=#-k=#-n=#-m=#-N=#_no=#.lp'\n"
		"	      where no=0, 1, 2, ... for the first, second, third, ... input graphs.\n"
		"	      If input filename contains `-r=#-k=#-n=#-m=#', then -r -k -n and -m\n"
		"	      can be omitted.\n", prog);

	exit(EXIT_FAILURE);
}

/* Write LP constraints of graph G, where x_i = 1 if the
   i-th edge of K_p, the complete graph on n+1 vertices,
   is in G, x_i = 0 if the i-th edge is not in G, and
   x_i is not set if the edge contains the n+1st vertex
   of K_p.
   (i is in the range [0, n+1 choose r].)
 */

static void
write_lp(Graph * g, Complete_graph * K_p, FILE * fp) {
	/* K_p = complete graph on n+1 vertices */
	vertex *edge;
	uint i, j = 0, edge_no = 0, isin;
	unsigned char *new;

	new = g_calloc(K_p->m, sizeof(unsigned char));

	/* i = edge index wrt. K_{n+1} */
	/* j = edge index wrt. K_n, as is stored in g->edges */
	/* edge_no = current edge in g */

	/* iterate over edges in K_{n+1} */
	for (edge = K_p->edges, i = 0; i < K_p->m; i++, edge += K_p->r) {

		if (vertex_is_in_edge(K_p->n, edge, K_p->r)) {
			/* edges containing the "new" vertex should be free variables */
			new[i] = 1;
			continue;
		}

		if (edge_no < g->m && j == g->edges[edge_no]) {
			isin = 1;
			edge_no++;
		} else
			isin = 0;

		fprintf(fp, " x%d = %d\n", i, isin);
		j++;
	}

	free(new);
}

static void
write_minval_lp(Graph * g, Complete_graph * K, Complete_graph * K_p, uint M, FILE * fp) {
	uint Gp_delta = M - g->m;
	uint *deg = get_vertex_degrees(g, K);
	vertex v, *edge;
	uint e, k;
	uint x = nCk(K->n - 1, K_p->r - 2);

	for (v = 0; v < K->n; v++) {
		k = 0;
		for (edge = K_p->edges, e = 0; e < K_p->m; e++, edge += K_p->r) {
			if (vertex_is_in_edge(K_p->n, edge, K_p->r) && vertex_is_in_edge(v + 1, edge, K_p->r)) {
				k++;
				fprintf(fp, " x%d %c", e, k < x ? '+' : ' ');
			}
		}
		fprintf(fp, ">= %d\n", Gp_delta - deg[v]);

	}

	free(deg);

}

int
main(int argc, char *argv[]) {
	FILE *in_fp, *out_fp;
	uint graph_no, i, n = 0, r = 0, k = 0, m = 0, M = 0, dummy = 0;
	Complete_graph *K_p, *K;
	Graph *tmp;
	int error = 0;

	init(argc, argv, "qvCr:k:n:m:o:f:D:M:");

	if (options->help)
		usage(argv[0]);

	if (options->infile)
		parse_infile(&r, &k, &n, &m, &dummy, &dummy, PFN_r | PFN_k | PFN_n | PFN_m);
	if (((!r && !(r = options->forbidden.r))
	     || (!k && !(k = options->forbidden.k))
	     || (!n && !(n = options->n))
	     || !(M = options->target_m)
	     || (!m && !(m = options->m))))
		usage(argv[0]);

	in_fp = open_infile();

	K = complete_graph(n, r);
	K_p = complete_graph(n + 1, r);

	graph_no = 0;
	while (!feof(in_fp)) {
		tmp = read_graph(K, m, in_fp);
		if (!tmp) {
			error = read_line_errno;
			break;
		}

		out_fp = open_outfile("%s/lpgraph-r=%d-k=%d-n=%d-m=%d-N=%d-M=%d_no=%d.lp",
				      options->graph_dir, r, k, n, m, n + 1, M, graph_no);
		if (!out_fp) {	/* should only happen if output file exists and noclobber is set */
			if (!options->quiet)
				infomsg("NOTICE: No output file for graph no. %d, skipping\n", graph_no + 1);
			continue;
		}

		write_minval_lp(tmp, K, K_p, M, out_fp);
		write_lp(tmp, K_p, out_fp);
		free_G(tmp);

		fputs("Binaries\n", out_fp);
		for (i = 0; i < K_p->m - 1; i++)
			fprintf(out_fp, " x%d", i);
		fprintf(out_fp, " x%d\n", i);
		fputs("End\n", out_fp);

		graph_no++;

		f_close(out_fp);
	}

	free_K(K);
	free_K(K_p);
	f_close(in_fp);

	return error;
}
