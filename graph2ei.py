#!/usr/bin/python

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

from sys import argv, stdin
import string, getopt, os, re
from itertools import combinations as nCk


r='r'
k='k'
n='n'
m='m'

def parse_fn(s):
	toks = re.split("[-.]", s)
	ret = { r : 0, k : 0, n : 0, m : 0 }
	for tok in toks:
		if tok[1] == '=':
			ret[tok[0]] = int(tok[2:])
	return ret


def f1(N, edge):
	K = range(1, N+1)
	for v in edge:
		K.pop(K.index(v))
	return tuple(K)



for fn in argv[1:]:
	g = parse_fn(fn)
	K = [i for i in nCk(range(1, 1+g[n]), g[r])]

	G = []
	f = open(fn)
	graph = []
	for line in f.readlines():
		if(line.startswith('---')):
			G.append(graph)
			graph = []
			print
		else:
			edge = tuple(map(int, line[:-1].strip().split(' ')))
			print K.index(edge),
	f.close()
