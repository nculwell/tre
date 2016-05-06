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

void* my_alloc(size_t size) {
  void* ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "Failed malloc.\n");
    abort();
  }
  return ptr;
}

void* my_realloc(void* old_ptr, size_t size) {
  void* ptr = realloc(old_ptr, size);
  if (!ptr) {
    fprintf(stderr, "Failed realloc.\n");
    abort();
  }
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

