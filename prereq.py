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

r = -1
k = -1
N = -1
M = -1

r,k,N,M = map(int, argv[1:])

class prereq:
	def __init__(self, n, m):
		self.n = n-1
		self.m =(m - r*m/n, m)

	def __repr__(self):
		return "n =%3d,  %3d <= m <= %3d" % (self.n, self.m[0], self.m[1])


n = N-1
PRE = [prereq(N,M)]
while n>k:
	PRE.append(prereq(n, PRE[-1].m[0]))
	n -=1

for i in PRE: print i
