# Makefile, ECE252
# Yiqing Huang

CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students
LIB_UTIL = zutil.o crc.o
SRCS   = catpng.c crc.c zutil.c
OBJS   = catpng.o $(LIB_UTIL)

TARGETS= catpng

all: ${TARGETS} findpng pnginfo

catpng: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

findpng: findpng.c
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

pnginfo: pnginfo.c
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) findpng pnginfo