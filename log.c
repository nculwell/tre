#include "hdrs.c"
#include "mh_log.h"

#if INTERFACE
#define LL_TRACE 1
#define LL_DEBUG 2
#define LL_INFO 3
#define LL_WARN 4
#define LL_ERROR 5
#define logmsg(level, ...) \
  impl_log(level, __FILE__, __LINE__, __VA_ARGS__)
#define logt(...) logmsg(LL_TRACE, __VA_ARGS__)
#define logd(...) logmsg(LL_DEBUG, __VA_ARGS__)
#define log_info(...) logmsg(LL_INFO, __VA_ARGS__)
#define log_warn(...) logmsg(LL_WARN, __VA_ARGS__)
#define log_err(...) logmsg(LL_ERROR, __VA_ARGS__)
#endif

int log_level =
#ifdef NDEBUG
  LL_INFO
#else
  LL_TRACE
#endif
;
FILE *log_file = NULL;

//void print_err(const char *err_msg)
//{
//  fprintf(stderr, err_msg);
//  putc('\n', stderr);
//}
#if INTERFACE
#define print_err(msg) log_err(msg)
#endif

// This function is defined just to force the compiler to expand the macros
// above and trigger error messages if a problem comes up.
void dummy_func()
{
  logt("abc");
  logt("%s", "abc");
  logd("abc");
  logd("%s", "abc");
  log_info("abc");
  log_info("%s", "abc");
  log_warn("abc");
  log_warn("%s", "abc");
  log_err("abc");
  log_err("%s", "abc");
  print_err("abc");
}

void impl_log(int level, const char *file, int line, const char *format, ...)
{
  static int attempted_open = 0;
  va_list args;
  char time_fmt[20];
  if (level < log_level) return;
  if (NULL == log_file && !attempted_open)
  {
    log_file = fopen(default_log_filename, "w");
    if (NULL == log_file)
    {
      print_err("Unable to open log file.");
      attempted_open = 1;
      return;
    }
    atexit(close_log_file);
  }
  fprintf(log_file, "[%s][%s:%d] ", iso_time(time_fmt), file, line);
  va_start(args, format);
  vfprintf(log_file, format, args);
  va_end(args);
  va_start(args, format);
  //vfprintf(stderr, format, args); // XXX: Do we need 2 streams here?
  va_end(args);
  putc('\n', log_file);
  //putc('\n', stderr);
  fflush(log_file);
}

void log_raw(const char *prefix_msg, const char *data, int len)
{
//#define LOG_RAW_BUF_SIZE 200
  //char buf[LOG_RAW_BUF_SIZE];
  //memset(buf, 0, LOG_RAW_BUF_SIZE);
  //memcpy(buf, data, len < LOG_RAW_BUF_SIZE ? len : LOG_RAW_BUF_SIZE);
  fprintf(stderr, prefix_msg);
  putc('"', stderr);
  for (int i=0; i < len; i++)
  {
    int c = data[i];
    if (isprint(c))
      putc(data[i], stderr);
    else
      fprintf(stderr, "\\x%02x", c);
  }
  putc('"', stderr);
  putc('\n', stderr);
}

const char *iso_time(char time_fmt[20])
{
  time_t rawtime;
  time(&rawtime);
  // struct tm *gmt = ;
  strftime(time_fmt, 20, "%F %T", gmtime(&rawtime));
  return time_fmt;
}

void close_log_file()
{
  if (NULL != log_file)
    fclose(log_file);
}
