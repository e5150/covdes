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

#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gsl/gsl_combination.h>
#include <gsl/gsl_sf_gamma.h>

typedef uint8_t vertex;
typedef uint32_t uint;
typedef uint64_t ulong;


typedef struct Complete_graph Complete_graph;
struct Complete_graph {
	vertex n;
	uint m;
	uint r;
	vertex *edges;
	size_t edge_len;
	Complete_graph *next;
};

typedef struct Graph Graph;
struct Graph {
	uint *edges;
	uint m;
	char *s6;
	Graph *next;
};

Graph *subgraphs_on_m_edges(Complete_graph*, uint);
Graph *Galloc(vertex, uint);
Graph *complement(Graph*, Complete_graph*);
Complete_graph *Kalloc(vertex, uint);
Complete_graph *complete_graph(vertex, uint);
void free_G(Graph*);
void free_K(Complete_graph*);
void cleanup(Graph*);
void set_s6(Graph*, Complete_graph*);
Graph *isoreduce(Graph*, Complete_graph*);
Graph *read_graph(Complete_graph *, uint, FILE*);
Graph *read_graph_to_complement(Complete_graph *, uint, FILE*);
int vertex_is_in_edge(vertex, vertex*, uint);
uint *get_vertex_degrees(Graph*, Complete_graph*);
uint Delta(Graph*, Complete_graph*);
uint delta(Graph*, Complete_graph*);
Graph *covering_design(Graph*,Complete_graph*,Complete_graph*);

void printgs(Graph*, Complete_graph*);
void writegs(Graph*, Complete_graph*, FILE*);
void printg(Graph*,  Complete_graph*);
void writeg(Graph*,  Complete_graph*, FILE*);

void writeg_ei(Graph*, FILE*);
void printg_ei(Graph*);
void writegs_ei(Graph*, FILE*);
void printgs_ei(Graph*);

uint nCk(uint, uint);

#endif
