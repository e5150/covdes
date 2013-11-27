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

options_t _options;
const options_t *options = (const options_t *)&_options;
static char prg_invoc_short_name[PATH_MAX];

uint
nCk(uint n, uint k) {
	gsl_sf_result res;
	uint ret;

	gsl_sf_choose_e(n, k, &res);
	ret = (uint) lround(res.val);
	if (res.val > UINT_MAX) {
		errmsg("FATAL: Precision loss: %u choose %u = %lf != %u\n", n, k, res.val, ret);
		exit(1);
	}
	return ret;
}

FILE *
f_open(const char *path, const char *mode) {
	FILE *fp;

	fp = fopen(path, mode);
	if (!fp) {
		errmsg("ERROR: fopen: %s: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	return fp;
}

void
f_close(FILE * fp) {
	if (fp == stdin || fp == stdout)
		return;

	if (fclose(fp) == EOF)
		errmsg("ERROR: fclose: %s\n", strerror(errno));
}

FILE *
open_infile() {
	if (_options.infile == NULL)
		return stdin;
	else
		return f_open(_options.infile, "r");
}

void *
g_malloc(size_t size) {
	void *ret = NULL;

	ret = malloc(size);
	if (ret == NULL) {
		errmsg("FATAL: could not allocate memory\n");
		exit(EXIT_FAILURE);
	}
	return ret;
}

void *
g_calloc(size_t nmemb, size_t size) {
	void *ret = NULL;

	ret = calloc(nmemb, size);
	if (ret == NULL) {
		errmsg("FATAL: could not allocate memory\n");
		exit(EXIT_FAILURE);
	}
	return ret;
}

void *
g_realloc(void *ret, size_t size) {
	ret = realloc(ret, size);
	if (ret == NULL) {
		errmsg("FATAL: could not allocate memory\n");
		exit(EXIT_FAILURE);
	}
	return ret;
}

static int
file_exists(char *path) {
	int ret;

	ret = access(path, F_OK);
	return ret == -1 ? 0 : 1;
}

void
errmsg(const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "%s: ", prg_invoc_short_name);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void
infomsg(const char *fmt, ...) {
	va_list args;

	va_start(args, fmt);
	fprintf(stdout, "%s: ", prg_invoc_short_name);
	vfprintf(stdout, fmt, args);
	va_end(args);
}

FILE *
open_outfile(const char *fmt, ...) {
	va_list args;

	if (_options.use_default_outfile) {
		va_start(args, fmt);
		vsnprintf(_options.outfile, PATH_MAX - 1, fmt, args);
		va_end(args);
	} else if (_options.outfile[0] == '-') {
		if (!_options.quiet)
			infomsg("Writing output to: stdout\n");
		return stdout;
	}

	/* Don't write to existing files unless append is set */
	if (_options.noclobber && !_options.append && file_exists(_options.outfile)) {
		if (!_options.quiet)
			errmsg("WARNING: Will not clobber existing %s\n", _options.outfile);
		/* Return NULL here and let caller take appropriate action */
		return NULL;
	}
	if (!_options.quiet) {
		infomsg("%s output to: %s\n", (_options.append ? "Appending" : "Writing"), _options.outfile);
	}
	return f_open(_options.outfile, _options.append ? "a" : "w");
}

/* read one line from file, regardless of length */
char *
read_line(FILE * fp) {
	size_t bufsize = BUFSIZ;
	size_t bytes = 0;
	size_t last = 0;
	char *buf = NULL;
	read_line_errno = 1;

	buf = g_malloc(BUFSIZ);

	while (!feof(fp)) {
		memset(buf + last, '\0', BUFSIZ);
		fgets(buf + last, BUFSIZ, fp);
		bytes = strlen(buf + last);

		if (!bytes || buf[last + bytes - 1] == '\n')
			break;	/* end of line, break and return */

		last += BUFSIZ - 1;
		bufsize = last + BUFSIZ;
		buf = (char *)g_realloc(buf, bufsize);
	}
	if (feof(fp) && buf && buf[0] == '\0')
		read_line_errno = 0;	/* only read null-terminator of last line */
	return buf;
}

void
init(int argc, char *argv[], const char *args) {
	int opt;
	int err;

	_options.infile = NULL;
	_options.graph_dir = NULL;
	_options.presolve = 1;
	_options.use_default_outfile = 1;
	_options.timelimit = 0;
	_options.threads = 0;
	_options.forbidden.r =
	 _options.solutions_min =
	 _options.solutions_max =
	 _options.forbidden.k =
	 _options.forbidden.m =
	 _options.n =
	 _options.m = _options.help = _options.quiet = _options.noclobber = _options.target_n = _options.target_m = 0;

	while (-1 != (opt = getopt(argc, argv, args))) {
		switch (opt) {
		case 'D':
			_options.graph_dir = optarg;
			break;
		case 'C':
			_options.noclobber = 1;
			break;
		case 'q':
			_options.quiet = 1;
			break;
		case 'v':
			/* _options.verbose = 1; */
			break;
		case 'a':
			_options.append = 1;
			break;
		case 'h':
			_options.help = 1;
			break;
		case 'o':
			strncpy(_options.outfile, optarg, PATH_MAX - 1);
			_options.use_default_outfile = 0;
			break;
		case 'r':
			_options.forbidden.r = atoi(optarg);
			break;
		case 'k':
			_options.forbidden.k = atoi(optarg);
			break;
		case 'n':
			_options.n = atoi(optarg);
			break;
		case 'm':
			_options.m = atoi(optarg);
			break;
		case 'N':
			_options.target_n = atoi(optarg);
			break;
		case 'M':
			_options.target_m = atoi(optarg);
			break;
		case 'f':
			_options.infile = optarg;
			break;
		case 'T':
			_options.timelimit = 60 * atoi(optarg);
			break;
		case 't':
			_options.threads = atoi(optarg);
			break;
		case 's':
			_options.solutions_min = atoi(optarg);
			break;
		case 'S':
			_options.solutions_max = atoi(optarg);
			break;
		case 'w':
			_options.writeback = atoi(optarg);
			break;
		case 'p':
			_options.presolve = 0;
			break;
		default:
			_options.help = 1;
		}
	}

	/* treat extraneous argument as input filename */
	if (optind < argc && _options.infile == NULL)
		_options.infile = argv[optind];

	snprintf(prg_invoc_short_name, PATH_MAX - 1, "%s", argv[0]);

	err = 0;
	if (_options.solutions_min && _options.solutions_max
	    && (err += _options.solutions_min > _options.solutions_max))
		errmsg("FATAL: s > S, s=%u, S=%u\n", _options.solutions_min, _options.solutions_max);
	if (_options.forbidden.r && _options.forbidden.k && (err += _options.forbidden.r > _options.forbidden.k))
		errmsg("FATAL: r > k, r=%u k=%u\n", _options.forbidden.r, _options.forbidden.k);
	if (_options.forbidden.k && _options.n && (err += _options.forbidden.k > _options.n))
		errmsg("FATAL: k > n, k=%u n=%u\n", _options.forbidden.k, _options.n);
	if (_options.n && _options.target_n && (err += (_options.n + 1 != _options.target_n)))
		errmsg("FATAL: n+1 != N, n=%u N=%u\n", _options.n, _options.target_n);
	if (err)
		exit(EXIT_FAILURE);

	if (_options.forbidden.k && _options.forbidden.r) {
		_options.forbidden.m = nCk(_options.forbidden.k, _options.forbidden.r);
	}

	if (!_options.graph_dir) {
		_options.graph_dir = getenv("GRAPH_DIR");
		if (!_options.graph_dir)
			_options.graph_dir = OUTDIR;
	}

}

int
parse_infile(uint * r, uint * k, uint * n, uint * m, uint * N, uint * M, uint flags) {
	int ret = 1;
	char *c;

	if ((flags & PFN_r))
		if (!(c = strstr(options->infile, "-r=")) || sscanf(c, "-r=%u", r) != 1)
			ret = 0;
	if ((flags & PFN_k))
		if (!(c = strstr(options->infile, "-k=")) || sscanf(c, "-k=%u", k) != 1)
			ret = 0;
	if ((flags & PFN_n))
		if (!(c = strstr(options->infile, "-n=")) || sscanf(c, "-n=%u", n) != 1)
			ret = 0;
	if ((flags & PFN_m))
		if (!(c = strstr(options->infile, "-m=")) || sscanf(c, "-m=%u", m) != 1)
			ret = 0;
	if ((flags & PFN_N))
		if (!(c = strstr(options->infile, "-N=")) || sscanf(c, "-N=%u", N) != 1)
			ret = 0;
	if ((flags & PFN_M))
		if (!(c = strstr(options->infile, "-M=")) || sscanf(c, "-M=%u", M) != 1)
			ret = 0;
	return ret;
}
