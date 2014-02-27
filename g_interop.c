
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
}

