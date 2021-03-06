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

int main(int argc, char *argv[]) {
  net_init();
  main_loop();
  return 0;
}

void main_loop() {
  int server_fd = net_listen();
  for (;;) {
    int client_fd = net_accept(server_fd);
    if (client_fd >= 0) {
      for (;;) {
        char client_cmd[BUFSIZ];
        char server_cmd[BUFSIZ];
        int client_cmd_len = net_recv(client_fd, client_cmd, BUFSIZ - 1);
        if (client_cmd_len < 1)
          break;
        client_cmd[client_cmd_len] = '\0';
        int server_cmd_len = process_command(client_cmd, client_cmd_len, server_cmd);
        if (server_cmd_len == 6 && !strcmp("quit\r\n", server_cmd)) {
          net_close(client_fd);
          break;
        }
        int send_result = net_send(client_fd, server_cmd, server_cmd_len);
        if (send_result < 1)
          break;
      }
    }
  }
}

int process_command(const char* client_cmd, int client_cmd_len, char* server_cmd) {
  if (client_cmd_len == 6 && !strcmp("quit\r\n", client_cmd)) {
    strcpy(server_cmd, "quit\r\n");
  } else {
    strcpy(server_cmd, "OK\r\n");
  }
  return strlen(server_cmd);
}

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

