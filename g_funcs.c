
#include <libguile.h>
#include "g_types.inc"

SCM_DEFINE(g_insert_char, "insert-char", 2, 0, 0, (SCM _buf, SCM _char),
    "Insert a character into the buffer at the point under the cursor.")
{
  TRE_Buf* buf = scm_to_buf(_buf);
  int c = scm_to_char(c);
  if (buf) {
    TRE_Buf_insert_char(buf, c);
  }
  return SCM_BOOL_F;
}

