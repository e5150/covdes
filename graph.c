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
#ifndef SHORTG_BIN
#define SHORTG_BIN "./shortg"
#endif

/* Write edge indices of graph to file, indices are with regards to
   lexicographical ordering of edges in the complete graph on the
   same number of vertices.
   Indices are separated by space.
   Exactly one graph per line.
   Indices are in the range [0, n choose r]
 */
void
writeg_ei(Graph * g, FILE * fp) {
	uint i;

	for (i = 0; i < g->m; i++)
		fprintf(fp, "%u ", g->edges[i]);
	fputc('\n', fp);
}

/* Write graphs to file.
   r vertices per edge, separated by space.
   One edge per line.
   Graph ends with "---" on a separate line.
   Vertices are in the range [1, n]
*/
void
writeg(Graph * g, Complete_graph * K, FILE * fp) {
	uint i, j;

	for (i = 0; i < g->m; i++) {
		for (j = 0; j < K->r; j++)
			fprintf(fp, "%u ", 1 + K->edges[g->edges[i] * K->r + j]);
		fprintf(fp, "\n");
	}
	fprintf(fp, "---\n");
}

void
writegs(Graph * g, Complete_graph * K, FILE * fp) {
	Graph *tmp;
	for (tmp = g; tmp; tmp = tmp->next)
		writeg(tmp, K, fp);
}

void
writegs_ei(Graph * g, FILE * fp) {
	Graph *tmp;
	for (tmp = g; tmp; tmp = tmp->next)
		writeg_ei(tmp, fp);
}

void
printg(Graph * g, Complete_graph * K) {
	writeg(g, K, stdout);
	fflush(stdout);
}

void
printgs(Graph * g, Complete_graph * K) {
	writegs(g, K, stdout);
}

void
printg_ei(Graph * g) {
	writeg_ei(g, stdout);
	fflush(stdout);
}

void
printgs_ei(Graph * g) {
	writegs_ei(g, stdout);
}

int
vertex_is_in_edge(vertex n, vertex * edge, uint r) {
	uint i;
	for (i = 0; i < r; i++)
		if (edge[i] == n - 1)
			return 1;
	return 0;
}

/* smallest valency of graph g */
uint
delta(Graph * g, Complete_graph * K) {
	uint min = UINT_MAX, *deg;
	vertex i;

	deg = get_vertex_degrees(g, K);
	for (i = 0; i < K->n; i++)
		if (deg[i] < min)
			min = deg[i];
	free(deg);

	return min;
}

/* largest valency of graph g */
uint
Delta(Graph * g, Complete_graph * K) {
	uint max = 0, *deg;
	vertex i;

	deg = get_vertex_degrees(g, K);
	for (i = 0; i < K->n; i++)
		if (deg[i] > max)
			max = deg[i];
	free(deg);

	return max;
}

/* return list of vertex valencies */
uint *
get_vertex_degrees(Graph * g, Complete_graph * K) {
	uint *deg, i, j;
	vertex *edge;

	deg = g_calloc(K->n, sizeof(uint));

	for (i = 0; i < g->m; i++) {
		edge = K->edges + (g->edges[i] * K->r);

		for (j = 0; j < K->r; j++)
			deg[edge[j]]++;
	}
	return deg;
}

Complete_graph *
Kalloc(vertex n, uint r) {
	Complete_graph *K;

	K = g_malloc(sizeof(Complete_graph));
	K->n = n;
	K->r = r;
	K->m = nCk((uint) n, r);
	K->edge_len = r * sizeof(vertex);
	K->edges = g_malloc(K->m * K->edge_len);

	return K;
}

Graph *
Galloc(vertex n, uint m) {
	Graph *tmp;
	(void)n;		/* gcc warning */

	tmp = g_malloc(sizeof(Graph));
	tmp->edges = g_malloc(m * sizeof(uint));
	tmp->m = m;
	tmp->next = NULL;

	return tmp;
}

void
free_K(Complete_graph * G) {
	if (G == NULL) {
		errmsg("CANTHAPPEN: Freeing NULL, Complete_graph\n");
		return;
	}
	if (G->edges == NULL) {
		errmsg("CANTHAPPEN: Freeing NULL, Complete_graph->edges\n");
		return;
	}
	free(G->edges);
	free(G);
}

void
free_G(Graph * G) {
	if (G == NULL) {
		errmsg("CANTHAPPEN: Freeing NULL, Graph\n");
		return;
	}
	if (G->edges == NULL) {
		errmsg("CANTHAPPEN: Freeing NULL, Graph->edges\n");
		return;
	}
	free(G->edges);
	free(G);
}

static unsigned char
edge_is_in_graph(uint edge, Graph * g) {
	uint i;

	for (i = 0; i < g->m; i++) {
		if (edge == g->edges[i])
			return 1;
	}
	return 0;
}

static uint
edge_index(Complete_graph * K, vertex * edge2) {
	uint i;
	vertex *edge1;

	for (edge1 = K->edges, i = 0; i < K->m; edge1 += K->r, i++) {
		if (!memcmp(edge1, edge2, K->edge_len))
			return i;
	}
	return -1;
}

static vertex *
invert_edge(vertex * edge, vertex n, uint r) {
	vertex *ret;
	uint i, j;

	ret = g_malloc((n - r) * sizeof(vertex));

	for (j = i = 0; i < n; i++) {
		/* vertex_is_in_edge expects vertex in range [1, n] */
		if (!vertex_is_in_edge(i + 1, edge, r)) {
			ret[j++] = i;
		}
	}
	return ret;
}

Graph *
complement(Graph * g, Complete_graph * K) {
	Graph *ret = NULL;
	uint e, i;

	ret = Galloc(K->n, K->m - g->m);
	for (e = i = 0; e < K->m; e++) {
		if (!edge_is_in_graph(e, g))
			ret->edges[i++] = e;
	}

	return ret;
}

Graph *
covering_design(Graph * g, Complete_graph * Kg, Complete_graph * Kd) {
	uint *g_complement;
	uint e, i, gci, gi;
	Graph *ret;
	uint blocks;
	int block_i;
	vertex *edge;
	vertex *block;

	blocks = Kg->m - g->m;

	ret = Galloc(Kg->n, blocks);
	g_complement = g_malloc(blocks * sizeof(uint));
	for (gci = gi = e = 0; e < Kg->m; e++) {
		if (!edge_is_in_graph(e, g))
			g_complement[gci++] = e;

	}

	for (i = 0; i < blocks; i++) {
		edge = Kg->edges + Kg->r * g_complement[i];
		block = invert_edge(edge, Kg->n, Kg->r);

		if ((block_i = edge_index(Kd, block)) == -1) {
			return NULL;
		} else {
			ret->edges[i] = block_i;
		}
		free(block);
	}

	free(g_complement);
	return ret;
}

/* Remove all graphs from linked list */
void
cleanup(Graph * head) {
	Graph *tmp, *tmp2;

	/* find first graph to keep */
	tmp = head;
	while (tmp) {
		tmp2 = tmp->next;
		free_G(tmp);
		tmp = tmp2;
	}

}

void
set_s6(Graph * g, Complete_graph * K) {
	uint n, byte, i, j, k, l, vs = 0, carrybits = 0;
	ulong y = 0, carry = 0;
	int nbits;
	size_t len;
	vertex *edge;

	/* number of vertices in Levi graph */
	n = K->n + g->m;

	/* number of bits needed to represent n-1 */
	k = 0;
	for (i = n - 1; i; i = i >> 1)
		k++;

	/* number of bytes needed for s6-representation of graph */
	len = 2			/* first byte: ':' + last byte: '\0' */
	 + (n < 63 ? 1 : 4)	/* representation of n */
	 +(k + 1) / 6.0		/* k+1 bits per "information-unit", 6 bits per byte */
	 * (n			/* one "movement" per vertex */
	    + K->r * g->m	/* edges */
	    + (((uint) K->n == g->m)	/* qlique of vertices */
	       ? (K->n - 1) * K->n / 2 : 0)
	    /*+ 1  just in case */
	 );

	g->s6 = g_malloc(len * sizeof(char));
	g->s6[0] = ':';
	g->s6[len - 1] = 0;

	if (n < 63) {
		g->s6[1] = 63 + n;
		byte = 2;
	} else {
		byte = 1;
		g->s6[byte++] = 126;
		g->s6[byte++] = 63 + (n >> 12);
		g->s6[byte++] = 63 + ((n >> 6) & 63);
		g->s6[byte++] = 63 + (n & 63);
	}

#define SETBITS(BITS_X) \
	y = (carry << (k + 1)) /* "b" == 0 */ | (BITS_X); \
	for(nbits = carrybits + k - 5; nbits >= 0; nbits -= 6) \
		g->s6[byte++] = 63 + ((y  >> (nbits)) & 63); \
	carrybits = nbits + 6; \
	carry = y & ((1 << carrybits) - 1); \


	/* vertices-proper, make a clique of them in case
	   the number of vertices and edges are equal,
	   to avoid false positive isomorphisms */
	if ((uint) K->n == g->m) {
		for (i = 1; i < K->n; i++) {
			SETBITS(i);
			for (l = 0; l < i; l++) {
				SETBITS(l);
			}
			vs++;
		}
	}

	/* edges-proper */
	for (i = 0; i < g->m; i++) {
		/* "goto" vertex g->n+i (edge-proper i) */
		SETBITS(K->n + i);

		edge = K->edges + (g->edges[i] * K->r);
		for (j = 0; j < K->r; j++) {
			/* make adjacent to vertex-proper */
			SETBITS(edge[j]);
		}
		vs++;
	}

#undef SETBITS

	if (carrybits) {	/* paddningsvilkor enligt ntos6() i gtools.c */
		if (6 - carrybits > k && vs == n - 2 && n == (uint) 1 << k)
			g->s6[byte++] = ((carry << (6 - carrybits)) | ((uint) 63 >> (carrybits + 1))) + 63;
		else
			g->s6[byte++] = ((carry << (6 - carrybits)) | ((uint) 63 >> carrybits)) + 63;
	}

	for ( /*sic! */ ; byte < len; byte++)
		g->s6[byte] = 0;

}

/* Remove superfluous graphs from linked list */
Graph *
isoreduce(Graph * head, Complete_graph * K) {
	Graph *tmp, *newhead = NULL;
	int i, j, out = 0, in = 0;
	unsigned char *keep;
	char *buf = NULL, *c;
	int w_pipe[2] = { -1, -1 }, r_pipe[2] = {
	-1, -1};
	pid_t pid;
	FILE *w_fp, *r_fp;
	Graph **glist;
	int status = 0;

	if (!head)
		return NULL;

	if (pipe(r_pipe) == -1 || pipe(w_pipe) == -1) {
		errmsg("FATAL: pipe(): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ((pid = fork()) == -1) {
		errmsg("FATAL: fork(): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	} else if (pid == 0) {	/* child, run nauty */
		close(w_pipe[1]);
		close(r_pipe[0]);

		if (0 > dup2(w_pipe[0], STDIN_FILENO)
		    || 0 > dup2(r_pipe[1], STDERR_FILENO))
			/* sic!, the information we want is written to stderr by nauty */
		{
			errmsg("FATAL: dup2(): %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		close(w_pipe[0]);
		close(r_pipe[1]);

		if (execlp(SHORTG_BIN, SHORTG_BIN, "-quvk", (char *)NULL) == -1) {
			errmsg("FATAL: execlp " SHORTG_BIN " -quvk: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	} else {		/* parent */
		close(w_pipe[0]);
		close(r_pipe[1]);

		if (!(w_fp = fdopen(w_pipe[1], "w"))) {
			errmsg("FATAL: fdopen nauty input: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* write s6 to nauty */
		for (tmp = head; tmp; tmp = tmp->next) {
			set_s6(tmp, K);

			fputs(tmp->s6, w_fp);
			fputc('\n', w_fp);

			free(tmp->s6);

			out++;
		}
		fclose(w_fp);

		/* Nauty will give us a list of indices of the first graph in 
		   any isomorphism class. Indices are with regards to the order
		   in which the graph appear in the output we just wrote,
		   they are in the range [1, number of graphs] */

		glist = g_malloc(out * sizeof(Graph *));
		keep = g_calloc(out, sizeof(unsigned char));
		for (i = 0, tmp = head; tmp; tmp = tmp->next, i++) {
			glist[i] = tmp;
		}
		assert(out == i);

		if (!(r_fp = fdopen(r_pipe[0], "r"))) {
			errmsg("FATAL: fdopen nauty output: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		while (!feof(r_fp)) {
			buf = read_line(r_fp);

			if ((c = strchr(buf, ':'))) {
				c++;
				j = atoi(c) - 1;
				if (j < 0 || j >= out) {
					errmsg("CANTHAPPEN: j = %d from nauty output: %s\n", j, buf);
					exit(EXIT_FAILURE);
				}
				keep[j] = 1;
				in++;
			}
			free(buf);

		}
		fclose(r_fp);

		newhead = NULL;
		for (j = 0; j < out; j++) {
			if (keep[j]) {
				tmp = glist[j];
				tmp->next = newhead;
				newhead = tmp;
			} else {
				free_G(glist[j]);
			}
		}

		free(keep);
		free(glist);

		waitpid(pid, &status, 0);
		if (0 != WEXITSTATUS(status)) {
			errmsg("FATAL: exec shortg: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		return newhead;
	}
}

/* Construct complete r-graph on n vertices,
   fill edges with vertices in range [0, n-1] in lexicographical order */
Complete_graph *
complete_graph(vertex n, uint r) {
	Complete_graph *K;
	gsl_combination *comb;
	vertex *e;
	uint i, j;

	K = Kalloc(n, r);
	comb = gsl_combination_calloc(n, r);
	for (e = K->edges, i = 0; i < K->m; e += r, i++) {
		for (j = 0; j < r; j++)
			e[j] = gsl_combination_get(comb, j);
		gsl_combination_next(comb);
	}

	gsl_combination_free(comb);

	return K;
}

/* return pointer to next number in str */
static char *
next_tok(char *str) {
	char *ret = str;
	while (isdigit(ret[0]))
		ret++;
	while (ret[0] == ' ' || ret[0] == '\t')
		ret++;
	if (!ret || ret[0] == '\0' || ret[0] == '\n' || ret[0] == '\t')
		return NULL;
	else
		return ret;
}

/* Convert char* of edge indices separated by
   whitespaces to edge indices in a Graph* */
int
str2graph(Complete_graph * K, Graph * g, char *str) {
	uint m, ei;
	char *p = str;
	unsigned char *edges;

	if (!str) {
		errmsg("CANTHAPPEN: str2graph *str is NULL, aborting\n");
		abort();
	}

	edges = g_calloc(K->m, sizeof(unsigned char));

	for (m = 0; m < g->m; m++) {

		if (!isdigit(p[0])) {
			errmsg("ERROR: invalid symbol: '%c' (chr: %d) expected digit\n", p[0], p[0]);
			free(edges);
			return -1;
		}

		ei = atoi(p);

		if (ei >= K->m) {
			errmsg("ERROR: edge index to large: %d, there are only "
			       "%d possible edges in %d-graphs on %d vertices\n", ei, K->m, K->r, K->n);
			free(edges);
			return -1;
		}

		if (edges[ei]) {
			errmsg("ERROR: duplicate edge index: %d\n", ei);
			free(edges);
			return -1;
		}
		edges[ei] = 1;

		g->edges[m] = ei;

		p = next_tok(p);
		if (!p)
			break;
	}
	free(edges);

	if (m >= g->m) {
		errmsg("ERROR: too many edges, should have %d\n", g->m);
		return -1;
	} else if (m + 1 < g->m) {
		errmsg("ERROR: too few edges, %d, should be %d\n", m + 1, g->m);
		return -1;
	}
	return 0;

}

Graph *
read_graph(Complete_graph * K, uint m, FILE * fp) {
	char *buf = NULL;
	Graph *tmp = NULL;

	if (feof(fp)) {
		return NULL;
	}

	buf = read_line(fp);
	if (buf == NULL) {
		return NULL;
	}
	if (buf[0] == '\0') {
		free(buf);
		return NULL;
	}

	tmp = Galloc(K->n, m);

	if (str2graph(K, tmp, buf) == -1) {
		errmsg("ERROR, corrupt graph near: %s\n", buf);
		printf("WARNING\n");
		free(buf);
		free_G(tmp);

		return NULL;
	}

	free(buf);
	return tmp;
}

Graph *
read_graph_to_complement(Complete_graph * K, uint m, FILE * fp) {
	Graph *comp = NULL;
	Graph *tmp = NULL;

	tmp = read_graph(K, m, fp);
	if (!tmp)
		return NULL;
	comp = complement(tmp, K);
	free_G(tmp);
	return comp;
}

/* Return likned list of all non-isomorphic m-sized subgraphs of K */
Graph *
subgraphs_on_m_edges(Complete_graph * K, uint m) {
	Graph *tmp, *head = NULL;
	uint i;
	gsl_combination *comb;

	comb = gsl_combination_calloc(K->m, m);

	do {
		tmp = Galloc(K->n, m);
		for (i = 0; i < m; i++)
			tmp->edges[i] = gsl_combination_get(comb, i);
		tmp->next = head;
		head = tmp;
	}
	while (GSL_SUCCESS == gsl_combination_next(comb));

	gsl_combination_free(comb);

	return isoreduce(head, K);
}
