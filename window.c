
#include "hdrs.c"
#include "mh_window.h"

#if INTERFACE
typedef struct {
  TRE_Buf *buf;
  bufpos_t view_start_pos;
  int cursor_x;
  int cursor_y;
  WINDOW *win; // the ncurses window; abstract this later
} TRE_Win;
#endif

TRE_Win *TRE_Win_new(int sz_y, int sz_x, int pos_y, int pos_x) {
  TRE_Win *this = g_new(TRE_Win, 1);
  this->buf = NULL; // start with no buffer loaded
  this->view_start_pos = 0;
  this->win = newwin(sz_y, sz_x, pos_y, pos_x);
  logt("Created new %dx%d window at (%d, %d)", sz_y, sz_x, pos_y, pos_x);
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
  logt("Trying to draw window");
  int winsz_x, winsz_y;
  // Clear the old contents of the window
  wclear(this->win);
  // Get some bounds info
  getmaxyx(this->win, winsz_y, winsz_x);
  bufpos_t pos = this->view_start_pos;
  bufpos_t end = this->buf->text_len + this->buf->gap_len;
  // Print as many lines as we have room for in the window
  for (int y=0; y < winsz_y; y++) {
    // print line, wrapping around
    if (pos == end) {
      logt("Reached end of file.");
      break;
    }
    logt("Printing a line.");
    int x=0;
    while (y < winsz_y && pos < end) {
      if (pos == this->buf->gap_start) {
        logt("At cursor position.");
        // Skip over the gap in the buffer
        pos += this->buf->gap_len;
        this->cursor_x = x, this->cursor_y = y;
      }
      // Get the char from the buffer, and advance pos
      int c = this->buf->text.c[pos++];
      // If this is a newline, do a CR-LF operation
      if (c == '\n') {
        logt("Reached newline char.");
        x = 0;
        break;
      }
      // Display the character
      wmove(this->win, y, x);
      if (ERR == waddch(this->win, c)) {
        // Failed to draw to screen, don't know why.
        // TODO: do a proper assertion
        log_err("Failed to display character on screen.");
        exit(1);
      } else {
        // logt("'%c' @ (%d, %d)", c, x, y);
      }
      x++;
      if (x == winsz_x) {
        x = 0, y++;
        logt("Wrapping around.");
      }
    }
  }
  wrefresh(this->win);
}

void TRE_Win_set_cursor(TRE_Win *this) {
  move(this->cursor_y, this->cursor_x);
}

void TRE_Win_insert_char(TRE_Win *this, int c) {
  TRE_Buf_insert_char(this->buf, c);
}

