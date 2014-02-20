
#include "hdrs.c"
#include "mh_curses.h"

void init_curses(void)
{
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  /* TODO: Set up window(s) */
  atexit(cleanup);
}

LOCAL void cleanup(void)
{
  endwin();
}

int read_char(void)
{
  return getch();
}

