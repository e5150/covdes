#!/usr/bin/python
from sys import argv,stdin,stdout,stderr

m = 0
c = stdin.readlines()
for line in c:
	m = max(m, line.count(' '))
for line in c:
	if line.count(' ') == m:
		stdout.write("%s" % line)

stderr.write('m=%d\n' % m);
