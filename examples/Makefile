#############################################################################
#
# XviD examples Makefile
#
# $Id: Makefile,v 1.1 2002-09-28 14:52:20 edgomez Exp $
#
#############################################################################

# CC=gcc

HDIR    = -I ../sr
CFLAGS  = -Wall -Os
LDFLAGS = -lxvidcore -lc -lm

SOURCES= xvid_stat.c xvid_encraw.c xvid_decraw.c
OBJECTS=$(SOURCES:.c=.o)
TESTS=$(SOURCES:.c=)

all: $(TESTS)

xvid_stat: xvid_stat.o
	$(CC) $(LDFLAGS) -o $@ $<

xvid_stat.o: xvid_stat.c
	$(CC) $(CFLAGS) $(HDIR) -c $<

xvid_encraw: xvid_encraw.o
	$(CC) $(LDFLAGS) -o $@ $<

xvid_encraw.o: xvid_encraw.c
	$(CC) $(CFLAGS) $(HDIR) -c $<

xvid_decraw: xvid_decraw.o
	$(CC) $(LDFLAGS) -o $@ $<

xvid_decraw.o: xvid_decraw.c
	$(CC) $(CFLAGS) $(HDIR) -c $<


clean:
	rm -f $(OBJECTS) $(TESTS)
