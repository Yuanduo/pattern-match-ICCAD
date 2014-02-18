CC = g++
SUFFIX  = cpp
IOROOT = ../io
IOLIB = ../lib/liboasis.a ../lib/libmisc.a
SPROOT = ../SP
SPLIB = ../lib/libsp.a
BOOLROOT = ../bool
BOOLLIB = ../lib/libbool.a
VFROOT = ../vflib2/include
VFLIB = ../lib/libvf.a

IFLAG = -I. -I$(IOROOT) -I$(SPROOT) -I$(BOOLROOT) -I$(VFROOT)
LFLAG = $(PG) -lm -lz $(DBG)

BINPATH = ../bin
OBJPATH = ../obj

SOURCES = $(wildcard *.$(SUFFIX))
OBJECTS = $(SOURCES:%.$(SUFFIX)=$(OBJPATH)/%.o)
HEADERS = $(wildcard ./*.h)
CFLAGS = $(IFLAG) -c -Wno-deprecated $(DBG) $(PG)

TARGET = $(BINPATH)/fpm5_update.exe
#TARGET = $(BINPATH)/fpm5_debug.exe

$(TARGET):$(OBJECTS) $(IOLIB) $(SPLIB) $(BOOLLIB) $(VFLIB)
	@echo "Now Generating $(TARGET)"
	$(CC) -o $(TARGET) $(OBJECTS) $(LFLAG) $(IOLIB) $(SPLIB) $(BOOLLIB) $(VFLIB)
	@echo "Done!"

# Compile the application code
$(OBJPATH)/%.o: %.$(SUFFIX) $(HEADERS)
	@echo "Now Compile $< ..."
	$(CC) -o $@ $(CFLAGS) $<
	@echo "Done!"
.KEEP_STATE:
clean:
	@echo "Now Removing ..."
	@-rm -rf $(OBJECTS)
	@-rm -rf $(BINPATH)/$(TARGET)
	@echo "Done!"
debug:
#	@$(MAKE) -f makefile DBG="-DDEBUG -g"
	@$(MAKE) -f makefile DBG="-DDEBUG -g" PG="-pg"
opt:
	@$(MAKE) -f makefile DBG="-O3"
explain:
	@echo "The following information represents your program:"
	@echo "Final executable name: $(TARGET)"
	@echo "Source files: $(SOURCES)"
	@echo "Object files: $(OBJECTS)"
