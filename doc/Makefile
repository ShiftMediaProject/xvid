##############################################################################
#
# Makefile to generate doc from doxygen comments included in code
#
# NB : It's a dumbass makefile, i'm not proud of it, but it does the job if
#      you have doxygen and tetex installed. These 2 programs are commonly
#      available on *nix systems and cygwin. Sorry no real Win32 support.
#      Custom headers/footers/author/latex packages are used and/or generated
#      on the fly.
#
# Author : Edouard Gomez <ed.gomez at wanadoo dot fr>
#
##############################################################################

DATE=${shell date +"%Y-%m-%d"}

all: API

.PHONY: clean-build clean API

API: API.dox foot.inc header.tex
	doxygen API.dox && \
	cd latex && \
	cat doxygen.sty | sed 's/Generated.*Doxygen/XviD API reference/' > tmp.sty && \
	mv tmp.sty doxygen.sty && \
	make && \
	cd .. && \
	mv latex/refman.pdf ./xvid-api-ref.pdf && \
	cp -R html xvid-api-ref && \
	make clean-build

foot.inc:
	cat foot.inc.in | sed 's/@DATE@/${DATE}/' > foot.inc

header.tex:
	cat header.tex.in | sed 's/@DATE@/${DATE}/' > header.tex

clean-build:
	rm -rf html latex foot.inc header.tex

clean: clean-build
	rm -rf xvid-api-ref.pdf xvid-api-ref
