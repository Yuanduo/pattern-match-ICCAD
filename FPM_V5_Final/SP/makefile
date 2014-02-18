CCSUFFIX = cc
CSUFFIX = cpp
GCC = g++

# Warning flags
CPPWARN_FLAGS = \
	-W -Wall \
	-Wimplicit \
	-Wswitch \
	-Wformat \
	-Wchar-subscripts \
	-Wparentheses \
	-Wmultichar \
	-Wtrigraphs \
	-Wpointer-arith \
	-Wcast-align \
	-Wreturn-type \
	-Wno-unused-function \
  -Wundef -Wshadow -Wformat=2 -Wstrict-aliasing -Wextra -Wredundant-decls -Wcast-align -Wswitch-default -Wswitch-enum -Wwrite-strings

OALIBDIR = /DATA/svn/share/src/oa/lib/linux_rhel30_gcc411_32/opt

GUILIBDIR = -L/usr/local/qt4/qt/lib -L../lib -L/home/admin/yaohl/ProgramFiles/boost/lib -L/home/admin/yaohl/ProgramFiles/CGAL/lib -L/usr/local/qt4/qt/lib

LIBDIR = -L../lib $(GUILIBDIR)

DEBUGEXT = 
OALIBS = -loaCommon$(DEBUGEXT) \
         -loaBase$(DEBUGEXT) \
         -loaPlugIn$(DEBUGEXT)\
         -loaDM$(DEBUGEXT)\
         -loaTech$(DEBUGEXT)\
         -loaDesign$(DEBUGEXT)

LIBS = -lm

IFLAG = -I.

CPPFLAGS = $(IFLAG) -c $(DBG)

LFLAG = $(LIBS) 

BINPATH = ./bin
LIBPATH = ../lib
OBJPATH = .

CSOURCES = $(wildcard *.$(CSUFFIX))
CCSOURCES = $(wildcard *.$(CCSUFFIX))
COBJECTS = $(CSOURCES:%.$(CSUFFIX)=$(OBJPATH)/%.o)
CCOBJECTS = $(CCSOURCES:%.$(CCSUFFIX)=$(OBJPATH)/%.o)

HEADERS = $(wildcard ./*.h)
CFLAGS = $(IFLAG) -c $(DBG)

TARGET = $(BINPATH)/sp.exe
TARGET1 = $(LIBPATH)/libsp.a

all: $(TARGET) $(TARGET1)
# Compile the application code
$(TARGET):$(COBJECTS) $(CCOBJECTS)
	@echo "Now Generating $(TARGET)"
	$(GCC) $(DBG) -o $(TARGET) $(COBJECTS) $(CCOBJECTS) $(LFLAG)
	@echo "Done!"
$(TARGET1):$(COBJECTS)
	@echo "Now Generating $(TARGET1)"
	@ar -rv $(TARGET1) $(COBJECTS)
	@ranlib $(TARGET1)
	@echo "Done!"
# Compile the application code
$(OBJPATH)/%.o:%.$(CSUFFIX) $(HEADERS)
	@echo "Now Compile $< ..."
	$(GCC) $(CPPFLAGS) $< -o $@
	@echo "Done!"
$(OBJPATH)/%.o:%.$(CCSUFFIX) $(HEADERS)
	@echo "Now Compile $< ..."
	$(GCC) $(CPPFLAGS) $< -o $@
	@echo "Done!"
.KEEP_STATE:
clean:
	@echo "Now Removing ..."
	@-rm -rf $(COBJECTS)
	@-rm -rf $(CCOBJECTS)
	@-rm -rf $(TARGET)
	@echo "Done!"
debug:
	@$(MAKE) -f makefile DBG="-g"
opt:
	@$(MAKE) -f makefile DBG="-O3"
explain:
	@echo "The following information represents your program:"
	@echo "Source files: $(CSOURCES) $(CCSOURCES)"
	@echo "Header files: $(HEADERS)"
	@echo "Object files: $(COBJECTS) $(CCOBJECTS)"
	@echo "Final executable name: $(TARGET)"
