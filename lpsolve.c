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

#include <math.h>
#include "gurobi_c.h"
#include "util.h"
#include <sys/types.h>
#include <time.h>
#include <sys/resource.h>

#define EXIT_UNFINISHED 2

static void
usage(const char *prog) {
	fprintf(stdout,
		"usage: %s -f filename [-T#] [-a] [-s#] [-S#] [-W#] [-p] [-D directory] [-q] [-t threads] [-C] [-o filename]\n"
		"	mandatory arguments\n"
		"    -f, linear program to solve\n"
		"   optional arguments\n"
		"	 -t, number of threads for gurobi to use\n"
		"	 -T, timelimit in minutes\n"
		"	 -a, append solutions to output file\n"
		"    -o, write output to file, use ``-'' for stdout\n"
		"	 -q, quiet, surppress misc output\n"
		"	 -s, minimum numer of solutions before quiting due to exceeding time limit\n"
		"	 -S, maximum numer of solutions, quit even if time limit has not been reached\n"
		"	 -C, don't clobber output file\n"
		"	 -W, write current state of LP back to file for every # solutions\n"
		"	 -p, turn off presolve\n"
		"	misc: Output directory will be choosen by:\n"
		"	      1) command line argument -D\n"
		"	      2) environment variable $GRAPH_DIR\n"
		"	      3) compile time definition OUTDIR=" OUTDIR "\n"
		"	      Default output filename is the input filename suffixed with ``.soln'' \n", prog);

	exit(EXIT_FAILURE);
}

static GRBenv *masterenv = NULL;
static GRBmodel *model = NULL;
static int n_vars;

static void
gurobi_err() {
	if (masterenv)
		errmsg("FATAL: gurobi: %s\n", GRBgeterrormsg(masterenv));
	else
		errmsg("FATAL: gurobi\n");

	if (model)
		GRBfreemodel(model);
	if (masterenv)
		GRBfreeenv(masterenv);

	exit(EXIT_FAILURE);
}

static void
init_gurobi(const char *path) {
	int error;
	GRBenv *env;

	error = GRBloadenv(&masterenv, NULL);
	if (error)
		gurobi_err();

	error = GRBsetintparam(masterenv, GRB_INT_PAR_OUTPUTFLAG, 0);
	if (error)
		gurobi_err();

	error = GRBreadmodel(masterenv, path, &model);
	if (error)
		gurobi_err();

	error = GRBgetintattr(model, GRB_INT_ATTR_NUMBINVARS, &n_vars);
	if (error)
		gurobi_err();

	env = GRBgetenv(model);
	if (!env)
		gurobi_err();

	if (options->threads) {
		error = GRBsetintparam(env, GRB_INT_PAR_THREADS, options->threads);
		if (error)
			gurobi_err();
	}

	if (!options->solutions_min && options->timelimit) {
		error = GRBsetdblparam(env, GRB_DBL_PAR_TIMELIMIT, options->timelimit);
		if (error)
			gurobi_err();
	}
	if (!options->presolve) {
		error = GRBsetintparam(env, GRB_INT_PAR_PRESOLVE, 0);
		if (error)
			gurobi_err();
	}

}

static int
solve() {
	int error, status;
	GRBenv *env;

	error = GRBoptimize(model);
	if (error)
		gurobi_err();

	error = GRBgetintattr(model, GRB_INT_ATTR_STATUS, &status);
	if (error)
		gurobi_err();

	if (status == GRB_INF_OR_UNBD) {
		errmsg("WARNING: Could not solve, turning off presolve and retrying\n");

		env = GRBgetenv(model);
		if (!env)
			gurobi_err();

		error = GRBsetintparam(env, GRB_INT_PAR_PRESOLVE, 0);
		if (error)
			gurobi_err();

		return solve();
	}
	return status;
}

/* Fetch solution for gurobi, each x[i] corresponds to one edge,
   x[i] = 1 if the i-th edge is part of the K-free graph, x[i] = 0
   otherwise. `val' is the number of edges in the graph.
*/
static void
get_solution(int *x, int *val) {
	int i, error;
	double sol;

	(*val) = 0;

	for (i = 0; i < n_vars; i++) {
		error = GRBgetdblattrelement(model, "X", i, &sol);
		if (error)
			gurobi_err();

		if (sol > 0.001) {
			(*val)++;
			x[i] = 1;
		} else {
			x[i] = 0;
		}
	}
}

static void
write_soln(int *x, FILE * fp) {
	int i;

	for (i = 0; i < n_vars; i++)
		if (x[i])
			fprintf(fp, "%d ", i);
	fputc('\n', fp);
}

/* Add the contraint that at least one
   of the variables x_i that are 1 be 0. */
static void
add_constraint(int *x, int m) {
	int *indices;
	double *coeffs;
	int error, i, j;

	indices = g_malloc(sizeof(int) * m);
	coeffs = g_malloc(sizeof(double) * m);

	for (i = 0, j = 0; i < n_vars; i++) {
		if (x[i]) {
			coeffs[j] = 1.0;
			indices[j] = i;
			j++;
		}
	}

	error = GRBaddconstr(model, m, indices, coeffs, GRB_LESS_EQUAL, m - 1, NULL);
	if (error)
		gurobi_err();

	free(indices);
	free(coeffs);

}

static int
time_left(time_t start_time) {
	time_t now;

	if (!options->timelimit)
		return 1;
	if ((now = time(NULL)) == -1) {
		errmsg("FATAL: time(): %s\n", strerror(errno));
		return 0;
	}
	return now - start_time < options->timelimit;
}

static void
save_state() {
	int error;
	if (!options->quiet)
		infomsg("Saving current state of LP\n");

	if (model) {
		error = GRBupdatemodel(model);
		if (error)
			gurobi_err();
		error = GRBwrite(model, options->infile);
		if (error)
			gurobi_err();
	}
}

static void
sighandler(int sig) {
	errmsg("Caught signal %d\n", sig);

	/* FIXME: Is there a way make gurobi play nice with signals? */
	/*
	   GRBterminate(model);
	   save_state();
	 */

	/* With exit() we can at least flush
	   output file on unexpected shutdown. */
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[]) {
	int *soln, status, m, retval = 0, error;
	FILE *fp;
	uint solutions = 0;
	uint time_limit_is_set = 0;
	uint writeback = 0;
	char *out_filename;
	size_t len;
	GRBenv *env;
	time_t start_time;

	init(argc, argv, "qvf:aD:o:T:t:s:S:w:p");

	if (options->help)
		usage(argv[0]);

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		if (signal(SIGINT, sighandler) == SIG_ERR)
			errmsg("ERROR: Cannot set up signal handler for SIGINT\n");
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)	/* TODO: can TERM be ignored? */
		if (signal(SIGTERM, sighandler) == SIG_ERR)
			errmsg("ERROR: Cannot set up signal handler for SIGTERM\n");
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)	/* TODO: can HUP be ignored? */
		if (signal(SIGHUP, sighandler) == SIG_ERR)
			errmsg("ERROR: Cannot set up signal handler for SIGHUP\n");

	init_gurobi(options->infile);

	start_time = time(NULL);

	len = strlen(options->infile) + strlen(".soln") + 1;
	out_filename = g_malloc(len);
	snprintf(out_filename, len, "%s.soln", options->infile);

	fp = open_outfile("%s/%s", options->graph_dir, basename(out_filename));
	if (!fp) {		/* should only happen if output file exists and noclobber is set */
		if (!options->quiet)
			infomsg("NOTICE: No output file, exiting\n");
		return 0;
	}

	free(out_filename);

	soln = g_calloc(n_vars, sizeof(int));

	while ((status = solve()) == GRB_OPTIMAL) {
		get_solution(soln, &m);
		write_soln(soln, fp);
		add_constraint(soln, m);
		solutions++;

		if (!options->quiet) {
			fprintf(stderr, ".");
			fflush(stderr);
		}

		if (options->writeback > 0 && ++writeback == options->writeback) {
			if (!options->quiet)
				fputc('\n', stderr);
			writeback = 0;
			save_state();
		}

		if (solutions >= options->solutions_min) {
			if (options->timelimit > 0 && options->solutions_min != 0	/* gurobi's time limit per soln is set in init_gurbi if !sol_min */
			    && time_limit_is_set == 0) {
				time_limit_is_set = 1;

				env = GRBgetenv(model);
				if (!env)
					gurobi_err();

				error = GRBsetdblparam(env, GRB_DBL_PAR_TIMELIMIT, options->timelimit);
				if (error)
					gurobi_err();
			}

			if (!time_left(start_time)) {
				status = GRB_TIME_LIMIT;
				break;
			}
		}

		if (solutions >= options->solutions_max && options->solutions_max > 0) {
			status = GRB_SOLUTION_LIMIT;
			break;
		}
	}

	if (solutions && !options->quiet)
		fputs("\n", stderr);	/* newline after solution dots */

	switch (status) {
	case GRB_SOLUTION_LIMIT:
		retval = EXIT_UNFINISHED;
		if (!options->quiet)
			errmsg("WARNING: Solution limit was reached, LP might have more solutions\n");
		fflush(fp);
		save_state();
		break;

	case GRB_TIME_LIMIT:
		retval = EXIT_UNFINISHED;
		if (!options->quiet)
			errmsg("WARNING: Time limit was reached, LP might have more solutions\n");
		fflush(fp);
		save_state();
		break;

	case GRB_INFEASIBLE:
	case GRB_OPTIMAL:
		retval = EXIT_SUCCESS;
		break;

	default:
		retval = EXIT_FAILURE;
		errmsg("WARNING: status = %d\n", status);

	}

	if (!options->quiet) {
		if (solutions == 1)
			infomsg("Found %u graph\n", solutions);
		else
			infomsg("Found %u graphs\n", solutions);
	}

	f_close(fp);
	free(soln);

	return retval;
}
