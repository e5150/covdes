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

#ifndef UTIL_H
#define UTIL_H

#ifndef OUTDIR
#define OUTDIR "."
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgen.h>
#include <ctype.h>
#include "graph.h"
#include <limits.h>

#ifndef PATH_MAX
	#define PATH_MAX 4096
#endif

#define PFN_r (1<<0)
#define PFN_k (1<<1)
#define PFN_n (1<<2)
#define PFN_m (1<<3)
#define PFN_N (1<<4)
#define PFN_M (1<<5)

typedef struct{
	struct {
		uint r;
		uint m;
		vertex k;
		size_t edge_len;
	} forbidden;

	uint n;
	uint m;
	uint target_m;
	uint target_n;
	uint noclobber;
	uint append;
	uint help;
	uint timelimit;
	uint solutions_min;
	uint solutions_max;
	int threads;
	uint writeback;
	uint presolve;

	uint quiet;
	const char *infile;
	const char *graph_dir;

	unsigned char use_default_outfile;
	char outfile[PATH_MAX];
} options_t;

extern const options_t *options;

uint nCk(uint, uint);
void init(int, char**, const char*);
int read_line_errno;

int parse_infile(uint*, uint*, uint*, uint*, uint*, uint*, uint);
FILE *f_open(const char*, const char*);
FILE *open_infile();
FILE *open_outfile(const char*, ...);
void errmsg(const char*, ...);
void infomsg(const char*, ...);
void f_close(FILE*);
void args_set_forbidden();
void *g_malloc(size_t);
void *g_calloc(size_t, size_t);
void *g_realloc(void*, size_t);
char *read_line(FILE*);


#endif
