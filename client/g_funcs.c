
#include "hdrs.c"

#include <libguile.h>
#include "mh_g_funcs.h"

// Register the functions that are defined below. We aren't using Guile
// function snarfing because it clashes with Makeheaders, and Makeheaders saves
// a lot more work than function snarfing does.
void g_init_funcs() {
  scm_c_define_gsubr("current-buffer", 0, 0, 0, g_current_buffer);
  scm_c_define_gsubr("insert-char!", 2, 0, 0, g_insert_char);
  scm_c_define_gsubr("read-char-at-cursor", 1, 0, 0, g_read_char);
  scm_c_define_gsubr("delete-char-at-cursor!", 1, 0, 0, g_delete_char);
}

LOCAL TRE_Buf* scm_to_buf(SCM _buf) {
  scm_assert_smob_type(guile_buf_tag, _buf);
  TRE_Buf* buf = (TRE_Buf*)SCM_SMOB_DATA(_buf);
  return buf;
}

LOCAL SCM g_current_buffer() {
  // FIXME: Will be scm_new_smob in the latest release
  SCM smob;
  SCM_NEWSMOB(smob, guile_buf_tag, 0);
  SCM_SET_SMOB_DATA(smob, global_rt->win->buf);
  return smob;
}

LOCAL SCM g_insert_char(SCM _buf, SCM _char) {
  //logt("In g_insert_char.");
  TRE_Buf* buf = scm_to_buf(_buf);
  //logt("Retrieved buf pointer.");
  SCM i = scm_char_to_integer(_char);
  int c = scm_to_int(i);
  //logt("Character to insert: %s", char_to_str(c));
  if (buf) {
    TRE_Buf_insert_char(buf, c);
    //logt("Char inserted successfully.");
  }
  return SCM_UNSPECIFIED;
}

LOCAL SCM g_read_char(SCM _buf) {
  //logt("In g_read_char");
  TRE_Buf* buf = scm_to_buf(_buf);
  //logt("Retrieved buf pointer.");
  int c = TRE_Buf_read_char_at_cursor(buf);
  SCM i = scm_from_int(c);
  return scm_integer_to_char(i);
}

LOCAL SCM g_delete_char(SCM _buf) {
  //logt("In g_delete_char");
  TRE_Buf* buf = scm_to_buf(_buf);
  //logt("Retrieved buf pointer.");
  TRE_Buf_delete(buf);
  return SCM_UNSPECIFIED;
}

