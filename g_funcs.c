
#include "hdrs.c"

#if 0
// The SCM_DEFINE macro screws up Makeheaders. In order to work around the
// problem, we give it an empty definition while the Makeheaders-generated
// header is parsed. Then, once it has been parsed, we undefine SCM_DEFINE
// again so that libguile.h can define it for real.
#define SCM_DEFINE(a,b,c,d,e,f,g)
// Have to pre-declare SCM here because we haven't included libguile.h yet.
typedef struct scm_unused_struct *SCM;
#include "mh_g_funcs.h"
#undef SCM_DEFINE
#endif

#include <libguile.h>
#include "mh_g_funcs.h"

LOCAL SCM g_insert_char (SCM _buf, SCM _char)
{
  TRE_Buf* buf = scm_to_buf(_buf);
  int c = scm_to_char(_char);
  if (buf) {
    TRE_Buf_insert_char(buf, c);
  }
  return SCM_BOOL_F;
}

void g_init_funcs() {
  scm_c_define_gsubr("insert-char", 2, 0, 0, g_insert_char);
}
