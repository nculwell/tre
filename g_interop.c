
#include <libguile.h>
#include "snarf_g_funcs.x"
#include "g_types.inc"

scm_t_bits guile_buffer_tag;

void guile_init() {
  guile_buffer_tag = scm_make_smob_type("buffer", sizeof(struct guile_buffer));
}

