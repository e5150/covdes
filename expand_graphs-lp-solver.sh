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

LPFILE=$1

export LD_LIBRARY_PATH=./lib

source ./colordef.sh

echo -e "${COLOR_INFO}$0 $*${COLOR_RESET}"

if [ -z $1 ];then
	echo -e "${COLOR_ERROR}usage: `basename $0` <linearprog.lp>${COLOR_RESET}"
	exit 1
fi
if [ ! -f $1 ];then
	echo -e "${COLOR_ERROR}FATAL: $LPFILE no such file${COLOR_RESET}"
	exit 1
fi
if [ -z "$GRAPH_DIR" ];then
	echo -e "${COLOR_ERROR}FATAL: GRAPH_DIR is not set${COLOR_RESET}"
	exit 1
fi
if [ ! -d "$GRAPH_DIR" ];then
	echo -e "${COLOR_ERROR}FATAL: GRAPH_DIR=$GRAPH_DIR is not a directory${COLOR_RESET}"
	exit 1
fi

if [ -f ~/.lpconfig ];then
	source ~/.lpconfig
fi

if [ -f ~/.lpconfig.sh ];then
	source ~/.lpconfig.sh
fi

if [ -f ~/.lpconfig.`hostname` ];then
	source ~/.lpconfig.`hostname`
fi

if [ -z $LPTIMEOUT ];then
	export LPTIMEOUT=120
fi
if [ -z $LPTHREADS ];then
	export LPTHREADS=1
fi
if [ -z $LPMINSOLN ];then
	export LPMINSOLN=0
fi
if [ -z $LPMAXSOLN ];then
	export LPMAXSOLN=1000
fi
if [ -z $LPWRTBACK ];then
	export LPWRTBACK=0
fi
if [ -z $LPSIFTTIMEOUT ];then
	export LPSIFTTIMEOUT=30
fi
if [ -z $LPOVERWRITEOLDSOLN ];then
	LPMVARG="-i"
elif [ "$LPOVERWRITEOLDSOLN" = "no" ];then
	LPMVARG="-i"
fi


# try to solve linear program
LPSOLUN=${LPFILE}.soln
date
echo -e "${COLOR_INFO}./lpsolve -av -T$LPTIMEOUT -t$LPTHREADS -s$LPMINSOLN -S$LPMAXSOLN -w$LPWRTBACK ${COLOR_RESET}"
./lpsolve -av -T$LPTIMEOUT -t$LPTHREADS -s$LPMINSOLN -S$LPMAXSOLN $LPFILE -o$LPSOLUN -w$LPWRTBACK
RETVAL=$?


cleanup() {
	if [ ! -d $GRAPH_DIR/_solutions ];then
		mkdir -p $GRAPH_DIR/_solutions
	fi

	rm $LPFILE

	# don't keep zero byte files
	find $LPSOLUN -empty -delete
	if [ ! -f $LPSOLUN ];then
		return
	fi

	gzip -f $LPSOLUN
	if [ -e $GRAPH_DIR/_solutions/`basename ${LPSOLUN}.gz` ];then
		echo -e "${COLOR_ERROR}Already exists: $GRAPH_DIR/_solutions/`basename ${LPSOLUN}.gz`${COLOR_RESET}"
	fi
	mv $LPMVARG ${LPSOLUN}.gz $GRAPH_DIR/_solutions/
}

if [ $RETVAL -eq 0 ];then
	echo -e "${COLOR_GOOD}LP has been solved${COLOR_RESET}"

	cleanup
	exit 0

elif [ $RETVAL -eq 2 ];then
	echo -e "${COLOR_WARNING}Limit reached for LP, splitting${COLOR_RESET}"
	SPLITLP=`./split.py $LPFILE`
	RET=$?
	if [ $RET -ne 0 ];then
		echo -e "${COLOR_ERROR}FATAL: Split failed${COLOR_RESET}"
		echo "cmd: ./split.py $TOSPLIT"
		echo "ret: $RET"
		echo "out: SPLIT"
		exit 1
	fi

	cleanup

	SPLIT1=`echo $SPLITLP | awk '{print $1}'`
	SPLIT2=`echo $SPLITLP | awk '{print $2}'`

	./sieve.sh $SPLIT1 $SPLIT2 1
	RET=$?
	if [ $RET -ne 0 ];then
		exit 1
	fi

	if [ "x$LPSTOP" = "xyes" ];then
		echo -e "${COLOR_WARNING}LPSTOP = yes, stopping on request${COLOR_RESET}"
		exit 2
	fi

	# solve the simplified programs
	$0 $SPLIT1
	RET=$?
	if [ $RET -ne 0 ];then
		exit 1
	fi
	exec $0 $SPLIT2

else
	echo -e "${COLOR_ERROR}FATAL: lpsolve returned $RETVAL when trying to solve $LPFILE${COLOR_RESET}"
	exit 1
fi
