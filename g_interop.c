
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

SCM g_proc_scm_to_string_display;
SCM g_proc_scm_to_string_write;

TRE_Buf* scm_to_buf(SCM _buf) {
  scm_assert_smob_type(guile_buf_tag, _buf);
  struct guile_buf* g_buf = (struct guile_buf*)SCM_SMOB_OBJECT(_buf);
  return g_buf->c_buf;
}

LOCAL SCM g_lookup_proc(const char* pname) {
  SCM psym = scm_c_lookup(pname);
  SCM proc = scm_variable_ref(psym);
  if (!scm_program_p(proc)) {
    log_err("Not a procedure: %s", pname);
    return SCM_BOOL_F;
  } else {
    return proc;
  }
}

void g_init_primitives() {
  guile_buf_tag = scm_make_smob_type("buffer", sizeof(struct guile_buf));
  guile_win_tag = scm_make_smob_type("window", sizeof(struct guile_win));
  g_init_funcs();
  SCM result = scm_c_primitive_load("builtin.scm");
  if (scm_is_false(result)) {
    // TODO: handle init failure
    log_err("Failed to read Scheme builtins.");
    return;
  } else {
    logt("Loaded Scheme builtins.");
  }
  g_proc_scm_to_string_display = g_lookup_proc("value-to-string");
  g_proc_scm_to_string_write = g_lookup_proc("value-to-string-write");
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

int g_scm_write(char* buffer, int buffer_len, SCM value) {
  size_t cstr_len;
  char* cstr = g_scm_string_write(value, &cstr_len);
  int n_appended = c_str_append(buffer, buffer_len, cstr, cstr_len);
  free(cstr);
  return n_appended;
}

int g_str_append(char* buffer, int buffer_len, SCM str) {
  logt("Appending string to buffer.");
  // Convert value to C string.
  size_t key_cstr_len;
  char* key_cstr = g_scm_string_display(str, &key_cstr_len);
  logt("Got string value.");
  int n_appended = c_str_append(buffer, buffer_len, key_cstr, key_cstr_len);
  free(key_cstr);
  logt("Appended text (len %d): %s", n_appended, buffer);
  return n_appended;
}

int g_str_list_append(const char* delim, SCM str_list, char* buffer,
    int buffer_len) {
  // Check for the empty list.
  if (!scm_is_pair(str_list)) {
    return c_str_append(buffer, buffer_len, "()", -1);
  }
  // Handle the list head.
  logt("List head.");
  SCM str = scm_car(str_list);
  int offset = c_str_append(buffer, buffer_len, "(", 1);
  offset +=  g_str_append(buffer + offset, buffer_len - offset, str);
  // Handle the list tail in a loop.
  SCM lst = scm_cdr(str_list);
  int list_node_n = 0;
  while (scm_is_pair(lst) && offset < buffer_len - 1) {
    str = scm_car(lst);
    list_node_n++;
    logt("List node #%d", list_node_n);
    logt("Appending delimiter (%d, %d)", offset, buffer_len - offset);
    offset += c_str_append(buffer + offset, buffer_len - offset, delim, -1);
    logt("Appending list element (%d, %d)", offset, buffer_len - offset);
    offset += g_str_append(buffer + offset, buffer_len - offset, str);
    lst = scm_cdr(lst);
  }
  offset += c_str_append(buffer + offset, buffer_len - offset, ")", 1);
  logt("Finished appending list.");
  return offset;
}

SCM g_scm_to_string_display(SCM value) {
  //logt("Converting Scheme value to string using display.");
  return g_scm_to_string_x(value, g_proc_scm_to_string_display);
}

SCM g_scm_to_string_write(SCM value) {
  //logt("Converting Scheme value to string using write.");
  return g_scm_to_string_x(value, g_proc_scm_to_string_write);
}

SCM g_scm_to_string_x(SCM value, SCM convert_func) {
  // Check if this is already a string, and if so, return it unchanged.
  if (scm_is_string(value)) {
    //logt("Value is already a string.");
    return value;
  } else {
    // Convert to a string and return. This calls a Scheme procedure (one of
    // our builtins) that does the conversion.
    //logt("Calling conversion function.");
    SCM str = scm_call_1(convert_func, value);
    if (scm_is_string(str)) {
      //logt("Retrieved a string.");
      return str;
    } else {
      //log_err("Failed to convert value to string.");
      return SCM_BOOL_F;
    }
  }
}

char* g_scm_string_display(SCM value, size_t *len) {
  // Get the string representation of this value using display.
  SCM str = g_scm_to_string_display(value);
  // Convert the value to a char array. (Not null terminated.)
  return scm_to_latin1_stringn(str, len);
}

char* g_scm_string_write(SCM value, size_t *len) {
  // Get the string representation of this value using write.
  SCM str = g_scm_to_string_write(value);
  // Convert the value to a char array. (Not null terminated.)
  return scm_to_latin1_stringn(str, len);
}

// Copy characters from source string to dest string. This is similar to
// strncpy, except that it handles truncation differently and it returns the
// number of chars copied, which makes it easier to chain multiple appends
// together. The return value is the number of chars copied, not including the
// null terminator.
int c_str_append(char* dest, int dest_buf_len, const char* src, int src_len) {
  // Don't copy anything if there's no room left in dest.
  if (dest_buf_len <= 0) {
    return 0;
  }
  int copy_len;
  if (src_len == -1) {
    // Keep track of both the insert position and the beginning of dest so that
    // we can determine the length after copying is done.
    char* dest_pos = dest;
    // Copy chars until dest buffer runs out of space or null terminator is
    // reached in src. The space left (dest_buf_len) is pre-incremented so that
    // one extra byte is reserved for the null terminator.
    while (*src && --dest_buf_len > 0) {
      copy_len++;
      *(dest_pos++) = *(src++);
    }
    *dest_pos = 0;
    return dest_pos - dest;
  } else {
    // Figure out how much to copy (i.e. truncate if there's not enough space).
    if (src_len < dest_buf_len) {
      copy_len = src_len;
    } else {
      copy_len = dest_buf_len - 1;
    }
    // Copy the chars and add a null terminator.
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
    return copy_len;
  }
}

