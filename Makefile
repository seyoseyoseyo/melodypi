CC      = gcc
CFLAGS  = -g  -D_DEFAULT_SOURCE -std=c99 -Werror -Iinclude -I../utils/include -pthread -lwiringPi

SOURCES = $(shell find $(basedir) -name '*.c') $(shell find ../utils -name '*.c')
OBJECTS = $(addprefix $(objdir)/, $(SOURCES:.c=.o))
HEADERS = $(shell find $(basedir)/include -name '*.h')

basedir = .
objdir 	= ../out/extension

.SUFFIXES: .c .o

.PHONY: all clean

all: melody_pi

melody_pi: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(objdir)/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS)
	rm -f melody_pi
