#/**************************************************************************
# *
# * linux makefile
# *
# * nasm args:
# * -f fmt		elf (linux), coff (djgpp)
# * -DPREFIX	add '_' prefix to symbol names
# *
# * libso:
# * (-lm not neccessary if using -ffast-math)
# *
# *************************************************************************/


include Makefile.inc

CC = gcc
#CC=icc
#CC = /opt/experimental/bin/gcc 	# that's where SuSE installs gcc3

CFLAGS = -DARCH_X86 -DLINUX
LIBDIR = /usr/local/lib

ifeq ($(CC),icc)
    CFLAGS+= -O3 -ip -tpp6 -xM  		# -tpp7 might be faster (P4 optmization)...
    CFLAGS+= -I/opt/intel/compiler50/ia32/include -I/opt/intel/compiler50/ia32/substitute_headers/

else

# CFLAGS += -funroll-loops -ffast-math -fstrict-aliasing -fomit-frame-pointer
# CFLAGS += -fPIC
# CFLAGS += -m486
# CFLAGS += -march=pentium -mcpu=pentium
 CFLAGS += -march=pentiumpro -mcpu=pentiumpro
# CFLAGS += -march=k6 -mcpu=k6

    CFLAGS += -Wall -O3 -funroll-loops -ffast-math
    CFLAGS += -fstrict-aliasing -fomit-frame-pointer 
endif

AS = nasm
AFLAGS = -f elf

%.o: %.asm
	$(AS) $(AFLAGS) $< -o $@

RM = rm -rf


all: $(LIB) $(LIBSO)


$(LIB): $(SRC:.c=.o) $(SRC_INTEL:.asm=.o)
	ar rcs $@ $^


$(LIBSO): $(SRC:.c=.o) $(SRC_INTEL:.asm=.o)
	$(CC) $(CFLAGS) $^ -shared -lc -lm -o $@


clean:	
	$(RM) `find $(SRCDIR) -name "*.o"`

remove-all:
	$(RM) $(DIRS)
	$(RM) $(DIRS_INTEL)
	$(RM) $(LIB)
	$(RM) $(LIBSO)


install: libxvidcore.so
	cp libxvidcore.so $(LIBDIR)			# you have to bee root for this
	/sbin/ldconfig 

install-test: libxvidcore.so				# if you don't want to overwrite previous compile
	cp libxvidcore.so $(LIBDIR)/libtestcore.so
	/sbin/ldconfig
