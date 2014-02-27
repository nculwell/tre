
#include "hdrs.c"
#include <libguile.h>
#include "mh_g_interop.h"

struct guile_buf {
  TRE_Buf* c_buf;
};

struct guile_win {
  TRE_Win* c_win;
};

scm_t_bits guile_buf_tag;
scm_t_bits guile_win_tag;

TRE_Buf* scm_to_buf(SCM _buf) {
  scm_assert_smob_type(guile_buf_tag, _buf);
  struct guile_buf* g_buf = (struct guile_buf*)SCM_SMOB_OBJECT(_buf);
  return g_buf->c_buf;
}

void g_init_primitives() {
  guile_buf_tag = scm_make_smob_type("buffer", sizeof(struct guile_buf));
  guile_win_tag = scm_make_smob_type("window", sizeof(struct guile_win));
  g_init_funcs();
  SCM result = scm_c_primitive_load("builtin.scm");
  if (scm_is_false(result)) {
    // TODO: handle init failure
    log_err("Failed to read Scheme builtins.");
  } else {
    logt("Loaded Scheme builtins.");
  }
}

SCM g_invoke(const char* func_name, int n_args, SCM *args) {
  logt("Invoking Scheme function: %s", func_name);
  SCM func_sym = scm_c_lookup(func_name);
  logt("Found symbol.");
  SCM func = scm_variable_ref(func_sym);
  logt("Retrieved variable.");
  if (!scm_program_p(func)) {
    log_err("Not a procedure: %s", func_name);
    return SCM_BOOL_F;
  }
  logt("Making call.");
  SCM result;
  if (n_args == 0) {
    result = scm_call_0(func);
  } else {
    result = scm_call_n(func, args, n_args);
  }
  logt("After call.");
  return result;
}

