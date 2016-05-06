// NOTE: Don't create any global variables in this file. This file is not
// linked with the test runner, so if there are references to anything defined
// here then it will break the build.

#include "hdrs.c"
#include "mh_main.h"
#include <getopt.h>
#include <malloc.h>
#ifdef _WIN32
# include <windows.h>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/*

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

#define ERROR_HANDLER_ERRBUF_LEN 1024
SCM error_handler(void* void_data, SCM key, SCM args) {
  logt("Caught exception.");
  // TRE_RT* rt = (TRE_RT*)void_data;
  char errbuf[ERROR_HANDLER_ERRBUF_LEN];
  int buflen = ERROR_HANDLER_ERRBUF_LEN;
  int offset = c_str_append(errbuf, buflen, "Error caught: ", -1);
  offset += g_scm_write(errbuf + offset, buflen - offset, key);
  offset += c_str_append(errbuf + offset, buflen - offset, " - ", 1);
  offset += g_scm_write(errbuf + offset, buflen - offset, args);
  log_err(errbuf);
  logt("format-apply");
  SCM msg = scm_call_1(g_proc_format_apply, args);
  logt("error message");
  g_str_append(errbuf, buflen, msg);
  log_err(errbuf);
  return SCM_BOOL_F;
}

int main(int argc, char *argv[]) {
  scm_boot_guile(argc, argv, inner_main, NULL);
  return 0;
}

void inner_main(void* data, int argc, char **argv) {
  TRE_Opts opts = init_opts(argc, argv);
  if (!init_terminal()) { // TODO: Make mode option-driven.
    log_err("Failed to initialize terminal.");
    fprintf(stderr, "Failed to initialize terminal.");
    exit(-1);
  }
  TRE_RT* rt = TRE_RT_init(&opts); // TODO: add in rt (runs init scripts)
  global_rt = rt;
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

*/

void* my_alloc(size_t size) {
  void* ptr = malloc(size);
  if (!ptr)
    abort();
  return ptr;
}

void* my_realloc(void* old_ptr, size_t size) {
  void* ptr = realloc(old_ptr, size);
  if (!ptr)
    abort();
  return ptr;
}

char* my_strdup(const char* str) {
  int len = strlen(str);
  char* newstr = my_alloc(len + 1);
  strcpy(newstr, str);
  return newstr;
}

void my_free(void* ptr) {
  assert(ptr);
  free(ptr);
}

char* my_file_get_contents(const char* filename, const char** error) {
  // TODO: better error message
  FILE* f = fopen(filename, "rb");
  if (!f) {
    *error = "Unable to open file.";
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  long file_len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* contents = my_alloc(file_len + 1);
    size_t n_read;
    size_t n_left = file_len;
    while ((n_read = fread(contents, 1, n_left, f)) < n_left) {
      if (ferror(f)) {
        *error = "Read error.";
        return NULL;
      }
      n_left -= n_read;
    }
  return contents;
}

int my_realpath(const char* path, char* resolved_path) {
#ifdef _WIN32
  DWORD path_len = GetFullPathName(path, PATH_MAX, resolved_path, NULL);
  return (path_len <= PATH_MAX);
#else
  return (NULL != realpath(path, resolved_path));
#endif
}

