
#
# This Makefile assumes that PF_RING is installed as a binary package: see http://packages.ntop.org 
#
#
# DNA Support
DNA_DEFINE = #-DENABLE_DNA_SUPPORT

#
# PF_RING aware libpcap
#
O_FLAG     = -O2 -DHAVE_PF_RING

#
# MONGODB
#
MONGODIR  = /usr/local/include/libmongoc-1.0
BSONDIR   = /usr/local/include/libbson-1.0
MONGOLIBDIR = /usr/local/lib
# Search directories
#
INCLUDE    =  -I${MONGODIR} -I${BSONDIR} 

#
# C compiler and flags
#
#
# CROSS_COMPILE=arm-mv5sft-linux-gnueabi-
#-std=gnu9
CC         = ${CROSS_COMPILE}gcc #--platform=native
CFLAGS     =  ${O_FLAG} -Wall -std=gnu99 ${INCLUDE} ${DNA_DEFINE} -D HAVE_ZERO -D ENABLE_BPF -D HAVE_LIBNUMA -D HAVE_PTHREAD_SETAFFINITY_NP -O2  -L${MONGOLIBDIR}# -g
# LDFLAGS  =

#
# User and System libraries
#
LIBS       = -lpcap -lpthread -lpfring  -lrt   -lnuma -lrt -lmongoc-1.0 -lbson-1.0

# How to make an object file
%.o: %.c 
	@echo "=*= making object $@ =*="
	${CC} ${CFLAGS} -c $< -o $@

#
# Main targets
#
PFPROGS   = amon

TARGETS   = ${PFPROGS} 

all: ${TARGETS}

amon:  bm_structs.o amon.o 
	${CC} ${CFLAGS} amon.o  bm_structs.o ${LIBS} -o $@

clean:
	@rm -f ${TARGETS} *.o *~
