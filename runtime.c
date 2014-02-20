
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
} TRE_RT;
#endif

#pragma GCC diagnostic ignored "-Wunused-parameter"

void TRE_RT_err_msg(TRE_RT *rt, const char *msg)
{
  fprintf(stderr, msg);
}

// This should only be called once
TRE_RT *TRE_RT_init(TRE_Opts *opts) {
  static TRE_RT rt;
  rt.mode = TRE_MODE_NORMAL;
  rt.win = TRE_Win_new();
  return &rt;
}

TRE_mode_t TRE_RT_get_mode(TRE_RT *this) {
  return this->mode;
}

void TRE_RT_load_buffer(TRE_RT *this, const char *filename) {
  TRE_Buf *buf = TRE_Buf_load(this, filename);
  if (buf == NULL) {
    // ignore for now
    logmsg("Failed to load buffer.");
    exit(1);
  }
  TRE_Win_set_buf(this->win, buf);
  logmsg("File loaded into buffer.");
}

void TRE_RT_update_screen(TRE_RT *this) {
  TRE_Win_draw(this->win);
  refresh();
}

