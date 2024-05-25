# Makefile, ECE252  
# Yiqing Huang

CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students 
# Source files for each executable
LIB_UTIL = zutil.o crc.o
FINDPNG_SRCS = findpng.c crc.c lab_png.c
CATPNG_SRCS = catpng.c crc.c zutil.c lab_png.c

# Object files for each executable
FINDPNG_OBJS = $(FINDPNG_SRCS: .c=.o)
CATPNG_OBJS = catpng.o lab_png.o $(LIB_UTIL: .c=.o)

# Dependency files for each executable
FINDPNG_DEPS = $(FINDPNG_SRCS:.c=.d)
CATPNG_DEPS = $(CATPNG_SRCS:.c=.d)

# Executable name
FINDPNG_TARGETS = findpng
CATPNG_TARGETS = catpng 

# Default targets
all: ${FINDPNG_TARGETS} ${CATPNG_TARGETS}

# Link object files and create executables
${FINDPNG_TARGETS}: ${FINDPNG_OBJS} 
	$(LD) $(CFLAGS) -o $(LDFLAGS) ${FINDPNG_TARGETS} ${FINDPNG_OBJS} 

${CATPNG_TARGETS}: ${CATPNG_OBJS} 
	$(LD) $(CFLAGS) -o $(LDFLAGS) ${CATPNG_TARGETS} ${CATPNG_OBJS}

# Compile sources files into object files
%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

# Include dependency files
-include $(FINDPNG_DEPS)
-include $(CATPNG_DEPS)

.PHONY: clean
clean:
	rm -f *.d *.o $(FINDPNG_TARGETS) $(CATPNG_TARGETS)

