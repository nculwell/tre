
#include "hdrs.c"

#include <libguile.h>
#include "mh_g_funcs.h"

// Register the functions that are defined below. We aren't using Guile
// function snarfing because it clashes with Makeheaders, and Makeheaders saves
// a lot more work than function snarfing does.
void g_init_funcs() {
  scm_c_define_gsubr("insert-char", 1, 0, 0, g_insert_char);
}

LOCAL SCM g_insert_char (SCM _buf, SCM _char)
{
  logt("In g_insert_char.");
  TRE_Buf* buf = scm_to_buf(_buf);
  logt("Retrieved buf pointer.");
  int c = scm_to_char(_char);
  if (buf) {
    TRE_Buf_insert_char(buf, c);
  }
  return SCM_BOOL_F;
}

