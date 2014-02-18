SUFFIX  = cpp
GCC = g++

IFLAG = -I. -I ./kbool/

BINPATH = .
OBJPATH = .

SOURCES = $(wildcard *.$(SUFFIX))
SOURCES2 = $(wildcard ./kbool/*.$(SUFFIX))
OBJECTS = $(SOURCES:%.$(SUFFIX)=$(OBJPATH)/%.o)
OBJECTS2 = $(SOURCES2:%.$(SUFFIX)=%.o)
HEADERS = $(wildcard ./*.h)
HEADERS2 = $(wildcard ./kbool/*.h)
CFLAGS = $(IFLAG) -c $(DBG) -Wunused

TARGET = $(BINPATH)/libbool.a

$(TARGET):$(OBJECTS) $(OBJECTS2)
	@echo "Now Generating $(TARGET)"
	@ar -rv $(TARGET) $(OBJECTS) $(OBJECTS2)
	@ranlib $(TARGET)
	cp -f $(TARGET) ../lib
	@echo "Done!"

# Compile the application code
./kbool/%.o: ./kbool/%.$(SUFFIX) ./$(HEADERS2)
	@echo "Now Compile $< ..."
	$(GCC) -o $@ $(CFLAGS) $<
	@echo "Done!"
$(OBJPATH)/%.o: %.$(SUFFIX) $(HEADERS)
	@echo "Now Compile $< ..."
	$(GCC) -o $@ $(CFLAGS) $<
	@echo "Done!"
.KEEP_STATE:
cleanlib:
	@echo "Now Removing libs ..."
	@-rm -rf $(TARGET)
clean:
	@echo "Now Removing ..."
	@-rm -rf $(OBJECTS)
	@-rm -rf $(OBJECTS2)
	@-rm -rf $(BINPATH)/$(TARGET)
	@echo "Done!"
debug:
	@$(MAKE) -f makefile DBG="-g" PG="-pg"
opt:
	@$(MAKE) -f makefile DBG="-O3"
explain:
	@echo "The following information represents your program:"
	@echo "Final executable name: $(TARGET)"
	@echo "Source files: $(SOURCES)"
	@echo "Object files: $(OBJECTS)"
	@echo "Source files: $(SOURCES2)"
	@echo "Object files: $(OBJECTS2)"
