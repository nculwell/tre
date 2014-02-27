
#include "hdrs.c"
#include "mh_main.h"
#include <getopt.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#if LOCAL_INTERFACE
struct inner_main_data {
  TRE_RT* rt;
};
#endif

#if INTERFACE
typedef struct {
  char* filename;
} TRE_Opts;
#endif

SCM run_editor(void* void_data) {
  TRE_RT* rt = (TRE_RT*)void_data;
  refresh();
  while (1) {
    scm_c_catch(SCM_BOOL_T, run_loop, rt, error_handler, rt, NULL, NULL);
  }
}

SCM run_loop(void* void_data) {
  TRE_RT* rt = (TRE_RT*)void_data;
  TRE_RT_update_screen(rt);
  int c = read_char();
  SCM_TICK;
  TRE_RT_handle_input(rt, c);
  return SCM_BOOL_T;
}

SCM error_handler(void* void_data, SCM key, SCM args) {
  logt("Caught exception.");
  // TRE_RT* rt = (TRE_RT*)void_data;
  char errmsg[1024];
  strcpy(errmsg, "Error caught: ");
  int offset = strlen(errmsg);
  SCM key_str = scm_symbol_to_string(key);
  offset += g_str_append(key_str, errmsg + offset, 1024 - offset);
  if (offset < 1021) {
    errmsg[offset++] = ':';
    errmsg[offset++] = ' ';
  }
  offset += g_str_list_append(", ", args, errmsg + offset, 1024 - offset);
  errmsg[offset] = 0;
  log_err(errmsg);
  return SCM_BOOL_F;
}

int main(int argc, char *argv[]) {
  scm_boot_guile(argc, argv, inner_main, NULL);
  return 0;
}

void inner_main(void* data, int argc, char **argv) {
  TRE_Opts opts = init_opts(argc, argv);
  if (!init_terminal()) { /* TODO: Make mode option-driven. */
    log_err("Failed to initialize terminal.");
    fprintf(stderr, "Failed to initialize terminal.");
    exit(-1);
  }
  TRE_RT* rt = TRE_RT_init(&opts); /* TODO: add in rt (runs init scripts) */
  TRE_RT_load_buffer(rt, "tre.c");
  g_init_primitives();
  scm_c_catch(SCM_BOOL_T, run_editor, rt, error_handler, rt, NULL, NULL);
  // run_editor(rt);
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

