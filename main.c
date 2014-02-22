
#include "hdrs.c"
#include "mh_main.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
TRE_Opts *init_opts() { return NULL; }

void run_editor(TRE_RT *rt) {
  refresh();
  while (1) {
    TRE_RT_update_screen(rt);
    int c = read_char();
    handle_input(rt, c);
  }
}

/*
LOCAL catch_signals() {
  struct sigaction act, oact;
  if (-1 == sigaction(SIGBRK, &act, &oact)) {
    log_err("Unable to install signal handler(s): %s", strerror(errno));
  }
}
*/

void handle_input(TRE_RT *rt, int c) {
  if (c == KEY_F(12)) {
    exit(0);
  }
  else if (isalpha(c) || isdigit(c) || c == ' ') {
    // Do some typing
    TRE_RT_insert_char(rt, c);
  }
  else if (c == '\n') {
    TRE_RT_insert_char(rt, '\n');
  }
  else if (c == KEY_BACKSPACE) {
    TRE_RT_backspace(rt);
  }
  else if (c == KEY_LEFT || c == KEY_RIGHT || c == KEY_UP || c == KEY_DOWN) {
    TRE_RT_arrow_key(rt, c);
  }
  else {
    logt("Other input");
  }
}

void draw_statusbar(TRE_RT *rt) {
  TRE_mode_t mode = TRE_RT_get_mode(rt);
  if (mode == TRE_MODE_INSERT) {
    // Display "-- INSERT --" In the status bar
  }
}

int main(int argc, char *argv[]) {
  TRE_Opts *opts = init_opts(); /* TODO: add in main */
  init_curses(); /* TODO: Make mode option-driven. */
  TRE_RT *rt = TRE_RT_init(opts); /* TODO: add in rt (runs init scripts) */
  TRE_RT_load_buffer(rt, "tre.c");
  run_editor(rt);
  return 0;
}
#pragma GCC diagnostic pop

