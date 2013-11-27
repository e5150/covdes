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

from sys import argv, stdin,stdout
import string, getopt, os, re
from itertools import combinations as nCk

if os.path.basename(argv[0]) == 'verifydoublecover.py':
	Lambda = 2
else:
	Lambda = 1


r='r'
k='k'
n='n'
m='m'
t='t'

def parse_fn(s):
	toks = re.split("[-.]", s)
	ret = { t : 0, r : 0, k : 0, n : 0, m : 0 }
	for tok in toks:
		if tok[1] == '=':
			ret[tok[0]] = int(tok[2:])
	return ret



for fn in argv[1:]:
	try:
		g = parse_fn(fn)
	except:
		print 'cannot parse filename %s' % fn
		continue

	t_subsets = [set(i) for i in nCk(range(1, 1 + g[n]), g[t])]
	CD = []
	covering = []

	f = open(fn)
	i = 0
	for line in f.readlines():
		if(line.startswith('---')):
			for t_set in t_subsets:
				good = 0
				for block in covering:
					if t_set.issubset(block):
						good += 1
				if good < Lambda:
					print 'bad: %s: t-set: %s missing from design no: %d' % (fn, tuple(t_set), i+1)
					break
			i += 1
			covering = []
		else:
			covering.append(set(map(int, line[:-1].strip().split(' '))))

	f.close()
