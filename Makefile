.PHONY: all prebuild release common

SHELL = /bin/sh
CC = gcc
WARNFLAGS = -pedantic -Wall -Wextra -Werror
CFLAGS = -std=c99 $(WARNFLAGS) $(shell pkg-config --cflags glib-2.0)
LDFLAGS =
LDLIBS = $(shell pkg-config --libs glib-2.0) -lncurses -L/usr/lib
LDLIBS += /usr/lib/libcunit.a libtermkey/.libs/libtermkey.a
SOURCES = $(wildcard *.c)
TEST_SOURCES = $(wildcard test/*.c)
HEADERS = $(addprefix :mh_, $(addsuffix .h, $(basename $(SOURCES))))
OBJECTS = $(SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)
TEST_RUNNER = test/test_main
MHFLAGS =
SUBPROJECTS = libtermkey

EXECUTABLE=tre

all: CFLAGS += -g
all: LDFLAGS += -rdynamic
all: MHFLAGS += -L
all: common
release: CFLAGS += -DNDEBUG
release: common
	strip $(EXECUTABLE)
common: prebuild $(SOURCES) $(TEST_SOURCES) $(EXECUTABLE) $(TEST_RUNNER)
	@echo ---------- BUILD COMPLETED SUCCESSFULLY ----------

prebuild:
	@echo --------------------------------------------------
	makeheaders $(MHFLAGS) $(join $(SOURCES), $(HEADERS))
	makeheaders $(MHFLAGS) $(addsuffix :, $(SOURCES)) $(TEST_SOURCES)
	-for f in $(SUBPROJECTS); do (cd "$$f" && $(MAKE) $(MFLAGS) ); done

$(EXECUTABLE): $(OBJECTS)
$(TEST_RUNNER): $(TEST_OBJECTS) $(filter-out main.o, $(OBJECTS))

clean:
	-for f in $(SUBPROJECTS); do (cd "$$f" && $(MAKE) $(MFLAGS) clean ); done
	-rm -f *.h *.o test/*.o test/*.h

distclean: clean
	-rm -f $(EXECUTABLE) $(EXECUTABLE).pid *.log $(TEST_RUNNER)

