#!/usr/bin/env python

# Copyright © 2011,2012,2013 Lars Lindqvist <lars.lindqvist at yandex.ru>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the “Software”),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import gzip, os, re, string, itertools, sys
argv = sys.argv
argc = len(argv)
from subprocess import *

# Splitta .lp-fil n nivaer. t.ex. 
# ./split.py solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-01010011101.lp 4
# ger:
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010000.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010001.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010010.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010011.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010100.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010101.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010111.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011000.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011001.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011010.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011011.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011100.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011101.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011111.lp

n_splits = 1

def errmsg(str):
	sys.stderr.write(std + '\n')

def usage():
	errmsg("usage: %s <filename> [number of splits]" % argv[0])
	exit(1)

if(argc > 1):
	if(argc == 3):
		n_splits = int(argv[2])
	filename = argv[1]
	if not os.path.isfile(filename):
		errmsg("no such file %s" % filename)
		usage()
else:
	usage()


edges = []
edge_regex = re.compile(r'^( R[0-9]*:|) x[0-9]* = [01]\n$')
edge_index = re.compile(r'(.*x| = [01]\n)')

	
def make_filenames():
	tmp = re.compile(r'.*no=[0-9]*\.lp*')
	if filename.endswith('.gz'):
		suffix = ".lp.gz"
	else:
		suffix = ".lp"
	
	if tmp.match(filename):
		infix = '-'
	else:
		infix= ''

	x = re.split('\.lp', filename)
	assert(len(x) == 2)

	fns = []
	for i in itertools.product('01',repeat=n_splits):
		fns.append(x[0] + infix + string.join(i,'') + suffix)
	return fns

outfiles = []
for i in make_filenames():
	if os.path.isfile(i):
		errmsg("split.py: %s already exists" % i)
		exit(1)
	if i.endswith('.gz'):
		outfiles.append(gzip.open(i, 'wb', 6))
	else:
		outfiles.append(open(i, 'w'))

# read output from zcat rather than reading gz-file ourselves, this way
# we can read one line at a time without being clever with read().
if filename.endswith('.gz'):
	cmd = Popen('zcat "%s"' % filename,shell=True, stdout=PIPE)
else:
	cmd = Popen('cat "%s"' % filename,shell=True, stdout=PIPE)

lastvar = 0
added = False
for line in cmd.stdout.readlines():
	if(edge_regex.match(line)):
		x = edge_index.split(line)
		if len(x) != 5 or not x[2].isdigit():
			errmsg("Can't continue, possibly bad LP constraint: %s" % line)
			exit(1)
		edges.append(int(x[2]))

	elif line in ('Bounds\n', 'Binaries\n') and not added:
		added = True
		found = 0
		i = 0
		free_edges = []
		while found < n_splits:
			if i not in edges:
				free_edges.append(i)
				found += 1
			i += 1

		i = 0
		for edgevals in itertools.product('01',repeat=n_splits):
			for edge_n in range(len(free_edges)):
				outfiles[i].write(' x%d = %s\n' % (free_edges[edge_n], edgevals[edge_n]))
			i += 1
	elif added and not line.startswith('End') and not line.startswith('Binaries'):
		try: lastvar = int(line.split()[-1][1:]) + 1
		except: pass

	for outfile in outfiles:
		outfile.write(line)

		
for f in outfiles:
	print f.name,
	f.close()

if lastvar != 0:
	print lastvar - (len(edges) + n_splits)
else:
	print (len(edges) + n_splits)
