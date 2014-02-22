.PHONY: all prebuild release common

SHELL=/bin/sh
CC=gcc
WARNFLAGS=-pedantic -Wall -Wextra -Werror
CFLAGS = -std=c99 $(WARNFLAGS) $(shell pkg-config --cflags glib-2.0)
LDFLAGS=
LDLIBS=$(shell pkg-config --libs glib-2.0) -lncurses
SOURCES=$(wildcard *.c)
HEADERS=$(addprefix :mh_, $(addsuffix .h, $(basename $(SOURCES))))
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=tre

all: CFLAGS += -g
all: common
release: CFLAGS += -DNDEBUG
release: common
	strip $(EXECUTABLE)
common: prebuild $(SOURCES) $(EXECUTABLE)
#	@echo ---------- BUILD COMPLETED SUCCESSFULLY ----------

prebuild:
#	@echo --------------------------------------------------
	@makeheaders $(join $(SOURCES), $(HEADERS))

$(EXECUTABLE): $(OBJECTS)

clean:
	-rm -f *.h *.o

distclean: clean
	-rm -f $(EXECUTABLE) $(EXECUTABLE).pid *.log
