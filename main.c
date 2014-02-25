
#include "hdrs.c"
#include "mh_main.h"
#include <getopt.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#if INTERFACE
typedef struct {
  char* filename;
} TRE_Opts;
#endif

void run_editor(TRE_RT *rt) {
  refresh();
  while (1) {
    TRE_RT_update_screen(rt);
    int c = read_char();
    TRE_RT_handle_input(rt, c);
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

void draw_statusbar(TRE_RT *rt) {
  TRE_mode_t mode = TRE_RT_get_mode(rt);
  if (mode == TRE_MODE_INSERT) {
    // Display "-- INSERT --" In the status bar
  }
}

int main(int argc, char *argv[]) {
  TRE_Opts opts = init_opts(argc, argv);
  if (!init_terminal()) { /* TODO: Make mode option-driven. */
    log_err("Failed to initialize terminal.");
    fprintf(stderr, "Failed to initialize terminal.");
    return -1;
  }
  TRE_RT *rt = TRE_RT_init(&opts); /* TODO: add in rt (runs init scripts) */
  TRE_RT_load_buffer(rt, "tre.c");
  run_editor(rt);
  return 0;
}

#pragma GCC diagnostic pop

TRE_Opts init_opts(int argc, char *argv[]) {
  TRE_Opts opts;
  memset(&opts, 0, sizeof(TRE_Opts));
  // no_argument, required_argument, optional_argument
  static struct option long_opts[] = {
    { .name = "file", .has_arg = required_argument, .flag = NULL, .val = 'f' },
    { 0, 0, 0, 0 }
  };
  int opt, opt_index = 0;
  while (-1 != (opt = getopt_long(argc, argv, "f:", long_opts, &opt_index))) {
    switch (opt) {
      case 0:
        // flag-based option
        break;
      case 'f':
        opts.filename = g_strdup(optarg);
        break;
      default:
        log_fatal("Invalid flag: %c", opt);
    }
  }
  return opts;
}
