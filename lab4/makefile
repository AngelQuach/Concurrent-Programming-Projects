# Makefile, ECE252  
# Yiqing Huang <yqhuang@uwaterloo.ca>

CC = gcc 
CFLAGS_XML2 = $(shell xml2-config --cflags)
CFLAGS_CURL = $(shell curl-config --cflags)
CFLAGS = -Wall $(CFLAGS_XML2) $(CFLAGS_CURL) -std=gnu99 -g -DDEBUG1_ -MMD -MP
LD = gcc
LDFLAGS = -std=gnu99 -g 
LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS_PTHREAD = -lpthread
LDLIBS = $(LDLIBS_XML2) $(LDLIBS_CURL) $(LDLIBS_PTHREAD)

SRCS = main.c data_structure.c thread_func.c findpng2.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGETS = findpng2

all: $(TARGETS)

$(TARGETS): $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) *.png *.html