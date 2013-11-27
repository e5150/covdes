#!/bin/bash

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


export LD_LIBRARY_PATH=./lib

r=$1
k=$2
MAXN=$3

if [ $# -ne 3 ];then
	echo "usage: `basename $0` <r> <k> <maxn>"
	exit 1
fi

if `basename $0 | grep -q double`;then
	DUBSUF="-double"
	SEEDSIZE=2
else
	DUBSUF=""
	SEEDSIZE=1
fi

if [ -z $GRAPH_DIR ];then
	export GRAPH_DIR=/tmp/EX${DUBSUF}
fi
if [ ! -d $GRAPH_DIR ];then
	mkdir -p $GRAPH_DIR
fi

# initialize forbidden graph minus one edge
echo "./seed -C -r$r -k$k -M${SEEDSIZE}"
./seed -C -r$r -k$k -M${SEEDSIZE}
if [ $? -ne 0 ];then
	echo -e "${COLOR_ERROR}FATAL: seed did not exit cleanly ${COLOR_RESET}"
	exit 1
fi

MINN=$(ls $GRAPH_DIR/graphs-r=$r-k=$k-n=*-m=*.ei 2>/dev/null \
	| gawk -vn=$k -F'-n=' '{
	if(int($2) > n) {
		n = int($2);
	}
} END {
	printf("%d\n", n+1);
}')


for N in `seq $MINN $MAXN`;do
	n=$((N-1))

	# find upper bound, based on the largest graph available on N-1 vertices
	MAXM=$(ls $GRAPH_DIR/graphs-r=$r-k=$k-n=$n-m=*.ei 2>/dev/null \
	| gawk -vN=$N -vr=$r -F'-m=' '{
		if(int($2) > m) {
			m = int($2);
		}
	} END {
		printf("%d\n", N*m/(N-r));
	}')

	for M in `seq $MAXM -1 1`; do
		./expand_graphs-lp${DUBSUF}.sh $r $k $N $M
		RET=$?
		if [ $RET -eq 0 ];then
			# graphs were expanded, we don't need more
			break;
		fi
		if [ $RET -eq 1 ];then
			exit 1
		fi
	done
done
