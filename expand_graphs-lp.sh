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

r=$1 # uniformity
k=$2 # veritices in forbidden graph
N=$3 # target vertices
M=$4 # target edges
n=$((N-1))

export LD_LIBRARY_PATH=./lib

source ./colordef.sh

if [ $# -ne 4 ];then
	echo -e "usage: `basename $0` <r> <k> <N> <M>"
	exit 1
fi
if [ -f $GRAPH_DIR/dontexist-$N-$M ];then
	echo -e "${COLOR_WARNING}No expansions were possible for N=$N M=$M ${COLOR_RESET}"
	exit 2
fi

if `basename $0 | grep -q double`;then
	DUBSUF="-double"
else
	DUBSUF=""
fi


mkdir -p $GRAPH_DIR/_helpers \
         $GRAPH_DIR/_solutions


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
	else
		# Recursively run _this_script_ until we have
		# all smaller graphs that we need.
		$0 $r $k $n $minm
		RET=$?
		if [ $RET -ne 0 ];then
			echo -e "${COLOR_ERROR}FATAL: $0 $r $k $n $minm did not exit clearly${COLOR_RESET}"
			exit 1
		fi
	fi
fi


LPHEAD=$GRAPH_DIR/_helpers/lphead${DUBSUF}-r=${r}-k=${k}-N=${N}-M=${M}.lp
if ! [ -f ${LPHEAD}.gz ];then
	./lphead${DUBSUF} -qC -r$r -k$k -N$N -M$M -o- | gzip > ${LPHEAD}.gz
	RET=$?
	if [ $RET -ne 0 ];then
		echo -e "${COLOR_ERROR}FATAL: ./lphead${DUBSUF} -qC -r$r -k$k -N$N -M$M -o-${COLOR_RESET}"
		echo -e "${COLOR_ERROR}ret: $RET"
		exit 1
	fi
fi

UNSOLVED=0

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
	for LPGRAPH in `./lpgraph -M${M} -v $graph -D$GRAPH_DIR/_helpers \
		| grep Writing \
		| cut -d: -f3`
	do
		LPFILE=$GRAPH_DIR/solve${DUBSUF}-r=$r-k=$k-n=$n-m=$m-N=$N-M=${M}_no=$i.lp

		if [ ! -f $LPGRAPH ];then
			echo -e "${COLOR_ERROR}FATAL: $LPGRAPH missing${COLOR_RESET}"
			echo -e "${COLOR_ERROR}./lpgraph -M${M} -v $graph -D$GRAPH_DIR/_helpers${COLOR_RESET}"
			echo -e "${COLOR_ERROR}should have produced this file.${COLOR_RESET}"
			exit 1
		fi

		# Create LP.
		cat ${LPHEAD}.gz > ${LPFILE}.gz
		cat ${LPGRAPH} | gzip >> ${LPFILE}.gz
		rm ${LPGRAPH}

		./expand_graphs-lp-solver.sh ${LPFILE}.gz
		RET=$?
		
		if [ $RET -eq 2 ];then
			# LPSTOP=yes
			((UNSOLVED++))
		elif [ $RET -ne 0 ];then
			echo -e "${COLOR_ERROR}FATAL: ./expand_graphs-lp-solver.sh ${LPFILE}.gz did not exit cleanly${COLOR_RESET}"
			exit 1
		fi

		((i++))
	done
done

if [ $UNSOLVED -gt 0 ];then
	echo -e "${COLOR_ERROR}WARNING: $UNSOLVED unsolved files seems to remain"
	ls -1 $GRAPH_DIR/solve${DUBSUF}-r=$r-k=$k-n=$n-m=*-N=$N-M=${M}_no*
	echo -ne "${COLOR_RESET}"
	if [ "x$LPSTOP" = "xyes" ];then
		echo -e "${COLOR_WARNING}Since LPSTOP=yes this is somewhat expected${COLOR_RESET}"
	fi
	echo -e "${COLOR_ERROR}Will not try to do isomorphism reduction${COLOR_RESET}"
	exit 1
fi

if ! which shortg > /dev/null 2>&1;then
	PATH=$PATH:.
fi

FILES=`find $GRAPH_DIR/_solutions | grep "N=${N}-M=${M}" | wc -l`

if [ $FILES -gt 0 ]
then
	echo -e "${COLOR_INFO} piping all solutions for N=$N M=$M to ./isoreduce -v -r$r -k$k -n$N -m$M -o${TARGET_GRAPHS}${COLOR_RESET}"
	find $GRAPH_DIR/_solutions/ \
		| grep "N=${N}-M=${M}" \
		| xargs zcat \
		| ./isoreduce -v -r$r -k$k -n$N -m$M -o$TARGET_GRAPHS
	RET=${PIPESTATUS[*]}
	if [ "$RET" != "0 0 0 0" ];then
		echo -e "${COLOR_ERROR}isoreduce did not exit cleanly${COLOR_RESET}"
		echo -e "${COLOR_ERROR}exit statuses: $RET (find grep xargs isoreduce)${COLOR_RESET}"
		exit 1
	fi
fi

find $GRAPH_DIR/_solutions/ \
	| grep "N=${N}-M=${M}" \
	| xargs rm

find $TARGET_GRAPHS -empty -delete 2>/dev/null

if [ -f $TARGET_GRAPHS ];then
	echo -e "${COLOR_GOOD}All non-isomorphic K_$k-free $r-graphs on $N vertices and $M edges have been found${COLOR_RESET}"
	exit 0
else
	echo -e "${COLOR_WARNING}No expansions were possible for N=$N M=$M ${COLOR_RESET}"
	echo "No expansions were possible for N=$N M=$M" > $GRAPH_DIR/dontexist-$N-$M
	exit 2
fi
