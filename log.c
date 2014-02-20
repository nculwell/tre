
#include "hdrs.c"
#include "mh_log.h"

void logmsg(const char *msg) {
  FILE *logfile = fopen("dbg.log", "at");
  if (logfile != NULL) {
    fprintf(logfile, msg);
    putc('\n', logfile);
    fclose(logfile);
  }
}

