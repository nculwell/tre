
#include "hdrs.c"
#include "mh_window.h"

#if INTERFACE
typedef struct {
  TRE_Buf *buf;
  bufpos_t view_start_pos;
  WINDOW *win; // the ncurses window; abstract this later
} TRE_Win;
#endif

TRE_Win *TRE_Win_new() {
  TRE_Win *this = g_new(TRE_Win, 1);
  this->buf = NULL; // start with no buffer loaded
  this->view_start_pos = 0;
  int srcsz_x, scrsz_y;
  getmaxyx(stdscr, srcsz_x, scrsz_y);
  this->win = newwin(srcsz_x, scrsz_y, 0, 0);
  return this;
}

void TRE_Win_set_buf(TRE_Win *this, TRE_Buf *buf) {
  this->buf = buf;
  bufpos_t view_start_pos = buf->gap_start;
  // Back up the view start position to the start of the line with the cursor
  // in it (or to the beginning of the file, whichever comes first)
  while (view_start_pos > 0) {
    view_start_pos--;
    if (buf->text.c[view_start_pos] == '\n') {
      view_start_pos++;
      break;
    }
  }
  this->view_start_pos = view_start_pos;
}

void TRE_Win_draw(TRE_Win *this) {
  logmsg("Trying to draw window");
  int winsz_x, winsz_y;
  // Clear the old contents of the window
  wborder(this->win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
  getmaxyx(this->win, winsz_y, winsz_x);
  bufpos_t pos = this->view_start_pos;
  bufpos_t end = this->buf->text_len + this->buf->gap_len;
  for (int y=0; y < winsz_y; y++) {
    // print line, wrapping around
    int x=0;
    while (y < winsz_y && pos < end) {
      if (pos == this->buf->gap_start) {
        pos += this->buf->gap_len;
      }
      int c = this->buf->text.c[pos];
      if (c == '\n') {
        break;
      }
      if (ERR == waddch(this->win, c)) {
        // Failed to draw to screen, don't know why.
        // TODO: do a proper assertion
        exit(1);
      }
      x++;
      if (x == winsz_x) {
        x = 0;
        y++;
      }
    }
  }
}

