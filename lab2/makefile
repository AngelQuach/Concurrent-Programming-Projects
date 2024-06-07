# Makefile for the PNG concatenation project

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99 -g

# Libraries
LIBS = -lpthread -lcurl -lz

# Source files
SRC = paster.c crc.c zutil.c

# Object files
OBJ = $(SRC:.c=.o)

# Dependency files
DEPS = $(SRC:.c:.d)

# Executable name
EXEC = paster

# Default target
all: $(EXEC)

# Link the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Generate dependency files
%.d: %.c
	$(CC) -MM -MF $@ -MT $(@:.d=.o) $<


# Include dependency files
-include $(DEPS)

# Clean up build files
clean:
	rm -f *.o *.d $(EXEC)

# Phony targets
.PHONY: all clean
