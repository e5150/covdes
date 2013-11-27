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
#include "graph.h"

static void
usage(const char *prog) {
	fprintf(stdout,
		"usage: %s -r# -n# -m# [-q] [-o filename] [-a] [-C] [-D directory] [-f filename]\n"
		"	semi-mandatory arguments\n"
		"	 -r = graph uniformity\n"
		"	 -k = vertices in forbidden graph\n"
		"	 -n = vertices in input graphs\n"
		"	 -m = edges in input graphs\n"
		"	optional arguments\n"
		"	 -a, append graphs to output file\n"
		"	 -o, write output to file, use `-' for stdout\n"
		"	 -f, graphs are read from given file, rather than stdin\n"
		"	 -C, don't clobber output file\n"
		"	 -q, quiet, surppress misc output\n"
		"	input: list of edge indices with regards to K^r_n,\n"
		"	       one graph per line\n"
		"	       indicies in range [0, nCr - 1]\n"
		"	output: list of edge indicies of the non-isomorphic input graphs\n"
		"	misc: Output directory will be choosen by:\n"
		"	      1) command line argument -D\n"
		"	      2) environment variable $GRAPH_DIR\n"
		"	      3) compile time definition OUTDIR=" OUTDIR "\n"
		"	      Default output filename is `graphs-r=#-k=#-n=#-m=#.ei'\n"
		"         If input filename contains `-r=#-k=#-n=#-m=#', then -r -k -n and -m\n"
		"         can be omitted.\n", prog);

	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[]) {
	Graph *tmp, *comp, *comp_head = NULL;
	Complete_graph *K;
	FILE *in_fp, *out_fp;
	uint r = 0, k = 0, m = 0, n = 0, ngraphs, dummy;
	int error = 0;

	init(argc, argv, "qvr:k:n:m:o:aCD:f:");

	if (options->help)
		usage(argv[0]);
	if ((!(r = options->forbidden.r)
	     || !(k = options->forbidden.k)
	     || !(n = options->n)
	     || !(m = options->m))
	    && (!options->infile || !parse_infile(&r, &k, &n, &m, &dummy, &dummy, PFN_r | PFN_k | PFN_n | PFN_m)))
		usage(argv[0]);

	out_fp = open_outfile("%s/graphs-r=%d-k=%d-n=%d-m=%d.ei", options->graph_dir, r, k, n, m);
	if (!out_fp) {		/* should only happen if output file exists and noclobber is set */
		if (!options->quiet)
			infomsg("NOTICE: No output file, exiting\n");
		return 0;
	}

	K = complete_graph(n, r);

	in_fp = open_infile();

	ngraphs = 0;
	while (!feof(in_fp)) {
		comp = read_graph_to_complement(K, m, in_fp);
		if (!comp) {
			error = read_line_errno;
			break;
		}
		comp->next = comp_head;
		comp_head = comp;
		ngraphs++;
	}
	f_close(in_fp);

	if (!options->quiet)
		infomsg("Read %u graphs\n", ngraphs);

	comp_head = isoreduce(comp_head, K);

	ngraphs = 0;
	for (comp = comp_head; comp; comp = comp->next) {
		/* write edge index of original graph, not the complement */
		tmp = complement(comp, K);
		assert(tmp);
		writeg_ei(tmp, out_fp);
		free_G(tmp);
		ngraphs++;
	}
	if (!options->quiet)
		infomsg("Found %u non-isomorphic graphs\n", ngraphs);

	f_close(out_fp);

	free_K(K);
	cleanup(comp_head);
	return error;
}
