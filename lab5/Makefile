# Makefile, ECE252  
# Yiqing Huang

CC = gcc 
CFLAGS_LIBS = $(shell pkg-config --cflags libxml-2.0 libcurl)
CFLAGS = -Wall $(CFLAGS_LIBS) -std=gnu99 -g
LD = gcc
LDFLAGS = -std=gnu99 -g 
LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS_PTHREAD = -lpthread
LDLIBS = $(LDLIBS_XML2) $(LDLIBS_CURL) $(LDLIBS_PTHREAD)

SRCS = main.c data_structure.c curl_func.c findpng3.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGETS = findpng3

all: $(TARGETS)

$(TARGETS): $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) *.png *.html