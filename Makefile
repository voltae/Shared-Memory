# Valentin Platzgummer - ic17b06@technikum-wien.at
# Lara Kammerer - ic17b001technikum-wien.at
# Makefile - Shared memory
# Date 11.05.2018

# Define the required macros
CFLAGS = -Wall -Werror -Wextra -Wstrict-prototypes -Wformat=2 -pedantic -fno-common -ftrapv -O3 -g -std=gnu11
CC = gcc52
SEND=sender_test.o
REC=receiver_test.o

DOXYGEN=doxygen
CD=cd
MV=mv
RM=rm
GREP=grep
EXCLUDE_PATTERN=footrulewidth
# add the linking libraries from Semaphores
LIB1=libpthread.a
LIB2=librt.a
# TODO: add the linking libraries s and shared library

%.c: %o
	$(CC) $(CFLAGS) -c

all: $(REC)
	$(CC) $(CFLAGS) -c -l$(LIB1) -l$(LIB2)


.PHONY: clean

clean:
	rm -f *.o $(SEND) $(REC)

.PHONY: distclean

distclean: clean
	$(RM) -rf doc

# create doxy documentation
doc: html pdf

.PHONY: html

# create html version of documentation
html:
	$(DOXYGEN) doxygen.dcf

# create pdf version of documentation
pdf: html
	$(CD) doc/pdf && \
    $(MV) refman.tex refman_save.tex && \
    $(GREP) -v $(EXCLUDE_PATTERN) refman_save.tex > refman.tex && \
    $(RM) refman_save.tex && \
    make && \
    $(MV) refman.pdf refman.save && \
    $(RM) *.pdf *.html *.tex *.aux *.sty *.log *.eps *.out *.ind *.idx \
    *.ilg *.toc *.tps Makefile && \
	$(MV) refman.save refman.pdf

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... distclean"
	@echo "... doc"
	@echo "... html"
	@echo "... pdf"
.PHONY : help