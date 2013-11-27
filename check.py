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

def x(filename):

	edges = []
	edge_regex = re.compile(r'^( R[0-9]*:|) x[0-9]* = [01]\n$')
	edge_index = re.compile(r'(.*x| = [01]\n)')

		
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
			print len(edges),filename
			break

for i in argv[1:]:
	x(i)
