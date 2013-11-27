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
		"usage: %s <-f filename> [-T seconds] [-t threads]\n"
		"    -f, linear program to solve\n"
		"    -t, number of threads for gurobi to use (default: 1)\n"
		"    -T, timelimit in seconds (default: 30)\n", prog);

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

	error = GRBsetintparam(env, GRB_INT_PAR_THREADS, options->threads ? options->threads : 1);
	if (error)
		gurobi_err();

	if (options->timelimit)
		error = GRBsetdblparam(env, GRB_DBL_PAR_TIMELIMIT, options->timelimit ? options->timelimit / 60.0 : 30);
	if (error)
		gurobi_err();

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

int
main(int argc, char *argv[]) {
	int status, retval = 0;
	GRBenv *env;

	init(argc, argv, "f:T:t:");

	if (options->help)
		usage(argv[0]);

	init_gurobi(options->infile);

	env = GRBgetenv(model);
	if (!env)
		gurobi_err();

	status = solve();

	switch (status) {
	case GRB_TIME_LIMIT:
		fprintf(stdout, "Timed out: %s\n", options->infile);
		retval = EXIT_FAILURE;
		break;

	case GRB_OPTIMAL:
		fprintf(stdout, "Has solutions: %s\n", options->infile);
		retval = EXIT_FAILURE;
		break;

	case GRB_INFEASIBLE:
		fprintf(stdout, "Infeasible: %s\n", options->infile);
		retval = EXIT_SUCCESS;
		break;

	default:
		retval = EXIT_FAILURE;
		errmsg("WARNING: status = %d\n", status);

	}

	return retval;
}
