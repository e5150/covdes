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
		"usage: %s -r# -k# -n# -m# [-o filename] [-q] [-a] [-C] [-D directory] [-f filename]\n"
		"	semi-mandatory arguments\n"
		"	 -r = graph uniformity\n"
		"	 -k = vertices in forbidden graph\n"
		"	 -n = vertices in input graphs\n"
		"	 -m = edges in input graphs\n"
		"	optional arguments\n"
		"	 -q, quiet, surppress misc output\n"
		"	 -C, don't clobber output file\n"
		"	 -a, append graphs to output file\n"
		"	 -D, output directory\n"
		"	 -o, write output to file, use `-' for stdout\n"
		"	 -f, graphs are read from given filename, rather than stdin\n"
		"	input: List of edge indices with regards to K^r_n.\n"
		"	output: List of vertices in edges, one edge per line.\n"
		"	misc: Output directory will be choosen by:\n"
		"	      1) command line argument -D\n"
		"	      2) environment variable $GRAPH_DIR\n"
		"	      3) compile time definition OUTDIR="
		OUTDIR "\n"
		"	      Default output filename is `graphs-r=#-k=#-n=#-m=#.txt'\n"
		"	      -r, -k, -n and -m may be omitted if input filename\n"
		"	      contains `-r=#-k=#-n=#-m=#'\n", prog);

	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[]) {
	FILE *in_fp, *out_fp;
	uint n = 0, r = 0, k = 0, m = 0, dummy;
	Complete_graph *K;
	Graph *tmp;
	int error = 0;

	init(argc, argv, "r:k:qn:m:ao:f:D:Cv");

	if (options->help)
		usage(argv[0]);
	if ((!(r = options->forbidden.r)
	     || !(k = options->forbidden.k)
	     || !(n = options->n)
	     || !(m = options->m))
	    && (!options->infile || !parse_infile(&r, &k, &n, &m, &dummy, &dummy, PFN_r | PFN_k | PFN_n | PFN_m))) {
		usage(argv[0]);
	}

	out_fp = open_outfile("%s/graphs-r=%d-k=%d-n=%d-m=%d.txt", options->graph_dir, r, k, n, m);
	if (!out_fp) {		/* should only happen if output file exists and noclobber is set */
		if (!options->quiet)
			infomsg("NOTICE: No output file, exiting\n");
		return 0;
	}

	K = complete_graph(n, r);
	in_fp = open_infile();
	while (!feof(in_fp)) {
		tmp = read_graph(K, m, in_fp);
		if (!tmp) {
			error = read_line_errno;
			break;
		}
		writeg(tmp, K, out_fp);
		free_G(tmp);
	}
	f_close(out_fp);
	f_close(in_fp);
	free_K(K);

	return error;
}
