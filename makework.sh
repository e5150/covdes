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

r=$1
k=$2
N=$3
M=$4
n=$((N-1))

export LD_LIBRARY_PATH=./lib

source ./colordef.sh

if `basename $0 | grep -q double`;then
	DUBSUF="-double"
else
	DUBSUF=""
fi


if [ $# -ne 4 ];then
	echo -e "usage: `basename $0` <r> <k> <N> <M>"
	exit 1
fi

mkdir -p $GRAPH_DIR/_helpers \
         $GRAPH_DIR/_solutions \
         $GRAPH_DIR/work


echo -e "${COLOR_INFO}$0 $*${COLOR_RESET}"


# Assume that all r-graphs on N vertices and M edges
# have already been found if target file exists.
TARGET_GRAPHS=$GRAPH_DIR/graphs-r=$r-k=$k-n=$N-m=$M.ei
if [ -f $TARGET_GRAPHS ];then
	echo "$TARGET_GRAPHS already exists, skipping"
	exit 0
fi


# To find K^r_k-free r-graphs on N vertices and M edges we need all K-free
# r-graphs on n = N-1 vertices and at least minm = M - floor(Mr / N) edges.
minm=$((M - M*r/N))

# Check for the required graphs
MISSING=$((M+1))
for edges in `seq $minm $M`;do
		if [ ! -f $GRAPH_DIR/graphs-r=$r-k=$k-n=$n-m=$edges.ei ];then
				MISSING=$edges
				break;
		fi
done
if [ $MISSING -eq $minm ];then
	# We'll have to create the required smaller graphs.

	echo -e "${COLOR_WARNING}We need graphs on $n verices and $minm edges${COLOR_RESET}"
	if [ $n -eq $k ];then
		# No expansion needed for graphs on the same number
		# of vertices as the forbidden graph.
		MAXM=`./nCk $k $r`
		echo "./seed -C -r$r -k$k -M$((MAXM-$minm))"
		./seed -C -r$r -k$k -M$((MAXM-$minm))
		if [ $? -ne 0 ];then
			echo -e "${COLOR_ERROR}FATAL: seed did not exit cleanly ${COLOR_RESET}"
			exit 1
		fi
	else
		exec $0 $r $k $n $minm
	fi
fi


LPHEAD=$GRAPH_DIR/_helpers/lphead${DUBSUF}-r=${r}-k=${k}-N=${N}-M=${M}.lp
if ! [ -f ${LPHEAD} ];then
	./lphead${DUBSUF} -qC -r$r -k$k -N$N -M$M -o${LPHEAD}
fi


# Expand all graphs where n = $n and m \in [$minm, $M]
for m in `seq $M -1 $minm`; do

	# Graphs to expand.
	graph=$GRAPH_DIR/graphs-r=$r-k=$k-n=$n-m=$m.ei
	if [ ! -f $graph ];then
		continue
	fi
	echo "expanding $graph"

	# Each graph*.ei file may contain several graphs,
	# we get one set on LP constraints from each graph.
	i=0
	./lpgraph -v -M${M} $graph -D$GRAPH_DIR/_helpers \
		| grep Writing \
		| cut -d: -f3 \
		| while read LPGRAPH; do
		if [ ! -f $LPGRAPH ];then
			echo -e "${COLOR_WARNING}WARNING $LPGRAPH missing${COLOR_RESET}"
			echo -e "${COLOR_ERROR}press return to continue${COLOR_RESET}"
			read junk
			((i++))
			continue
		fi

		LPFILE=$GRAPH_DIR/work/solve${DUBSUF}-r=$r-k=$k-n=$n-m=$m-N=$N-M=${M}_no=$i.lp.gz
		if [ -f $LPFILE ];then
			echo -e "${COLOR_INFO}already exists: ${LPFILE}${COLOR_RESET}"
			((i++))
			rm $LPGRAPH
			continue
		fi

		cat $LPHEAD $LPGRAPH | gzip > $LPFILE

		if [ $? -ne 0 ];then
			echo -e "${COLOR_ERROR}ERROR: $LPFILE ${COLOR_RESET}"
			echo -e "${COLOR_ERROR}press return to continue${COLOR_RESET}"
			read junk
		else
			echo -e "${COLOR_GOOD}created ${LPFILE}${COLOR_RESET}"
		fi

		rm $LPGRAPH
		((i++))
	done
done
