#############################################################################
#
# XviD examples Makefile
#
# $Id: Makefile,v 1.3 2002-11-27 21:09:10 edgomez Exp $
#
#############################################################################

# CC=gcc

HDIR    = -I ../src
CFLAGS  = -Wall -Os
LDFLAGS = -lc -lm

SOURCES= xvid_stat.c xvid_encraw.c xvid_decraw.c
OBJECTS=$(SOURCES:.c=.o)
TESTS=$(SOURCES:.c=)

all: $(TESTS)

xvid_stat: xvid_stat.o ../build/generic/libxvidcore.a
	$(CC) $(LDFLAGS) -o $@ $< ../build/generic/libxvidcore.a

xvid_stat.o: xvid_stat.c
	$(CC) $(CFLAGS) $(HDIR) -c $<

xvid_encraw: xvid_encraw.o ../build/generic/libxvidcore.a
	$(CC) $(LDFLAGS) -o $@ $< ../build/generic/libxvidcore.a

xvid_encraw.o: xvid_encraw.c
	$(CC) $(CFLAGS) $(HDIR) -c $<

xvid_decraw: xvid_decraw.o ../build/generic/libxvidcore.a
	$(CC) $(LDFLAGS) -o $@ $< ../build/generic/libxvidcore.a

xvid_decraw.o: xvid_decraw.c
	$(CC) $(CFLAGS) $(HDIR) -c $<


clean:
	rm -f $(OBJECTS) $(TESTS)
