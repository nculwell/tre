
#include "hdrs.c"
#include "mh_log.h"

const char log_filename[] = "dbg.log";

void init_log() {
  FILE *logfile = fopen(log_filename, "wt");
  if (logfile != NULL) {
    fprintf(logfile, "Started.\n");
    fclose(logfile);
  }
}

void logmsg(const char *msg) {
  FILE *logfile = fopen(log_filename, "at");
  if (logfile != NULL) {
    fprintf(logfile, msg);
    putc('\n', logfile);
    fclose(logfile);
  }
}

