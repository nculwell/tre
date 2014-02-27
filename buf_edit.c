#include "hdrs.c"
#include "mh_buf_edit.h"

// Insert a character into the gap.
void TRE_Buf_insert_char(TRE_Buf *buf, char c) {
  logt("Inserting character: %s", char_to_str(c));
  // Editing clears the column affinity.
  TRE_Buf_clear_col_affinity(buf);
  // Put the character into the buffer at the start of the gap.
  buf->text.c[buf->gap_start++] = c;
  // Update buffer position info.
  if (c == '\n') {
    // Inserting a newline splits the current line. (It's really a new line but
    // the new info is written directly into buf->cursor_line.)
    buf->cursor_line.num++;
    buf->cursor_line.off += buf->cursor_col + 1;
    buf->cursor_line.len -= buf->cursor_col;
    buf->cursor_col = 0;
    buf->n_lines++;
  } else {
    buf->cursor_col++;
    buf->cursor_line.len++;
  }
  buf->gap_len--;
  buf->text_len++;
  // On insertions (when the gap gets smaller) it's necessary to check if we
  // have to create a new gap.
  check_gap(buf, 0);
}

// Insert an entire string into the gap.
// FIXME: This could be more efficient. Currently it just adds a character at a
// time. It should add chars all in one bunch. However, the current
// implementation is easy to get correct so it's OK for now.
void TRE_Buf_insert_string(TRE_Buf* buf, const char* str) {
  int i = 0;
  int c;
  while ((c = str[i++])) {
    TRE_Buf_insert_char(buf, c);
  }
}

// Delete the first character after the gap.
void TRE_Buf_delete(TRE_Buf *buf) {
  // Editing clears the column affinity.
  TRE_Buf_clear_col_affinity(buf);
  if (buf->gap_start == buf->text_len) {
    log_info("Attempted to delete at the end of the buffer.");
    return;
  }
  int c = buf->text.c[buf->gap_start + buf->gap_len];
  if (c == '\n') {
    // If a newline is being deleted, join this line with the following one.
    TRE_Line next_line = scan_next_line(buf, buf->cursor_line);
    buf->cursor_line.len += next_line.len - 1;
    buf->n_lines--;
  } else {
    buf->cursor_line.len--;
  }
  buf->gap_len++;
  buf->text_len--;
}

// Delete the last character before the gap.
void TRE_Buf_backspace(TRE_Buf *buf) {
  // Editing clears the column affinity.
  TRE_Buf_clear_col_affinity(buf);
  if (buf->gap_start == 0) {
    log_info("Attempted to backspace at the start of the buffer.");
    return;
  }
  int c = buf->text.c[buf->gap_start - 1];
  if (c == '\n') {
    // If a newline is being backspaced over, join this line with the previous
    // one.
    int cur_line_len = buf->cursor_line.len;
    buf->cursor_line = scan_prev_line(buf, buf->cursor_line);
    buf->cursor_col = buf->cursor_line.len - 1;
    buf->cursor_line.len += cur_line_len - 1;
    buf->n_lines--;
  } else {
    buf->cursor_col--;
    buf->cursor_line.len--;
  }
  buf->gap_start--;
  buf->gap_len++;
  buf->text_len--;
}

// Check if the gap needs to be expanded. This needs to be done when it
// completely runs out of space. (The "extra_space" here is space needed if
// we're going to insert a whole block of text into the buffer at once. After
// a single-character insert, extra_space is zero.)
LOCAL void check_gap(TRE_Buf *buf, int extra_space) {
  if (buf->gap_len < extra_space) {
    buf->gap_len = TRE_BUFFER_GAP_SIZE + extra_space;
    if (buf->text_len + buf->gap_len > buf->buf_size) {
      // Time to expand the buffer size to fit more text
      buf->buf_size += TRE_BUFFER_BLOCK_SIZE;
      buf->text.c = g_realloc(buf->text.c, buf->buf_size);
    }
    if (buf->gap_start < buf->text_len) {
      // Gap is before the end of the buffer, so the portion after the gap
      // needs to be relocated to enlarge the gap. (If the gap is at the end of
      // the buffer then nothing else needs to be done.
      memmove(buf->text.c + buf->gap_start + buf->gap_len,
          buf->text.c + buf->gap_start,
          buf->text_len - buf->gap_start);
    }
  }
}
