#############################################################################
#
# XviD examples Makefile
#
# $Id: Makefile,v 1.6 2003-02-22 18:54:19 chl Exp $
#
#############################################################################

# CC=gcc

HDIR    = -I ../src
CFLAGS  = -Wall -Os
LDFLAGS = -lc -lm

SOURCES= xvid_stat.c xvid_bstat.c xvid_encraw.c xvid_decraw.c 
OBJECTS=$(SOURCES:.c=.o)
TESTS=$(SOURCES:.c=)

all: $(TESTS)

xvid_stat: xvid_stat.o ../build/generic/libxvidcore.a
	$(CC) $(LDFLAGS) -o $@ $< ../build/generic/libxvidcore.a

xvid_stat.o: xvid_stat.c
	$(CC) $(CFLAGS) $(HDIR) -c $<

xvid_bstat: xvid_bstat.o ../build/generic/libxvidcore.a
	$(CC) $(LDFLAGS) -o $@ $< ../build/generic/libxvidcore.a

xvid_bstat.o: xvid_bstat.c
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
