# Valentin Platzgummer - ic17b06@technikum-wien.at
# Lara Kammerer - ic17b001technikum-wien.at
# Makefile - Shared memory
# Date 11.05.2018

# Define the required macros
CFLAGS=-Wall -Werror -Wextra -Wstrict-prototypes -Wformat=2 -pedantic -fno-common -ftrapv -O3 -g -std=gnu11
LDLIBS = -lpthread -lrt

OBJECTS_RECEIVER=receiver.o
OBJECTS_SENDER=sender.o
OBJECTS_COMMON=common.o
EXECUTABLES=receiver sender
HEADER=sharedMemory.h

#get machines name
MACHINENAME := $(shell hostname)
ANNUMINAS=annuminas.technikum-wien.at
# conditional change of compiler
CC=$(GCC)

# distingush between the two different machines for the compiler
ifeq ($(MACHINENAME), $(ANNUMINAS))
GCC=gcc52
else
GCC=gcc
endif

DOXYGEN=doxygen
CD=cd
MV=mv
RM=rm
GREP=grep
EXCLUDE_PATTERN=footrulewidth
# get the current id from shell
ID = $(shell id -g)

%.o: %.c
	$(CC) $(CFLAGS)  -c $<


all: $(EXECUTABLES)

receiver: $(OBJECTS_RECEIVER) $(OBJECTS_COMMON)
	$(CC) $(CFLAGS) $(OBJECTS_RECEIVER) $(OBJECTS_COMMON) $(HEADER) -o$@ $(LDLIBS)

sender: $(OBJECTS_SENDER) $(OBJECTS_COMMON)
	$(CC) $(CFLAGS) $(OBJECTS_SENDER) $(OBJECTS_COMMON) $(HEADER) -o$@ $(LDLIBS)

# runs the test on annuminas
runtest: receiver sender
	test_sender_empfaenger.sh -s./sender -e./receiver -f

# remove the object files
.PHONY: clean
clean:
	$(RM) -f *.o $(EXECUTABLES)

.PHONY: deleteResources
#delete the semaphores and shared memory from /dev/shm/ with the naming from the description
deleteResources:
	rm /dev/shm/sem.sem_$(ID)* /dev/shm/shm_$(ID)*

.PHONY: distclean
# remove the documentation folder
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
	@echo "... testsimple"
	@echo "... runtest"
	@echo "... clean"
	@echo "... deleteResources"
	@echo "... distclean"
	@echo "... doc"
	@echo "... html"
	@echo "... pdf"
.PHONY : help