#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "test_main.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char *argv[]) {
  if (CUE_SUCCESS != CU_initialize_registry()) {
    die("Failed call to CU_initialize_registry: %s", CU_get_error_msg());
  }

  buf_main_psuite();

  /* Run all tests using the CUnit Basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();

  return 0;
}
#pragma GCC diagnostic pop

void die(const char* msg, ...) {
  va_list ap;
  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  va_end (ap);
  putc('\n', stderr);
  exit(CU_get_error());
}
