
#include "hdrs.c"
#include "mh_window.h"

#if INTERFACE
typedef struct {
  TRE_Buf *buf;
  int view_start_pos;
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
  // TODO: Use a get cursor position function here instead of the gap.
  int view_start_pos = TRE_Buf_get_cursor_offset(buf);
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

// TODO: Most of this logic should really be in buffer.c, since it's
// buffer-related logic. Only buffer.c should be dealing with offsets and
// especially with the gap.
void TRE_Win_draw(TRE_Win *this) {
  logt("Drawing window");
  int winsz_x, winsz_y;
  // Clear the old contents of the window
  wclear(this->win);
  // Get some bounds info
  getmaxyx(this->win, winsz_y, winsz_x);
  TRE_Buf_draw(this->buf, winsz_y, winsz_x, this->view_start_pos, this);
  wrefresh(this->win);
}

TRE_OpResult TRE_Win_display_char(TRE_Win* this, int y, int x, int c) {
  wmove(this->win, y, x);
  waddch(this->win, c);
  return TRE_SUCC;
}

void TRE_Win_move_cursor(TRE_Win* this, int y, int x) {
  this->cursor_y = y;
  this->cursor_x = x;
}

void TRE_Win_set_focus(TRE_Win *this) {
  move(this->cursor_y, this->cursor_x);
}

void TRE_Win_insert_char(TRE_Win *this, int c) {
  TRE_Buf_insert_char(this->buf, c);
}

void TRE_Win_backspace(TRE_Win *this) {
  TRE_Buf_backspace(this->buf);
}

void TRE_Win_arrow_key(TRE_Win *this, int key) {
  switch (key) {
    case KEY_LEFT:
      TRE_Buf_move(this->buf, -1);
      break;
    case KEY_RIGHT:
      TRE_Buf_move(this->buf, +1);
      break;
    case KEY_UP:
      TRE_Buf_move_linewise(this->buf, -1);
      break;
    case KEY_DOWN:
      TRE_Buf_move_linewise(this->buf, +1);
      break;
  }
}

