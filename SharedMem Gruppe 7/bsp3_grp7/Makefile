##
## @file Makefile
## mypopen GNU-like popen Library Makefile
## Beispiel 2
##
## @author Dominic Mages <ic17b014@technikum-wien.at>
## @author Ralph Hödl <ic17b003@technikum-wien.at>
## @author David North <ic17b086@technikum-wien.at>
## @date 2018/06/14
##
## @version 2 $
##
## @todo
##
## Last Modified: $Author: Dominic Mages $
##

CC=gcc52
CFLAGS=-DDEBUG -Wall -pedantic -Werror -Wextra -Wstrict-prototypes -fno-common -g -O3 -std=gnu11 -c
CP=cp
CD=cd
MV=mv
GREP=grep
DOXYGEN=doxygen

OBJECTS_SENDER=sender.o datah.o
OBJECTS_EMPFAENGER=empfaenger.o datah.o

EXCLUDE_PATTERN=footrulewidth

##
## ----------------------------------------------------------------- rules --
##

%.o: %.c
	$(CC) $(CFLAGS) -c $<

##
## --------------------------------------------------------------- targets --
##

.PHONY: all sender empfaenger
all: sender empfaenger

sender: $(OBJECTS_SENDER)
	$(CC) $^ -o $@ -lpthread -lrt
	
empfaenger: $(OBJECTS_EMPFAENGER)
	$(CC) $^ -o $@ -lpthread -lrt

test:
	/usr/local/bin/test_sender_empfaenger.sh -f

cleanshm:
	find /dev/shm -user $(whoami) -name "sem.$(id -u)*" -type f -delete

.PRECIOUS: %.tex

.PHONY: clean
clean:
	$(RM) *.o *~ sender empfaenger

.PHONY: distclean
distclean: clean
	$(RM) -r doc

doc: html pdf

.PHONY: html
html:
	$(DOXYGEN) doxygen.dcf

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

##
## ---------------------------------------------------------- dependencies --
##

##
## =================================================================== eof ==
##
