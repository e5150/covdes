#!/bin/sh
LPTHREADS=1
LPMINSOLN=0
LPWRTBACK=0
LPTIMEOUT=20
LPMAXSOLN=1000
LPSTOP=no


case `hostname` in
tor)
	LPMAXSOLN=100
	LPTIMEOUT=600
	LPMINSOLN=1
	;;
kvaser|oden|freja)
	LPMAXSOLN=20
	LPTIMEOUT=1
	LPSTOP=yes
	;;
esac
