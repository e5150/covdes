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


source ./colordef.sh
echo -e "${COLOR_INFO}$0 $*${COLOR_RESET}"

if [ -f ~/.lpconfig ];then
        source ~/.lpconfig
fi
if [ -f ~/.lpconfig.sh ];then
        source ~/.lpconfig.sh
fi
if [ -f ~/.lpconfig.`hostname` ];then
        source ~/.lpconfig.`hostname`
fi
if [ -z $LPTHREADS ];then
        export LPTHREADS=1
fi
if [ -z $LPSIFTTIMEOUT ];then
        export LPSIFTTIMEOUT=30
fi
if [ -z $LPSIFTMAXRECURSE ];then
	export LPSIFTMAXRECURSE=0
fi


FILE1=$1
FILE2=$2

./sift -T$LPSIFTTIMEOUT -t$LPTHREADS $FILE1
RET1=$?
./sift -T$LPSIFTTIMEOUT -t$LPTHREADS $FILE2
RET2=$?

if [ $RET1 -eq 0 -a $RET2 -eq 0 ];then
	# both are infeasible, do nothing
	exit 0
elif [ $RET1 -ne 0 -a $RET2 -ne 0 ];then
	# both are feasible, or timed out, do nothing
	exit 0
elif [ $RET1 -eq 0 ];then
	# file1 is infeasible
	TOSPLIT=$FILE2
else
	TOSPLIT=$FILE1
fi

SPLIT=`./split.py $TOSPLIT`
RET=$?
if [ $RET -ne 0 ];then
	echo -e "${COLOR_ERROR}FATAL: Split failed${COLOR_RESET}"
	echo "cmd: ./split.py $TOSPLIT"
	echo "ret: $RET"
	echo "out: SPLIT"
	exit 1
fi

# keep filenames short
SPLIT1=`echo $SPLIT | awk '{print $1}'`
SPLIT2=`echo $SPLIT | awk '{print $2}'`
echo "mv $SPLIT1 $FILE1"
mv $SPLIT1 $FILE1
echo "mv $SPLIT2 $FILE2"
mv $SPLIT2 $FILE2


FREELEFT=`echo $SPLIT | awk '{print $3}'`
if [ ! -z $FREELEFT ];then
	if [ $FREELEFT -le 0 ];then
		echo -e "${COLOR_WARNING}Out of free variables. No more splitting.${COLOR_RESET}"
		exit 0
	fi
fi


RECURSELVL=$3
if [ -z $RECURSELVL ];then
	RECURSELVL=1
else
	RECURSELVL=$((RECURSELVL+1))
fi

if [ $LPSIFTMAXRECURSE -gt 0 -a $RECURSELVL -gt $LPSIFTMAXRECURSE ];then
	echo -e "${COLOR_WARNING}Maximum recursion depth reached. No more splitting.${COLOR_RESET}"
	exit 0
fi

exec ./sieve.sh $FILE1 $FILE2 $RECURSELVL

