#include <CUnit/CUnit.h>
#include "../hdrs.c"
#include "buffer.h"

struct test buffer_tests[] = {
  { "create empty buffer", test_buffer_create },
  { "load buffer from string", test_buffer_load_from_string },
  { "empty buffer matches buffer loaded from empty string",
    test_empty_buf_from_string_matches_new_buf },
  { "move cursor right two spaces", test_move_cursor_right },
  { "move cursor right to end of line", test_move_cursor_right_to_eol },
  { "move cursor right past end of line (wrap to next line)",
    test_move_cursor_right_wrap_to_next_line },
  { "move cursor right to end of file", test_move_cursor_right_to_eof },
  { "move cursor right past end of file 3 times",
    test_move_cursor_right_past_eof_3_times },
  { "move down through empty lines", test_move_down_through_empty_line },
  { "move up through empty lines", test_move_up_through_empty_line },
  { NULL, NULL }
};

struct test_suite buffer_suite = {
  .name = "Buffer",
  .init = NULL,
  .cleanup = NULL,
  .tests = buffer_tests
};

void test_buffer_create() {
  static const char TEST_FILE_NAME[] = "test_file.txt";
  TRE_Buf* buf = TRE_Buf_new(TEST_FILE_NAME);
  CU_ASSERT(!strcmp(buf->filename, TEST_FILE_NAME));
  CU_ASSERT(buf->buf_size == TRE_BUFFER_BLOCK_SIZE);
  CU_ASSERT(buf->text_len == 1);
  CU_ASSERT(buf->gap_start == 0);
  CU_ASSERT(buf->gap_len == TRE_BUFFER_GAP_SIZE - 1);
  CU_ASSERT(buf->n_lines == 1);
  CU_ASSERT(buf->encoding == TRE_BUF_ENCODING_ASCII);
  CU_ASSERT(buf->col_affinity == -1);
  CU_ASSERT(buf->cursor_line.num == 0);
  CU_ASSERT(buf->cursor_line.off == 0);
  CU_ASSERT(buf->cursor_line.len == 1);
  CU_ASSERT(buf->cursor_col == 0);
  CU_ASSERT(gap_matches_cursor(buf));
}

void test_empty_buf_from_string_matches_new_buf() {
  TRE_Buf* bn = TRE_Buf_new(NULL);
  TRE_Buf* bs = TRE_Buf_load_from_string("");
  CU_ASSERT(bn->filename == bs->filename);
  CU_ASSERT(bn->buf_size == bs->buf_size);
  CU_ASSERT(bn->text_len == bs->text_len);
  CU_ASSERT(bn->gap_start == bs->gap_start);
  CU_ASSERT(bn->gap_len == bs->gap_len);
  CU_ASSERT(bn->n_lines == bs->n_lines);
  CU_ASSERT(bn->encoding == bs->encoding);
  CU_ASSERT(bn->col_affinity == bs->col_affinity);
  CU_ASSERT(bn->cursor_line.num == bs->cursor_line.num);
  CU_ASSERT(bn->cursor_line.off == bs->cursor_line.off);
  CU_ASSERT(bn->cursor_line.len == bs->cursor_line.len);
  CU_ASSERT(bn->cursor_col == bs->cursor_col);
  CU_ASSERT(compare_buffers(bn, bs));
  CU_ASSERT(gap_matches_cursor(bn));
  CU_ASSERT(gap_matches_cursor(bs));
}

void test_buffer_load_from_string() {
  static const char* TEST_STRING = "abc\ndef\nxyz\njkl\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(TEST_STRING);
  CU_ASSERT(buf->filename == NULL);
  CU_ASSERT(buf->buf_size == 2 * TRE_BUFFER_BLOCK_SIZE);
  CU_ASSERT(buf->text_len == (int)strlen(TEST_STRING));
  CU_ASSERT(buf->gap_start == 0);
  CU_ASSERT(buf->gap_len == TRE_BUFFER_GAP_SIZE);
  CU_ASSERT(buf->n_lines == 4);
  CU_ASSERT(buf->encoding == TRE_BUF_ENCODING_ASCII);
  CU_ASSERT(buf->col_affinity == -1);
  CU_ASSERT(buf->cursor_line.num == 0);
  CU_ASSERT(buf->cursor_line.off == 0);
  CU_ASSERT(buf->cursor_line.len == 4);
  CU_ASSERT(buf->cursor_col == 0);
  CU_ASSERT(0 ==
      memcmp(TEST_STRING,
        buf->text.c + buf->gap_start + buf->gap_len,
        strlen(TEST_STRING)));
  CU_ASSERT(gap_matches_cursor(buf));
}

void test_move_cursor_right() {
  static const char* TEST_STRING = "abc\ndef\nxyz\njkl\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(TEST_STRING);
  TRE_Buf_move_charwise(buf, 1);
  CU_ASSERT(gap_matches_cursor(buf));
  TRE_Buf_move_charwise(buf, 1);
  CU_ASSERT(gap_matches_cursor(buf));
  CU_ASSERT(buf->cursor_line.num == 0);
  CU_ASSERT(buf->cursor_line.off == 0);
  CU_ASSERT(buf->cursor_line.len == 4);
  CU_ASSERT(buf->cursor_col == 2);
}

void test_move_cursor_right_to_eol() {
  static const char* TEST_STRING = "abc\ndef\nxyz\njkl\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(TEST_STRING);
  for (int i=0; i < 3; i++) {
    TRE_Buf_move_charwise(buf, 1);
    CU_ASSERT(gap_matches_cursor(buf));
  }
  CU_ASSERT(buf->cursor_line.num == 0);
  CU_ASSERT(buf->cursor_line.off == 0);
  CU_ASSERT(buf->cursor_line.len == 4);
  CU_ASSERT(buf->cursor_col == 3);
}

void test_move_cursor_right_wrap_to_next_line() {
  static const char* TEST_STRING = "abc\ndef\nxyz\njkl\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(TEST_STRING);
  for (int i=0; i < 4; i++) {
    TRE_Buf_move_charwise(buf, 1);
    CU_ASSERT(gap_matches_cursor(buf));
  }
  CU_ASSERT(buf->cursor_line.num == 1);
  CU_ASSERT(buf->cursor_line.off == 4);
  CU_ASSERT(buf->cursor_line.len == 4);
  CU_ASSERT(buf->cursor_col == 0);
}

void test_move_cursor_right_to_eof() {
  static const char* TEST_STRING = "abc\ndef\nxyz\njkl\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(TEST_STRING);
  for (int i=0; i < 16; i++) {
    TRE_Buf_move_charwise(buf, 1);
    CU_ASSERT(gap_matches_cursor(buf));
  }
  CU_ASSERT(buf->cursor_line.num == 3);
  CU_ASSERT(buf->cursor_line.off == 12);
  CU_ASSERT(buf->cursor_line.len == 4);
  CU_ASSERT(buf->cursor_col == 3);
}

void test_move_cursor_right_past_eof() {
  static const char* TEST_STRING = "abc\ndef\nxyz\njkl\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(TEST_STRING);
  for (int i=0; i < 17; i++) {
    TRE_Buf_move_charwise(buf, 1);
    CU_ASSERT(gap_matches_cursor(buf));
  }
  CU_ASSERT(buf->cursor_line.num == 3);
  CU_ASSERT(buf->cursor_line.off == 12);
  CU_ASSERT(buf->cursor_line.len == 4);
  CU_ASSERT(buf->cursor_col == 3);
}

void test_move_cursor_right_past_eof_3_times() {
  static const char* TEST_STRING = "abc\ndef\nxyz\njkl\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(TEST_STRING);
  for (int i=0; i < 19; i++) {
    TRE_Buf_move_charwise(buf, 1);
    CU_ASSERT(gap_matches_cursor(buf));
  }
  CU_ASSERT(buf->cursor_line.num == 3);
  CU_ASSERT(buf->cursor_line.off == 12);
  CU_ASSERT(buf->cursor_line.len == 4);
  CU_ASSERT(buf->cursor_col == 3);
}

void test_move_down_through_empty_line() {
  static const char test_file[] = "\n\n\n\n\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(test_file);
  TRE_Buf_move_linewise(buf, 1);
  CU_ASSERT(gap_matches_cursor(buf));
  TRE_Buf_move_linewise(buf, 1);
  CU_ASSERT(gap_matches_cursor(buf));
  exit(0);
  CU_ASSERT(buf->cursor_line.num == 2);
  CU_ASSERT(buf->cursor_line.off == 2);
  CU_ASSERT(buf->cursor_line.len == 1);
  CU_ASSERT(buf->cursor_col == 0);
}

void test_move_up_through_empty_line() {
  static const char test_file[] = "\n\n\n\n\n";
  TRE_Buf* buf = TRE_Buf_load_from_string(test_file);
  TRE_Buf_move_linewise(buf, 1);
  CU_ASSERT(gap_matches_cursor(buf));
  TRE_Buf_move_linewise(buf, 1);
  CU_ASSERT(gap_matches_cursor(buf));
  TRE_Buf_move_linewise(buf, -1);
  CU_ASSERT(gap_matches_cursor(buf));
  TRE_Buf_move_linewise(buf, -1);
  CU_ASSERT(gap_matches_cursor(buf));
  CU_ASSERT(buf->cursor_line.num == 0);
  CU_ASSERT(buf->cursor_line.off == 0);
  CU_ASSERT(buf->cursor_line.len == 1);
  CU_ASSERT(buf->cursor_col == 0);
}

// Compare the gaps and text of two buffers to determine if they are identical.
// Returns nonzero if they are identical, zero if not.
// XXX: Include this in the actual program code?
LOCAL int compare_buffers(TRE_Buf* b1, TRE_Buf* b2) {
  // Stop if the gaps don't match because the text compare that follows won't
  // be valid in that case.
  if (b1->gap_start != b2->gap_start || b1->gap_len != b2->gap_len) {
    return 0;
  }
  // Compare the part of the buffer before the gap.
  if (0 == memcmp(b1->text.c, b2->text.c, b1->gap_start)) {
    return 0;
  }
  // Compare the part of the buffer after the gap.
  int offset = b1->gap_start + b1->gap_len;
  int len = b1->text_len - b1->gap_start;
  if (0 == memcmp(b1->text.c + offset, b2->text.c + offset, len)) {
    return 0;
  }
  return 1;
}

LOCAL int gap_matches_cursor(TRE_Buf* buf) {
  return buf->gap_start == buf->cursor_line.off + buf->cursor_col;
}
