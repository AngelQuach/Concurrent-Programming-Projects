# Modifed from Makefile, ECE252  
# Yiqing Huang


CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 -Iinclude # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz


# Source files for each executable
FINDPNG_SRCS = findpng.c crc.c
CATPNG_SRCS = catpng.c crc.c zutil.c


# Object files for each executable
FINDPNG_OBJS = $(FINDPNG_SRCS:.c=.o)
CATPNG_OBJS = $(CATPNG_SRCS:.c=.o)


# Dependency files for each executable
FINDPNG_DEPS = $(FINDPNG_SRCS:.c:.d)
CATPNG_DEPS = $(CATPNG_SRCS:.c:.d)


# Executable name
FINDPNG_TARGET = findpng
CATPNG_TARGET = catpng


# Default targets
all: $(FINDPNG_TARGET) $(CATPNG_TARGET)


# Link object files and create executables
$(FINDPNG_TARGET): $(FINDPNG_OBJS)
	$(LD) $(LDFLAGS) -o $(FINDPNG_TARGET) $(FINDPNG_OBJS) $(LDLIBS)


$(CATPNG_TARGET): $(CATPNG_OBJS)
	$(LD) $(LDFLAGS) -o $(CATPNG_TARGET) $(CATPNG_OBJS) $(LDLIBS)


# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# Generate dependency files
%.d: %.c
	$(CC) -MM -MF $@ -MT $(@:.d=.o) $<


# Include dependency files
-include $(FINDPNG_DEPS)
-include $(CATPNG_DEPS)


.PHONY: clean
clean:
	rm -f *.o *.d $(FINDPNG_TARGET) $(CATPNG_TARGET)