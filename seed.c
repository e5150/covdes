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

#include "graph.h"
#include "util.h"

static void
usage(const char *prog) {
	fprintf(stdout,
		"usage: %s -r# -k# -M# [-q] [-o filename] [-D directory] [-C]\n"
		"	mandatory arguments\n"
		"	 -r = graph uniformity\n"
		"	 -k = vertices in forbidden graph\n"
		"	 -M, number of edges to remove from K^r_k\n"
		"	optional arguments\n"
		"	 -q, quiet, surppress misc output\n"
		"	 -o, write output to file, use `-' for stdout\n"
		"	 -C, don't clobber output file\n"
		"	output: Non-isomorphic subgraphs of K^r_k\n"
		"	  if -M is given, then only subgraphs with nCk - M edges\n"
		"	  will be produced. Otherwise output will include every\n"
		"	  subgraph of K^r_k\n"
		"	  format: list of edge indices with regards to K^r_k,\n"
		"	          one graph per line\n"
		"	          indicies in range [0, nCk - 1]\n"
		"	misc: Output directory will be choosen by:\n"
		"	      1) command line argument -D\n"
		"	      2) environment variable $GRAPH_DIR\n"
		"	      3) compile time definition OUTDIR="
		OUTDIR "\n" "	      Default output filename is `graphs-r=#-k=#-n=#-m=#.ei'\n", prog);

	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[]) {
	Complete_graph *K;
	Graph *head;
	uint m, k, r, minm, i, j;
	FILE *fp;
	Graph *forbidden, *tmp, *comp;
	gsl_combination *comb;

	init(argc, argv, "r:k:o:D:CM:vq");
	if (!options->forbidden.m || options->help || !(k = options->forbidden.k) || !(r = options->forbidden.r)) {
		usage(argv[0]);
	}

	K = complete_graph(k, r);

	if (options->target_m == 1) {	/* There is only one graph unique graph on one edge less than the complete graph */
		fp = open_outfile("%s/graphs-r=%d-k=%d-n=%d-m=%d.ei", options->graph_dir, r, k, k, K->m - 1);
		if (!fp) {
			if (!options->quiet)
				infomsg("NOTICE: No output file, exiting\n");
			free_K(K);
			return 0;
		}

		/* print all but one edge indices from the complete graph */
		for (i = 0; i < K->m - 1; i++)
			fprintf(fp, "%u ", i);
		fprintf(fp, "\n");

		f_close(fp);
	} else if (options->target_m >= 2) {
		m = K->m - options->target_m;

		fp = open_outfile("%s/graphs-r=%d-k=%d-n=%d-m=%d.ei", options->graph_dir, r, k, k, m);

		if (!fp) {	/* should only happen if output file exists and noclobber is set */
			if (!options->quiet)
				infomsg("NOTICE: No output file, skipping\n");
			free_K(K);
			return 0;
		}

		/* Dummy forbidden graph, unique K - e */
		forbidden = Galloc(K->n, K->m - 1);
		if (!forbidden) {
			errmsg("CANTHAPPEN: Galloc(K->n, K->m - 1) == NULL\n");
			assert(forbidden);
		}

		j = 0;
		head = NULL;
		/* M = options->target_m is the number of edges to remove from K */
		/* Choose M-1 edges from K-e to construct the complement of every
		   possible subgraph of K - e with M edges */
		comb = gsl_combination_calloc(forbidden->m, options->target_m - 1);

		do {
			comp = Galloc(K->n, options->target_m);
			if (!comp) {
				errmsg("CANTHAPPEN: Galloc complement failed\n");
				assert(comp);
			}
			for (i = 0; i < options->target_m - 1; i++)
				comp->edges[i] = gsl_combination_get(comb, i);

			/* add the edge not in K - e to all complemented subgraphs */
			comp->edges[i] = forbidden->m;
			comp->next = head;
			head = comp;
			j++;
		} while (GSL_SUCCESS == gsl_combination_next(comb));

		gsl_combination_free(comb);

		head = isoreduce(head, K);

		j = 0;
		for (comp = head; comp; comp = comp->next) {
			tmp = complement(comp, K);
			if (!tmp) {
				errmsg("CANTHAPPEN: complement(comp, K) == NULL\n");
				writeg(comp, K, stderr);
				assert(tmp);
			}
			writeg_ei(tmp, fp);
			free_G(tmp);
			j++;
		}

		f_close(fp);
		cleanup(head);
		free_G(forbidden);
	} else {
		if (options->target_m)
			minm = K->m - options->target_m;
		else
			minm = 1;

		for (m = minm; m < K->m; m++) {

			if (options->target_m && m != minm)
				break;

			fp = open_outfile("%s/graphs-r=%d-k=%d-n=%d-m=%d.ei", options->graph_dir, r, k, k, m);
			if (!fp) {	/* should only happen if output file exists and noclobber is set */
				if (!options->quiet)
					infomsg("NOTICE: No output file, skipping\n");
				continue;
			}

			head = subgraphs_on_m_edges(K, m);
			writegs_ei(head, fp);

			f_close(fp);
			cleanup(head);

			if (minm != 1)
				break;
		}
	}

	free_K(K);
	return 0;
}
