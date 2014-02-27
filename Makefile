.PHONY: all prebuild release common

SHELL = /bin/sh
CC = gcc
STD= -std=c99
WARNFLAGS = -Wall -Wextra -Werror
BASE_CFLAGS = $(STD) $(WARNFLAGS)
BASE_CFLAGS += $(shell pkg-config --cflags glib-2.0)
BASE_CFLAGS += $(shell pkg-config --cflags guile-2.0)
CFLAGS = $(BASE_CFLAGS) -pedantic
GUILE_CFLAGS = $(BASE_CFLAGS)
LDFLAGS =
LDLIBS =
LDLIBS += $(shell pkg-config --libs glib-2.0)
LDLIBS += $(shell pkg-config --libs guile-2.0)
LDLIBS += -lncurses
#LDLIBS += -L/usr/lib
LDLIBS += /usr/lib/libcunit.a
LDLIBS += libtermkey/.libs/libtermkey.a
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

# Special rules to compile with Guile. Guile uses a void cast that ISO C
# objects to, which prevents use of the -pedantic flag.
g_interop.o: g_interop.c
	$(CC) $(GUILE_CFLAGS) $(shell pkg-config --cflags guile-2.0) -c -o $@ $<
g_funcs.o: g_funcs.c
	$(CC) $(GUILE_CFLAGS) $(shell pkg-config --cflags guile-2.0) -c -o $@ $<

clean:
	-for f in $(SUBPROJECTS); do (cd "$$f" && $(MAKE) $(MFLAGS) clean ); done
	-rm -f *.h *.o *.x test/*.o test/*.h

distclean: clean
	-rm -f $(EXECUTABLE) $(EXECUTABLE).pid *.log $(TEST_RUNNER)

