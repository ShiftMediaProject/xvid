##############################################################################
#
# - Unified Makefile for XviD for *nix environments -
#
# Copyright(C) 2003 Edouard Gomez <ed.gomez@free.fr>
#
#
# Description:
#  This Makefile allows building XviD sources to obtain a shared library
#  and a static library. This Makefie uses variables defined in the
#  platform.inc file. This platform.inc file is usually created by the
#  ./configure script whenever a unix shell is available.
#
# Nota Bene for mingw32/djgpp users:
#   However if you provide a platform.inc file adapted to your OS, then
#   change the include platform.inc to include yourfile.inc and it should
#   work too.
#
#
##############################################################################

include sources.inc
include platform.inc

RM = rm -rf

##############################################################################
#
# Build rules
#
##############################################################################

# Our main targets
STATIC_LIB=libxvidcore.$(STATIC_EXTENSION)
SHARED_LIB=libxvidcore.$(SHARED_EXTENSION)

# Their Objects
OBJECTS=$(GENERIC_OBJECTS)
OBJECTS+=$(DIVX4COMPAT_OBJECTS)
OBJECTS+=$(ASSEMBLY_OBJECTS)

#-----------------------------------------------------------------------------
# The default rule
#-----------------------------------------------------------------------------

.SUFFIXES: .$(OBJECT_EXTENSION) .$(ASSEMBLY_EXTENSION) .c

all: platform.inc $(STATIC_LIB) $(SHARED_LIB)

#-----------------------------------------------------------------------------
# Generic assembly rule
#-----------------------------------------------------------------------------

.$(ASSEMBLY_EXTENSION).$(OBJECT_EXTENSION):
	$(AS) $(AFLAGS) $< -o $@

#-----------------------------------------------------------------------------
# Generic C rule
#-----------------------------------------------------------------------------

.c.$(OBJECT_EXTENSION):
	$(CC) -c $(CFLAGS) $< -o $@

#-----------------------------------------------------------------------------
# Static Library
#-----------------------------------------------------------------------------

$(STATIC_LIB): $(OBJECTS)
	ar rc $(STATIC_LIB) $(OBJECTS)

#-----------------------------------------------------------------------------
# Shared Library
#-----------------------------------------------------------------------------

$(SHARED_LIB): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(SHARED_LIB) $(OS_LDFLAGS)

#-----------------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------------

install: all
	@echo
	@echo "+---------- Installing XviD libraries in $(libdir) ----------+"
	@echo
	$(INSTALL) -m 755 $(SHARED_LIB) $(libdir)/$(SHARED_LIB)
	$(INSTALL) -m 755 $(STATIC_LIB) $(libdir)/$(STATIC_LIB)
	@echo
	@echo "+---------- Installing XviD header in $(includedir) ----------+"
	@echo
	$(INSTALL) -m 644 $(SRCDIR)/xvid.h $(includedir)/xvid.h
	@echo

#-----------------------------------------------------------------------------
# Platorm specific file -- dumb rule for people executing make before
# ./configure
#-----------------------------------------------------------------------------

platform.inc: platform.inc.in
	./configure

#-----------------------------------------------------------------------------
# .PHONY targets
#-----------------------------------------------------------------------------

.PHONY: distclean clean info list-objects list-targets list-install-path

clean:
	$(RM) $(OBJECTS)
	$(RM) $(SHARED_LIB)
	$(RM) $(STATIC_LIB)

distclean: clean
	$(RM) config.log
	$(RM) platform.inc
	$(RM) autom4te.cache

list-objects:
	@echo
	@echo "Object files used for this build"
	@echo "---------------------------------------------------------------"
	@echo
	@echo $(OBJECTS)
	@echo

list-targets:
	@echo
	@echo "Target Libraries"
	@echo "---------------------------------------------------------------"
	@echo
	@echo Shared library: $(SHARED_LIB)
	@echo Static library: $(STATIC_LIB)
	@echo

list-install-path:
	@echo
	@echo "Install Paths"
	@echo "---------------------------------------------------------------"
	@echo
	@echo Include: $(includedir)
	@echo Library: $(libdir)
	@echo

info: list-objects list-targets list-install-path
