# Valentin Platzgummer - ic17b06@technikum-wien.at
# Lara Kammerer - ic17b001technikum-wien.at
# Makefile - Shared memory
# Date 11.05.2018

# Define the required macros
CFLAGS=-Wall -Werror -Wextra -Wstrict-prototypes -Wformat=2 -pedantic -fno-common -ftrapv -O3 -g -std=gnu11
CC=gcc
LDLIBS = -lpthread -lrt

OBJECTS_RECEIVER=receiver_test.o common.o
OBJECTS_SENDER=sender_test.o common.o
HEADER=common.h

DOXYGEN=doxygen
CD=cd
MV=mv
RM=rm
GREP=grep
EXCLUDE_PATTERN=footrulewidth

%.o: %.c
	$(CC) $(CFLAGS)  -c $<

all: receivertest sendertest

receivertest: $(OBJECTS_RECEIVER)
	$(CC) $(CFLAGS) $(OBJECTS_RECEIVER) $(HEADER) -o$@ $(LDLIBS)

sendertest: $(OBJECTS_SENDER)
	$(CC) $(CFLAGS) $(OBJECTS_SENDER) $(HEADER) -o$@ $(LDLIBS)


testsimple: sender_simple receiver_simple

sender_simple: sender_simple.c
	$(CC) $< -o$@ $(LDLIBS)
receiver_simple: receiver_simple.c
	$(CC) $< -o$@ $(LDLIBS)
.PHONY: clean

clean:
	rm -f *.o

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