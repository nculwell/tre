
#include "hdrs.c"
#include "libtermkey/termkey.h"
#include "mh_curses.h"

TermKey* termkey;

TRE_OpResult init_terminal(void)
{
  // Init termkey.
  TERMKEY_CHECK_VERSION;
  if (NULL == (termkey = termkey_new(0, 0))) {
    return TRE_FAIL;
  }
  // Init curses.
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  /* TODO: Set up window(s) */
  atexit(cleanup);
  return TRE_SUCC;
}

LOCAL void cleanup(void)
{
  endwin();
  termkey_destroy(termkey);
}

int read_char(void)
{
  return getch();
}

