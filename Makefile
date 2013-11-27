CFLAGS+=-g -O3 --std=c99 -Wall -Wextra -W -pedantic -D_XOPEN_SOURCE=600 -I./include
LDFLAGS=-lgsl -lgslcblas -lm -L./lib
CC=gcc

LIBSRC=graph.c util.c
LIBOBJ=${LIBSRC:.c=.o}
HDR=${LIBSRC:.c=.h}
PRGSRC=seed.c ei2s6.c isoreduce.c lphead.c lphead-double.c nCk.c lpgraph.c ei2graph.c ei2cd.c sift.c lpsolve.c
PRGOBJ=${PRGSRC:.c=.o}
PRGEXE=${PRGSRC:.c=}

%.o : %.c ${HDR}
	${CC} -c ${CFLAGS} $< -o $@

all: ${LIBOBJ} ${PRGOBJ} ${PRGEXE}

sift: sift.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS} -lgurobi45 -lpthread -lm

lpsolve: lpsolve.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS} -lgurobi45 -lpthread -lm

seed: seed.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS} 

ei2s6: ei2s6.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS} 

isoreduce: isoreduce.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS} 

lphead: lphead.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS} 

lphead-double: lphead-double.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS}  

nCk: nCk.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS}  

lpgraph: lpgraph.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS}  

ei2graph: ei2graph.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS}  

ei2cd: ei2cd.o ${LIBOBJ} ${HDR}
	${CC} -o $@ $@.o ${LIBOBJ} ${LDFLAGS}  

clean:
	rm -f ${LIBOBJ} ${PRGOBJ} ${PRGEXE}
