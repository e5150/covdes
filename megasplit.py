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

import gzip, os, re, string, itertools
from sys import argv
argc = len(argv)
from subprocess import *

#Splitta bara ut nollorna (uteslutna kanter)
#t.ex.
#./megasplit.py solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-01010011101.lp 10
#ger:
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111010.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-0101001110110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-01010011101110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-0101001110111110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-01010011101111110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011111110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-0101001110111111110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-01010011101111111110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011111111110.lp
#solve-r=4-k=6-n=9-m=108-N=10-M=176_no=0-010100111011111111111.lp


n_splits = 1

def usage():
	print "usage: %s <filename> [number of splits]" % argv[0]
	exit(1)

if(argc > 1):
	if(argc == 3):
		n_splits = int(argv[2])
	filename = argv[1]
	if not os.path.isfile(filename):
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
	spt = ''
	for i in range(n_splits):
		fns.append(x[0] + infix + spt + '0' + suffix)
		spt = spt + '1'
	fns.append(x[0] + infix + spt + suffix)
	return fns

outfiles = []
for i in make_filenames():
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

added = False
for line in cmd.stdout.readlines():
	if(edge_regex.match(line)):
		x = edge_index.split(line)
		if len(x) != 5 or not x[2].isdigit():
			print "can't continue, possibly bad LP constraint:",line,
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

		print len(edges),free_edges
		for i in range(n_splits):
			outfiles[i].write(' x%d = 0\n' % free_edges[i])
			for j in range(i+1,n_splits+1):
				outfiles[j].write(' x%d = 1\n' % free_edges[i])

	for outfile in outfiles:
		outfile.write(line)

		
for f in outfiles:
	print f.name
	f.close()
