# ------------------------------------------------------------
# Nathan Klapstein - CMPUT 379 A3
#
# It is preferred to use Cmake over Makefiles. It is not
# the stone age anymore.
#
# Usage: make // compile programs
#        make tar // create a 'tar.gz' archive of 'allFiles'
#        make clean // remove unneeded files
# ------------------------------------------------------------


# name of the executable to build
BINARY = a3sdn

RM=rm -v -rf
MKDIR = mkdir

CPPFLAGS=-g -pthread -I/sw/include/root -std=c++17 -Wall

BUILDDIR = build
SOURCEDIR = src
HEADERDIR = src
TESTDIR = test

SOURCES := $(shell find $(SOURCEDIR) -name '*.cpp')
SOURCESFILES = $(shell find $(SOURCEDIR) -name '*')
TESTFILES = $(shell find $(TESTDIR) -name '*.txt')
OBJECTS := $(addprefix $(BUILDDIR)/,$(SOURCES:%.cpp=%.o))


.PHONY: all setup clean help tar


# compile a3sdn
all: setup $(BINARY)

$(BINARY): $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(OBJECTS) -o $(BINARY)

$(BUILDDIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) -I$(HEADERDIR) -I$(dir $<) -c $< -o $@

setup:
	$(MKDIR) -p $(BUILDDIR)/$(SOURCEDIR)


# make the submission tar file
ALLFILES = README.md LICENSE Makefile cmput379_assignment_3_report.pdf CMakeLists.txt $(SOURCESFILES) $(TESTFILES)
TARTARGET = submit

tar:
	touch $(TARTARGET).tar.gz
	mv $(TARTARGET).tar.gz  x$(TARTARGET).tar.gz
	tar -cvf $(TARTARGET).tar $(ALLFILES)
	gzip $(TARTARGET).tar


# clean up make outputs
clean:
	$(RM) $(BUILDDIR)
	$(RM) $(BINARY)

distclean: clean


# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... $(EXECNAME)"
	@echo "... tar (generate the submission tar)"
