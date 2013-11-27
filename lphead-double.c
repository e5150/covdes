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

static void
usage(const char *prog) {
	fprintf(stdout,
		"usage: %s -r# -k# -N# -M# [-q] [-o filename] [-C] [-D directory]\n"
		"	mandatory arguments\n"
		"	 -r = uniformity of graphs\n"
		"	 -k = vertices in forbidden graph\n"
		"	 -N = vertices in target graphs\n"
		"	 -M = edges in target graph\n"
		"	optional arguments\n"
		"	 -o, write output to file, use `-' for stdout\n"
		"	 -C, don't clobber output file(s)\n"
		"	 -q, quiet, surppress misc output\n"
		"	output: LP constraints for exclusion of K^r_k in graphs on N vertices\n"
		"	misc: Output directory will be choosen by:\n"
		"	      1) command line argument -D\n"
		"	      2) environment variable $GRAPH_DIR\n"
		"	      3) compile time definition OUTDIR=" OUTDIR "\n"
		"	      Default output filename is `lphead-r=#-k=#-N=#-M=#.lp'\n", prog);

	exit(EXIT_FAILURE);
}

/* check if vertices in `edge' is a subset of vertices in `comb' */
static unsigned char
is_subset(vertex * edge, size_t * comb, uint r, uint k) {
	uint i, j, match = 0;
	for (i = 0; i < k; i++)
		for (j = 0; j < r; j++)
			if (comb[i] == edge[j])
				if (++match == r)
					return 1;
	return 0;
}

static unsigned char
vertex_is_in_K(vertex v, size_t * comb, uint k) {
	uint i;
	for (i = 0; i < k; i++)
		if (comb[i] == v)
			return 1;
	return 0;
}

int
main(int argc, char *argv[]) {
	gsl_combination *comb;
	Complete_graph *K;
	FILE *fp;
	uint i, N, M, k, r, m, edgelimit, first;
	vertex *edge;

	init(argc, argv, "qvr:k:N:M:o:CD:");

	if (options->help || !(r = options->forbidden.r)
	    || !(k = options->forbidden.k)
	    || !(N = options->target_n)
	    || !(M = options->target_m)) {
		usage(argv[0]);
	}

	fp = open_outfile("%s/lphead-r=%d-k=%d-N=%d-M=%d.lp", options->graph_dir, r, k, N, M);
	if (!fp) {		/* should only happen if output file exists and noclobber is set */
		if (!options->quiet)
			infomsg("NOTICE: No output file, exiting\n");
		return 0;
	}

	/* number of potential edges in r-graph in N vertices */
	m = nCk(N, r);
	edgelimit = nCk(k, r) - 2;
	K = complete_graph(N, r);

	fputs("Maximize\n", fp);
	for (i = 0; i < m - 1; i++)
		fprintf(fp, " x%d +", i);
	fprintf(fp, " x%d\n", i);

	fputs("Subject to\n", fp);

	/* sum of edges */
	for (i = 0; i < m - 1; i++)
		fprintf(fp, " x%d +", i);
	fprintf(fp, " x%d = %d\n", i, M);

	/* foreach k-subset of vertices */
	comb = gsl_combination_calloc(N, k);
	do {

		/* Skip complete graphs that does not contain the new vertex,
		 * since all graphs should already have been verified to not
		 * have these as subgraphs. */
		if (!vertex_is_in_K(N - 1, comb->data, k))
			continue;

		/* foreach possible edge */
		first = 1;
		for (edge = K->edges, i = 0; i < K->m; i++, edge += K->r) {
			if (is_subset(edge, comb->data, r, k)) {
				if (first) {
					fprintf(fp, " x%d", i);
					first = 0;
				} else {
					fprintf(fp, " + x%d", i);
				}
			}
		}
		fprintf(fp, " <= %u\n", edgelimit);
	}
	while (GSL_SUCCESS == gsl_combination_next(comb));

	gsl_combination_free(comb);

	f_close(fp);

	free_K(K);

	return 0;
}
