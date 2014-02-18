# src/Makefile -- top-level Makefile
#
# last modified:   01-Jan-2010  Fri  21:55
#
# Copyright (c) 2004 SoftJin Infotech Private Ltd.
# This software may be used only under the terms of the SoftJin
# Source License.  See the accompanying file LICENSE for details.


CXX   = g++
FLEX  = flex

ZlibIncDir =
Zlibrary   = -lz
##  If your system does not have zlib 1.1.4 or above, download and
##  install it, and add -I and -L directives here, e.g.,
##  ZlibIncDir = -I/usr/local/include
##  Zlibrary   = -L/usr/local/lib -lz
##  (or just Zlibrary = /usr/local/lib/libz.a)


BoostIncDir = -I..
##  If the compiler complains about missing files in the boost
##  directory, download the Boost distribution, unpack it, and put its
##  pathname instead of .. above, e.g.,
##  BoostIncDir = -I/tmp/boost_1_31_0


LFS_CFLAGS  := $(shell getconf LFS_CFLAGS)
LFS_LDFLAGS := $(shell getconf LFS_LDFLAGS)
##  Enable support for large files (> 2 GiB) on 32-bit systems.
##  You will have to change the syntax if you are not using GNU make.


DebugOptions = -O2 -g
##  Add -DNDEBUG to remove the debugging assertions.
##  You might have to remove -g or -O2 if your C++ compiler cannot
##  handle both together.


WarnOptions = -W -Wall
MachineOptions =
#MachineOptions = -m64 -mcpu=ultrasparc


IncDirs      = -I. $(BoostIncDir) $(ZlibIncDir)
CxxOptions   = $(WarnOptions) $(DebugOptions) $(MachineOptions)

CXXFLAGS     = $(CxxOptions) $(IncDirs)
COMPILE_CXX  = $(CXX) $(CXXFLAGS) $(LFS_CFLAGS)
LINK_CXX     = $(CXX) $(CXXFLAGS) $(LFS_LDFLAGS)


StandardGarbage = a.out core core.[0-9][0-9]* make.log TAGS xxx
##  Junk to be removed from each source directory by the 'clean' target.


#-----------------------------------------------------------------------
# You should not need to change anything below.


bindir  = ../bin
libdir  = ../lib
objdir  = ../obj

##  Executables are put in $(bindir), libraries in $(libdir), and .o
##  files in $(objdir).  $(bindir), $(libdir) and $(objdir) are part of
##  the distribution and are not deleted by the clean targets.  That is
##  because I do not know how to make sure that even with "make -j4"
##  the directories always get created before the main build starts.


%.cc:   %.l
	$(FLEX) -o$@ $?


subdirs       = misc       gdsii       oasis       conv
subdirs_all   = misc_all   gdsii_all   oasis_all   conv_all
subdirs_clean = misc_clean gdsii_clean oasis_clean conv_clean


.PHONY:	   dirs all clean $(subdirs_all) $(subdirs_clean)
.DEFAULT:  all


all:	$(subdirs_all)

clean:	$(subdirs_clean)
	rm -f $(StandardGarbage)


TAGS:
	etags */*.cc */*.h */*.l


dirs:
	mkdir -p $(objdir) $(libdir) $(bindir)
	for dir in $(subdirs);  do  mkdir -p $(objdir)/$$dir;  done


##  Follow Peter Miller's advice in his paper "Recursive Make
##  Considered Harmful".  Load all the subdirectory dependencies and
##  build the whole package from this directory instead of cd'ing
##  to each subdirectory and running make there.
##
##  Each foo/Make-defs file defines targets foo_all and foo_clean.
##
include  misc/Make-defs
include gdsii/Make-defs
include oasis/Make-defs
include  conv/Make-defs
