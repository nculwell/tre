
#include "hdrs.c"
#include "mh_runtime.h"

#if INTERFACE
enum TRE_mode_t {
  TRE_MODE_NORMAL = 1,
  TRE_MODE_INSERT = 2,
  TRE_MODE_VISUAL = 3,
  TRE_MODE_VISUAL_LINE = 4
};

typedef struct {
  // Current editing mode
  TRE_mode_t mode;
  // Current buffer (only one supported right now)
  TRE_Win *win;
  // The status line
  WINDOW *statln;
} TRE_RT;

enum TRE_OpResult {
  TRE_FAIL = 0,
  TRE_SUCC = 1
}
#endif

#pragma GCC diagnostic ignored "-Wunused-parameter"

void TRE_RT_err_msg(TRE_RT *rt, const char *msg)
{
  fprintf(stderr, msg);
}

// This should only be called once
TRE_RT *TRE_RT_init(TRE_Opts *opts) {
  static TRE_RT rt;
  rt.win = TRE_Win_new(LINES - 1, COLS, 0, 0);
  rt.statln = newwin(1, COLS, LINES - 1, 0);
  rt.mode = TRE_MODE_NORMAL;
  return &rt;
}

TRE_mode_t TRE_RT_get_mode(TRE_RT *this) {
  return this->mode;
}

void TRE_RT_load_buffer(TRE_RT *this, const char *filename) {
  TRE_Buf *buf = TRE_Buf_load(this, filename);
  if (buf == NULL) {
    // ignore for now
    log_err("Failed to load buffer.");
    exit(1);
  }
  TRE_Win_set_buf(this->win, buf);
  logt("File loaded into buffer.");
}

void TRE_RT_update_screen(TRE_RT *this) {
  TRE_Win_draw(this->win);
  // TODO: Draw status line
  move(LINES - 1, 0);
  addstr("-- INSERT --");
  TRE_Win_set_focus(this->win);
  refresh();
}

void TRE_RT_insert_char(TRE_RT *this, int c) {
  TRE_Win_insert_char(this->win, c);
}

void TRE_RT_backspace(TRE_RT *this) {
  TRE_Win_backspace(this->win);
}

void TRE_RT_arrow_key(TRE_RT *this, int key) {
  TRE_Win_arrow_key(this->win, key);
}

