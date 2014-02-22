
#include "hdrs.c"
#include "mh_buffer.h"

// #define LOG_DRAWING

#if INTERFACE
typedef struct {
  char c;
} TRE_Opts;

typedef struct TRE_Line {
  int num; // line number of this line
  int off; // offset of first char in the line from start of file
  int len; // length of line (including newline)
} TRE_Line;

typedef struct {
  // The filename string will be freed when the buffer is destroyed.
  char *filename;  // name of disk file for buffer (NULL if none)
  // This type supports other char types, but for now only char is used.
  union { // the actual text buffer, which can use different pointer types
    char *c;
    guint16 *wc;
    guint32 *wc32;
  } text;
  int buf_size;    // space actually allocated for the buffer currently
  int text_len;    // length of text in chars (excluding gap)
  int gap_start;   // offset of the start of the gap
  int gap_len;     // length of the gap
  int n_lines;     // total number of lines in text
  int encoding;    // determines char width
  int cursor_col;  // cursor position, column
  TRE_Line cursor_line; // position info about the line where the cursor is
} TRE_Buf;

// High byte is an encoding ID, low byte is the width (8, 16 or 32 bits).
// (ASCII is really a 7-byte encoding, but char width is still 8 bits.)
// TODO: Support additional 8-byte encodings.
#define TRE_BUF_ENCODING_ASCII ((1 << 8) |  8)
#define TRE_BUF_ENCODING_UTF16 ((2 << 8) | 16)
#define TRE_BUF_ENCODING_UTF32 ((3 << 8) | 32)
#endif

// Size increment for managing the buffer.
#define BUFFER_BLOCK_SIZE 1024
// Size of the gap when it's created or moved.
#define BUFFER_GAP_SIZE BUFFER_BLOCK_SIZE
// TODO: Respect this. For larger files, create a memory mapped swap file.
// Size is 64 MB.
#define MAX_IN_MEMORY_FILE_SIZE (64 * 1024 * 1024)

// Create a new (empty) buffer
TRE_Buf *TRE_Buf_new(const char *filename) {
  // TODO: It would be much better to attempt to save off data before aborting
  // the program here.
  TRE_Buf *buf = g_new(TRE_Buf, 1);
  buf->text.c = g_new(char, BUFFER_BLOCK_SIZE);
  buf->filename = g_strdup(filename);
  buf->buf_size = BUFFER_BLOCK_SIZE;
  buf->text_len = 0;
  buf->gap_start = 0;
  buf->gap_len = BUFFER_GAP_SIZE;
  buf->n_lines = 0;
  buf->encoding = TRE_BUF_ENCODING_ASCII;
  return buf;
}

// TODO: Save/load last file position.
// TODO: Strip CR chars from file as it loads.
TRE_Buf *TRE_Buf_load(TRE_RT *rt, const char *filename) {
  struct stat fstat_buf;
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    TRE_RT_err_msg(rt, "Unable to open file.");
    return NULL;
  }
  fstat(fd, &fstat_buf);
  off_t file_size = fstat_buf.st_size;
  if (file_size == 0) {
    // Loading an empty file is pretty much like creating a new buffer.
    close(fd);
    logt("Loading empty file.");
    return TRE_Buf_new(filename);
  }
  else if (file_size > INT_MAX) {
    log_err("File is too large to open on this system.");
    return NULL;
  }
  // TODO: Respect MAX_IN_MEMORY_FILE_SIZE
  int buf_size_blocks = (file_size + BUFFER_GAP_SIZE) / BUFFER_BLOCK_SIZE + 1;
  int bufsize = buf_size_blocks * BUFFER_BLOCK_SIZE;
  TRE_Buf *buf = g_new(TRE_Buf, 1);
  buf->text.c = g_new(char, bufsize);
  buf->filename = g_strdup(filename);
  buf->buf_size = bufsize;
  buf->text_len = file_size;
  // Cursor starts at offset 0.
  buf->gap_start = 0;
  buf->gap_len = BUFFER_GAP_SIZE;
  buf->cursor_line = 0;
  buf->cursor_col = 0;
  buf->n_lines = 0; // this will be set below
  // Load the file contents into the buffer. (The call to read isn't guaranteed
  // to return all the requested data the first time it's called.)
  int n_read_total = 0;
  int insert_pos = buf->gap_start + buf->gap_len;
  do {
    // Read as much as possible from the file in one go
    ssize_t n_read = read(fd, buf->text.c + insert_pos, file_size);
    if (n_read == -1) {
      log_err("Unable to read file.");
      TRE_RT_err_msg(rt, "Unable to read file.");
      close(fd);
      return NULL;
    }
    // Count the lines in the portion just read.
    // TODO: While we're at it, find the position for the cursor.
    // (But for now we don't remember cursor position.)
    for (int i=0; i < n_read; i++) {
      if ('\n' == *(buf->text.c + insert_pos + i)) {
        buf->n_lines++;
      }
    }
    // Update counts
    n_read_total += n_read;
    insert_pos += n_read;
  } while (n_read_total < file_size);
  buf->n_lines = 0;
  buf->encoding = TRE_BUF_ENCODING_ASCII;
  logt("File loaded.");
  return buf;
}

// Insert a character into the gap.
void TRE_Buf_insert_char(TRE_Buf *buf, char c) {
  buf->text.c[buf->gap_start++] = c;
  buf->gap_len--;
  buf->text_len++;
  if (c == '\n') {
    buf->cursor_col = 0;
    buf->cursor_line++;
  }
  else {
    buf->cursor_col++;
  }
  check_gap(buf, 0);
}

// Delete the last character before the gap.
void TRE_Buf_delete(TRE_Buf *buf) {
  if (buf->gap_start == buf->text_len) {
    log_info("Attempted to backspace at the end of the buffer.");
    return;
  }
  buf->gap_len++;
  buf->text_len--;
}

// Delete the last character before the gap.
void TRE_Buf_backspace(TRE_Buf *buf) {
  if (buf->gap_start == 0) {
    log_info("Attempted to backspace at the start of the buffer.");
    return;
  }
  int c = buf->text.c[buf->gap_start - 1];
  if (c == '\n') {
    buf->cursor_line--;
    // TODO: Find cursor col
  }
  else {
    buf->cursor_col--;
  }
  buf->gap_start--;
  buf->gap_len++;
  buf->text_len--;
}

LOCAL void check_gap(TRE_Buf *buf, int extra_space) {
  // Check if the gap needs to be expanded. This needs to be done when it
  // completely runs out of space. (The "extra_space" here is space needed if
  // we're going to insert a whole block of text into the buffer at once. After
  // a single-character insert, extra_space is zero.)
  if (buf->gap_len < extra_space) {
    buf->gap_len = BUFFER_GAP_SIZE + extra_space;
    if (buf->text_len + buf->gap_len > buf->buf_size) {
      // Time to expand the buffer size to fit more text
      buf->buf_size += BUFFER_BLOCK_SIZE;
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

#if INTERFACE
enum move_linewrap_style_t {
  MOVE_LINEWRAP_NO,
  MOVE_LINEWRAP_YES
};
#endif

// Move forward (positive) or backward (negative) in the buffer by a given
// number of characters.
// TODO: Parameterize line wraparound behavior.
void TRE_Buf_move(TRE_Buf* buf, int distance_chars) {
  const enum move_linewrap_style_t linewrap_style = MOVE_LINEWRAP_YES;
  // This movement would pass the start of the buffer.
  if (distance_chars < 0
      && buf->gap_start < -distance_chars) {
    log_warn("Movement attempted to pass the start of the buffer.");
    return;
  }
  // Movement would pass the end of the buffer.
  int end = buf->text_len + buf->gap_len;
  if (distance_chars > 0
      && buf->gap_start + buf->gap_len + distance_chars > end) {
    log_warn("Movement attempted to pass the end of the buffer.");
    return;
  }
  logt("Moving character from %d, dist %d.", buf->gap_start, distance_chars);
  // Scan the text to see where the cursor will end up.
  if (distance_chars > 0) {
    mv_curs_right_charwise(buf, distance_chars, linewrap_style);
    TRE_Buf_move_gap(buf, buf->gap_start + distance_chars);
  }
  else if (distance_chars < 0) {
    mv_curs_left_charwise(buf, -distance_chars, linewrap_style);
  }
  logt("Cursor position: %d, %d", buf->cursor_line, buf->cursor_col);
}

LOCAL TRE_OpResult mv_curs_right_charwise(TRE_Buf* buf,
    int distance_chars, move_linewrap_style_t linewrap_style) {
  int pos = buf->gap_start + buf->gap_len;
  int end = pos + distance_chars;
  int buf_end = buf->text_len + buf->gap_len - 1;
  if (buf_end < end) {
    end = buf_end;
  }
  while (pos < end) {
    logt("Advancing one character.");
    if (buf->text.c[pos++] == '\n') {
      logt("Reached newline.");
      if (linewrap_style == MOVE_LINEWRAP_NO) {
        logt("Not wrapping to next line due to setting.");
        break;
      }
      logt("Wrapping to next line due to setting.");
      buf->cursor_line++;
      buf->cursor_col = 0;
    }
    else {
      buf->cursor_col++;
    }
  }
  return TRE_SUCC;
}

LOCAL TRE_OpResult mv_curs_left_charwise(TRE_Buf* buf, int n_chars,
    move_linewrap_style_t linewrap_style) {
  // If the entire move will be within the current line, no need to inspect any
  // intermediate characters along the way.
  if (n_chars <= buf->cursor_col) {
    logt("Simple move (within line)");
    buf->cursor_col -= n_chars;
    TRE_Buf_move_gap(buf, buf->gap_start - n_chars);
    return TRE_SUCC;
  }
  // Otherwise, begin by backing up to the beginning of the line, then one more
  // character beyond onto the newline that terminates the previous line.
  int pos = buf->gap_start - buf->cursor_col - 1;
  int chars_left_to_move = n_chars - buf->cursor_col - 1;
  int lines_moved = 1;
  // If a horizontal move shouldn't cross line bounds, then stop movement at
  // the first character in the line.
  if (linewrap_style == MOVE_LINEWRAP_NO) {
    logt("Stopped at beginning of line (MOVE_LINEWRAP_NO)");
    buf->cursor_col = 0;
    TRE_Buf_move_gap(buf, pos + 1);
    return TRE_SUCC;
  }
  // From here on, we have to actually scan the characters to find the newline
  // terminating each previous line. (Or the beginning of the file.)
  while (chars_left_to_move >= 0 && pos > 0) {
    logt("Scanning line from %d", pos);
    // Scan back to find where the line starts
    int next_pos = find_line_precedent(buf, pos);
    logt("Scan ended at %d", pos);
    // Subtract the distance moved from the chars left to move
    chars_left_to_move -= pos - next_pos;
    // Continue until we've moved far enough or reached 0
    pos = next_pos;
    lines_moved++;
  }
  // By now there's a good chance we've overshot the mark in looking for the
  // position we want. The (negative) value of chars_left_to_move tells us how
  // much we overshot by. It also tells us the column number of the new position.
  // Since we passed the beginning of the line, if we the position forward
  // again we also cross back over the newline, so in that case (if
  // chars_left_to_move < 0) then we also decrease the number of lines moved by
  // one to account for this.
  logt("Destination overshoot %d, lines moved %d", -chars_left_to_move,
      lines_moved);
  int final_pos = pos - chars_left_to_move;
  // Set the line number according to how many lines we moved.
  if (chars_left_to_move != 0) {
    lines_moved--;
  }
  buf->cursor_line = buf->cursor_line - lines_moved;
  // Set the column number.
  if (final_pos == 0) {
    // The cursor won't be handled quite right when we reach the beginning of
    // the buffer because we don't overshoot it the way we do with line
    // boundaries. Just assign 0 for the column number since we know that's
    // what it should be.
    buf->cursor_col = 0;
  }
  else {
    // Additional -1 to col because col numbering starts from zero.
    buf->cursor_col = -chars_left_to_move - 1;
  }
  TRE_Buf_move_gap(buf, final_pos);
  return TRE_SUCC;
}

// Scan backward through the file far for a specified number of lines.  Returns
// the position after moving back that many lines. Pos will be at the character
// just before the previous line, which means it's on a newline character, or
// at the beginning of the file it's at position -1 (a non-location that should
// not actually be read). The output parameter n_lines_moved will return
// containing the number of lines that were actually moved, which will be the
// same as n_lines_to_move unless the beginning of the file was reached, in
// which case it will be the number of newlines passed on the way to the
// beginning of the file. (It follows that this value is only relevant if the
// end position is -1.)
// FIXME: It never makes sense to back up past the beginning of the file,
// because are maintaining a row/col position so we know which line we are in.
// If this is line 0, it's not necessary to scan the chars to know that no more
// newlines will be seen before the beginning of the file.
LOCAL int back_up_n_lines(TRE_Buf* buf, int n_lines_to_move, int *n_lines_moved) {
  int pos = buf->gap_start;
  int lines_left_to_move = n_lines_to_move;
  while (lines_left_to_move > 0 && pos > 0) {
    logt("Scanning line from %d", pos);
    pos = find_line_precedent(buf, pos);
    logt("Scan ended at %d", pos);
    lines_left_to_move--;
  }
  // Figure the number of lines that were actually moved, which can be less
  // than the number requsted if the beginning of the file was reached.
  *n_lines_moved = n_lines_to_move - lines_left_to_move;
  return pos;
}

LOCAL TRE_Line scan_prev_line(TRE_Buf* buf, TRE_Line from_line) {
  TRE_Line prev_line;
  assert(from_line.num > 0); // don't call if this is line zero
  // Keeping track of the line number is straightforward.
  prev_line.num = from_line - 1;
  // Check if this is the first line.
  if (prev_line.num == 0) {
    // Don't scan line zero because we know there will be no newline.
    prev_line.off = 0;
    prev_line.len = from_line.off;
  } else {
    // Scan backward to find the next newline. It's not necessary to check for
    // hitting the start of the buffer because we've already stipulated that
    // this isn't line zero.
    int pos = from_line.off - 1;
    do {} while (buf->text.c[--pos] != '\n');
    prev_line.off = pos + 1;
    prev_line.len = from_line.off - prev_line.off;
  }
  return prev_line;
}

LOCAL TRE_Line scan_next_line(TRE_Buf* buf, TRE_Line from_line) {
  TRE_Line next_line;
  assert(from_line.num < buf->n_lines); // don't call if this is the last line
  // Keeping track of the line number is straightforward.
  next_line.num = from_line + 1;
  // The next line offset is 1 past the end of the previous line.
  next_line.off = from_line.off + from_line.len;
  // Check if this is the last line.
  if (next_line.num == buf->n_lines) {
    // Don't scan last line because we know it ends at end of buffer.
    next_line.len = buf->text_len - next_line.off;
  } else {
    // Scan forward to find the next newline. It's not necessary to check for
    // hitting the end of the buffer because we've already stipulated that
    // this isn't the last line.
    int pos = next_line.off;
    do {} while (buf->text.c[++pos] != '\n');
    next_line.len = (pos + 1) - next_line.off;
  }
  return next_line;
}

// Move cursor backward (up) linewise.
LOCAL TRE_OpResult mv_curs_up_linewise(TRE_Buf* buf, int distance_lines) {
  int lines_left_to_move = -distance_lines;
  int pos = buf->gap_start;
  while (lines_left_to_move > 0) {
    while (--pos >= 0) {
      logt("Regressed one character.");
      if (buf->text.c[pos] == '\n') {
        logt("Reached newline.");
        break;
      }
    }
    if (pos < 0) {
      pos++;
      break;
    }
    buf->cursor_line--;
    lines_left_to_move--;
    logt("Regressed one line.");
  }
  // After moving up the requisite number of lines, search backward to find
  // the beginning of the line, then set the correct column.
  resolve_curs_col(buf, &pos);    
  // Make the actual jump with the cursor.
  TRE_Buf_move_gap(buf, pos);
  return TRE_SUCC;
}

// Move forward (positive) or backward (negative) in the buffer by a given
// number of lines.
// TODO: Parameterize selection of the column to land on after moving.
void TRE_Buf_move_linewise(TRE_Buf* buf, int distance_lines) {
  logt("Moving line from (%d), dist %d", buf->gap_start, distance_lines);
  // TODO: Track persistent "preferred" column across multiple moves.
  // Moving forward and backward are subtly different, so they
  // are handled as distinct cases.
  if (distance_lines > 0) {
    mv_curs_down_linewise(buf, distance_lines);
  }
  else if (distance_lines < 0) {
    mv_curs_up_linewise(buf, distance_lines);
  }
  else {
    log_info("Linewise move called for a distance of zero.");
  }
}

// Move the cursor forward (down), linewise.
LOCAL TRE_OpResult mv_curs_down_linewise(TRE_Buf* buf, int distance_lines) {
  int lines_left_to_move = distance_lines;
  int pos = buf->gap_start + buf->gap_len;
  int end = buf->text_len + buf->gap_len;
  while (lines_left_to_move > 0) {
    logt("Begin scanning line (pos=%d, end=%d).", pos, end);
    while (pos < end) {
      // logt("Advancing one character.");
      if (buf->text.c[pos++] == '\n') {
        logt("Reached newline.");
        break;
      }
    }
    if (pos == end) {
      break;
    }
    buf->cursor_line++;
    lines_left_to_move--;
    logt("Advanced one line.");
  }
  // After moving down the requisite number of lines, advance to the correct
  // column, or to the end of the line/file, whichever comes first.
  // Exception: if at the end of the file, we're actually past the
  // destination column, so we need to back up to it.
  if (pos < end) {
    int cols_left_to_move = buf->cursor_col;
    while (pos < end && cols_left_to_move > 0) {
      if (buf->text.c[pos] == '\n') {
        break;
      }
      pos++, cols_left_to_move--;
    }
    buf->cursor_col -= cols_left_to_move;
  }
  else {
    resolve_curs_col(buf, &pos);
  }
  // Make the actual jump with the cursor.
  TRE_Buf_move_gap(buf, pos - buf->gap_len);
  return TRE_SUCC;
}

// Move cursor backward (up) linewise.
LOCAL TRE_OpResult mv_curs_up_linewise(TRE_Buf* buf, int distance_lines) {
  int lines_left_to_move = -distance_lines;
  int pos = buf->gap_start;
  while (lines_left_to_move > 0) {
    while (--pos >= 0) {
      logt("Regressed one character.");
      if (buf->text.c[pos] == '\n') {
        logt("Reached newline.");
        break;
      }
    }
    if (pos < 0) {
      pos++;
      break;
    }
    buf->cursor_line--;
    lines_left_to_move--;
    logt("Regressed one line.");
  }
  // After moving up the requisite number of lines, search backward to find
  // the beginning of the line, then set the correct column.
  resolve_curs_col(buf, &pos);    
  // Make the actual jump with the cursor.
  TRE_Buf_move_gap(buf, pos);
  return TRE_SUCC;
}

// Based on a position in the file, scan back to the beginning of the line to
// find the newline (or file edge) that precedes it. Return the position of the
// character before the first in this line. Since this function scans backward
// and doesn't check if it's passing the gap, it's only valid if called with a
// position that precedes the gap.
// FIXME: Don't back up when in line 0. Not sure if this should be handled by
// short-circuiting the scan in this function, or by asserting that this
// funciton shouldn't be called in that condition.
LOCAL int find_line_precedent(TRE_Buf* buf, int pos) {
  logt("find_line_precedent: pos=%d; gap_start=%d", pos, buf->gap_start);
  assert(pos <= buf->gap_start); // function is only valid before the gap
  while (--pos >= 0) {
    if (buf->text.c[pos] == '\n') {
      break;
    }
  }
  return pos;
}

// Based on a position in the file, scan back to the beginning of the line to
// determine what column the cursor should be in to at that position. This
// function assumes that gap_start is greater than or equal to pos.
LOCAL TRE_OpResult resolve_curs_col(TRE_Buf* buf, int *pos) {
  logt("resolve_curs_col: pos=%d; gap_start=%d", pos, buf->gap_start);
  assert(pos <= buf->gap_start); // function is only valid before the gap
  // If the cursor is on line 0 then there's no need to scan backward because
  // the column position and the absolute position are the same..
  if (0 == buf->cursor_line) {
  }
  int line_length = 0;
  while (--*pos >= 0) {
    if (buf->text.c[*pos] == '\n') {
      break;
    }
    line_length++;
  }
  (*pos)++; // went 1 past the beginning of the line, so return to the start
  int new_cursor_col = buf->cursor_col < line_length
    ? buf->cursor_col
    : line_length;
  logt("Skipping ahead to col: %d", new_cursor_col);
  *pos += new_cursor_col;
  buf->cursor_col = new_cursor_col;
  return TRE_SUCC;
}

/*
TRE_OpResult TRE_Buf_goto_line_and_col(TRE_Buf* buf, unsigned line, unsigned col) {
  // Track the line and column position through the portion just read.
  for (int i=0; i < n_read; i++) {
    if ('\n' == *(buf->text.c + insert_pos + i)) {
      buf->cursor_line++;
      buf->cursor_col = 0;
    } else {
      buf->cursor_col++;
    }
  }
}
*/

// Go to an absolute position in the buffer, expressed in bytes. (Ignoring the
// space taken up by the gap.)
TRE_OpResult TRE_Buf_move_gap(TRE_Buf* buf, int absolute_pos) {
  if (absolute_pos >= buf->text_len) {
    log_warn("Goto position is out of bounds, goto call ignored.");
    return TRE_FAIL;
  }
  // If moving to before the gap, shift the gap up.
  // --------------------------------------------|
  //      |ABSPOS          |  GAP  |             |
  // ->   |  GAP  |                |             |
  // --------------------------------------------|
  if (absolute_pos < buf->gap_start) {
    int block_len = buf->gap_start - absolute_pos;
    int move_to = absolute_pos + buf->gap_len;
    int move_from = absolute_pos;
    logt("Moving cursor LEFT from %d to %d"
       " (move %d b from %d to %d)",
       buf->gap_start, absolute_pos,
       block_len, move_from, move_to);
    memmove(buf->text.c + move_to, buf->text.c + move_from, block_len);
  }
  // If the target is after the gap, shift the gap down.
  // --------------------------------------------|
  //      |  GAP  |                |ABSPOS       |
  // ->   |                |  GAP  |             |
  // --------------------------------------------|
  else if (absolute_pos > buf->gap_start) {
    // Remember in these calculations that the value of absolute_pos doesn't
    // account for the gap size. (In other words, the actual new gap_start
    // after this move will be at absolute_pos, not absolute_pos + gap_len.)
    int block_len =  absolute_pos - buf->gap_start;
    int move_to = buf->gap_start;
    int move_from = buf->gap_start + buf->gap_len;
    logt("Moving cursor RIGHT from %d to %d"
       " (move %d b from %d to %d)",
       buf->gap_start, absolute_pos,
       block_len, move_from, move_to);
    memmove(buf->text.c + move_to, buf->text.c + move_from, block_len);
  }
  else {
    log_info("Moved cursor to current position, nothing to do.");
  }
  buf->gap_start = absolute_pos;
  return TRE_SUCC;
}

// TODO: Most of this logic should really be in buffer.c, since it's
// buffer-related logic. Only buffer.c should be dealing with offsets and
// especially with the gap.
void TRE_Buf_draw(TRE_Buf *buf, int winsz_y, int winsz_x,
    int view_start_pos, TRE_Win *win) {
  logt("Trying to draw window");
  int pos = view_start_pos;
  int end = buf->text_len + buf->gap_len;
  unsigned cursor_x, cursor_y;
  // Print as many lines as we have room for in the window
  for (int y=0; y < winsz_y; y++) {
    // print line, wrapping around
    if (pos == end) {
#ifdef LOG_DRAWING
      logt("Reached end of file.");
#endif
      break;
    }
#ifdef LOG_DRAWING
    logt("Printing a line.");
#endif
    int x=0;
    while (y < winsz_y && pos < end) {
      if (pos == buf->gap_start) {
#ifdef LOG_DRAWING
        logt("At cursor position.");
#endif
        // Skip over the gap in the buffer
        pos += buf->gap_len;
        cursor_x = x, cursor_y = y;
      }
      // Get the char from the buffer, and advance pos
      int c = buf->text.c[pos++];
      // If this is a newline, do a CR-LF operation
      if (c == '\n') {
#ifdef LOG_DRAWING
        logt("Reached newline char.");
#endif
        x = 0;
        break;
      }
      // Display the character
      if (TRE_FAIL == TRE_Win_display_char(win, y, x, c)) {
        // Failed to draw to screen, don't know why.
        // TODO: do a proper assertion
        log_err("Failed to display character on screen.");
        exit(1);
      } else {
        // logt("'%c' @ (%d, %d)", c, x, y);
      }
      x++;
      if (x == winsz_x) {
        x = 0, y++;
#ifdef LOG_DRAWING
        logt("Wrapping around.");
#endif
      }
    }
  }
  TRE_Win_move_cursor(win, cursor_y, cursor_x);
}

int TRE_Buf_get_cursor_offset(TRE_Buf* buf) {
  return buf->gap_start;
}

