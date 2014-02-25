#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "test_main.h"

#if INTERFACE
struct test {
  const char* description;
  void (*test_func)();  
};
struct test_suite {
  const char* name;
  CU_InitializeFunc init;
  CU_CleanupFunc cleanup;
  struct test* tests;
};
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char *argv[]) {
  return run_tests();
}
#pragma GCC diagnostic pop

void add_suite(const struct test_suite* suite) {
  fprintf(stderr, "Adding suite: %s\n", suite->name);
  CU_pSuite pSuite = CU_add_suite(suite->name, suite->init, suite->cleanup);
  if (NULL == pSuite) {
    die("Error adding test suite: ", CU_get_error_msg());
  }
  struct test t;
  int i = 0;
  for (; t = suite->tests[i], t.description != NULL; i++) {
    fprintf(stderr, "Adding test: %s\n", t.description);
    if (!CU_add_test(pSuite, t.description, t.test_func)) {
      die("Error adding test suite %s to registry: %s", suite->name,
          CU_get_error_msg());
    }
  }
  fprintf(stderr, "Tests added: %d\n", i);
}

int run_tests() {
  if (CUE_SUCCESS != CU_initialize_registry()) {
    die("Failed call to CU_initialize_registry: %s", CU_get_error_msg());
  }
  /* Add test suites. */
  add_suite(&buffer_suite);
  /* Run all tests using the CUnit Basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}

void die(const char* msg, ...) {
  va_list ap;
  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  va_end (ap);
  putc('\n', stderr);
  exit(CU_get_error());
}
