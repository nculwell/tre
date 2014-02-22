
#include "hdrs.c"
#include "mh_buf_main.h"

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
  int col_affinity; // col that vertical move should land on if possible
  TRE_Line cursor_line; // position info about the line where the cursor is
} TRE_Buf;

// High byte is an encoding ID, low byte is the width (8, 16 or 32 bits).
// (ASCII is really a 7-byte encoding, but char width is still 8 bits.)
// TODO: Support additional 8-byte encodings.
#define TRE_BUF_ENCODING_ASCII ((1 << 8) |  8)
#define TRE_BUF_ENCODING_UTF16 ((2 << 8) | 16)
#define TRE_BUF_ENCODING_UTF32 ((3 << 8) | 32)

enum move_linewrap_style_t {
  MOVE_LINEWRAP_NO,
  MOVE_LINEWRAP_YES
};

// Size increment for managing the buffer.
#define TRE_BUFFER_BLOCK_SIZE 1024
// Size of the gap when it's created or moved.
#define TRE_BUFFER_GAP_SIZE TRE_BUFFER_BLOCK_SIZE
#endif

#define LOG_CURSOR_POSITION() \
  logt("Cursor: (aff=%d) LC=%d,%d; line off=%d," \
     " len=%d; gap start=%d", \
      buf->col_affinity, buf->cursor_line.num, buf->cursor_col, \
      buf->cursor_line.off, buf->cursor_line.len, buf->gap_start);

// Move forward (positive) or backward (negative) in the buffer by a given
// number of characters.
// TODO: Parameterize line wraparound behavior.
void TRE_Buf_move_charwise(TRE_Buf* buf, int distance_chars) {
  const enum move_linewrap_style_t linewrap_style = MOVE_LINEWRAP_YES;
  // Sideward movement clears the column affinity.
  TRE_Buf_clear_col_affinity(buf);
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
  }
  else if (distance_chars < 0) {
    mv_curs_left_charwise(buf, -distance_chars, linewrap_style);
  }
  //logt("Cursor position: %d, %d", buf->cursor_line, buf->cursor_col);
  LOG_CURSOR_POSITION();
}

LOCAL TRE_OpResult mv_curs_right_charwise(TRE_Buf* buf,
    int n_chars, move_linewrap_style_t linewrap_style) {
  TRE_Line line = buf->cursor_line;
  // Figure movement from the beginning of the line to make looping over
  // subsequent lines simpler. This means we "back up" to the beginning by
  // adding the cursor column number to the number of chars to be moved.
  int chars_left_to_move = n_chars + buf->cursor_col;
  int last_line_num = buf->n_lines - 1;
  // Don't do any linewise movement if settings prevent it.
  if (linewrap_style != MOVE_LINEWRAP_NO) {
    // Count characters from the beginning of this line to the beginning of the
    // next, going until enough chars have been crossed to satisfy the move
    // distance.
    logt("Moving right from line %d, last line = %d", line.num, last_line_num);
    while (line.num < last_line_num) {
      chars_left_to_move -= line.len;
      if (chars_left_to_move < 0) {
        break;
      }
      line = scan_next_line(buf, line);
    }
  }
  if (chars_left_to_move >= 0) {
    // Movement stopped because the last line was reached, or because settings
    // prevent movement from wrapping to the next line.
    if (chars_left_to_move + 1 > line.len) {
      // Requested distance takes us past the end of the line.
      logt("Move right charwise passes end of buffer, limiting to end of line.");
      buf->cursor_col = line.len - 1;
    } else {
      // The movement lands us somewhere inside the bounds of the last line.
      logt("Move right charwise ends within last line.");
      buf->cursor_col = chars_left_to_move;
    }
  } else {
    // In normal movement (not at the end of the buffer), chars_left_to_move
    // ends up negative. In that case, its value tells us how far we have to
    // back up from the end of the current line to get to the correct position.
    logt("Move right charwise ends before last line.");
    buf->cursor_col = line.len + chars_left_to_move;
  }
  buf->cursor_line = line;
  TRE_Buf_move_gap(buf, line.off + buf->cursor_col);
  return TRE_SUCC;
}

LOCAL TRE_OpResult mv_curs_left_charwise(TRE_Buf* buf,
    int n_chars, move_linewrap_style_t linewrap_style) {
  TRE_Line line = buf->cursor_line;
  // Figure movement from the beginning of the line to make looping over
  // subsequent lines simpler. We do this "backing up" by adjusting the value
  // of chars_left_to_move accordingly. The notion of being at the start of
  // each line is just an assumption in the following code.
  int chars_left_to_move = n_chars - buf->cursor_col;
  // Don't do any linewise movement if settings prevent it.
  if (linewrap_style != MOVE_LINEWRAP_NO) {
    // Count characters from the beginning of this line to the beginning of the
    // next, going until enough chars have been crossed to satisfy the move
    // distance.
    while (line.num > 0) {
      if (chars_left_to_move <= 0) {
        break;
      }
      line = scan_next_line(buf, line);
      chars_left_to_move -= line.len;
    }
  }
  if (chars_left_to_move > 0) {
    // Movement stopped because the first line was reached, or because settings
    // prevent movement from wrapping to the next line.
    if (chars_left_to_move >= line.len) {
      // Requested distance takes us past the start of the line.
      buf->cursor_col = 0;
    } else {
      // The movement lands us somewhere inside the bounds of the line.
      buf->cursor_col = line.len - chars_left_to_move - 1;
    }
  } else {
    // In normal movement (not at the end of the buffer), chars_left_to_move
    // ends up <= 0. In that case, its value tells us how far we have to move
    // rightward from the beginning of the line to get to the right position.
    buf->cursor_col = -chars_left_to_move;
  }
  buf->cursor_line = line;
  TRE_Buf_move_gap(buf, line.off + buf->cursor_col);
  return TRE_SUCC;
}

// The functions scan_prev_line and scan_next_line are the core of the movement
// code. These functions keep track of the line/column count and respect the
// details of the buffer data structure, particularly including the gap. Other
// movement functions should be implemented in terms of lines, with these
// functions being used to move between lines.

// Scan chars to find the position and length of the line previous to the one
// given. It is illegal to call this on the first line of the buffer (i.e. when
// no previous line exists).
TRE_Line scan_prev_line(TRE_Buf* buf, TRE_Line from_line) {
  TRE_Line prev_line;
  assert(from_line.num > 0); // don't call if this is line zero
  // Keeping track of the line number is straightforward.
  prev_line.num = from_line.num - 1;
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
    // Scanning works differently depending on whether we might encounter gap.
    if (pos < buf->gap_start) {
      // Scanning before the gap. Don't need to account for gap size.
before_gap:
      do {} while (buf->text.c[--pos] != '\n');
    } else {
      // Scanning after the gap. Account for the gap size, and skip past the
      // gap if we reach it.
      int chars_until_gap = pos - buf->gap_start;
      pos += buf->gap_len;
      do {
        ++pos;
        --chars_until_gap;
        if (0 == chars_until_gap) {
          pos -= buf->gap_len;
          goto before_gap;
        }
      } while (buf->text.c[pos] != '\n');
      pos -= buf->gap_len;
    }
    prev_line.off = pos + 1;
    prev_line.len = from_line.off - prev_line.off;
  }
  return prev_line;
}

TRE_Line scan_next_line(TRE_Buf* buf, TRE_Line from_line) {
  TRE_Line next_line;
  assert(from_line.num + 1 < buf->n_lines); // don't call if curs in last line
  // Keeping track of the line number is straightforward.
  next_line.num = from_line.num + 1;
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
    // Scanning works differently depending on whether we might encounter gap.
    if (pos >= buf->gap_start) {
      // Scanning after the gap. Account for the gap size. (Line offsets
      // are file positions, not buffer positions, i.e. they pretend that the
      // file text is contiguous from start to finish.)
after_gap:
      pos += buf->gap_len;
      do {} while (buf->text.c[++pos] != '\n');
      pos -= buf->gap_len;
    } else {
      // Scanning before gap, have to switch scanning mode if we reach it.
      int chars_until_gap = buf->gap_start - pos;
      do {
        ++pos;
        --chars_until_gap;
        if (0 == chars_until_gap) {
          goto after_gap; // goto considered awesome
        }
      } while (buf->text.c[pos] != '\n');
    }
    next_line.len = (pos + 1) - next_line.off;
  }
  return next_line;
}

// Move forward (positive) or backward (negative) in the buffer by a given
// number of lines.
// TODO: Parameterize selection of the column to land on after moving.
void TRE_Buf_move_linewise(TRE_Buf* buf, int distance_lines) {
  logt("Moving line from (%d), dist %d", buf->gap_start, distance_lines);
  if (distance_lines == 0) {
    log_info("Linewise move called for a distance of zero.");
    return;
  }
  TRE_Line line = buf->cursor_line;
  logt("Start move at line: num %d, off %d, len %d",
      line.num, line.off, line.len);
  if (distance_lines > 0) { // moving forward/down
    int last_line = buf->n_lines - 1;
    for (int ln = distance_lines; ln > 0; ln--) {
      logt("Moving one line down.");
      if (line.num == last_line) {
        logt("Move prevented because this is the last line.");
        break;
      }
      line = scan_next_line(buf, line);
      logt("Scanned next line: num %d, off %d, len %d",
          line.num, line.off, line.len);
    }
  } else { // distance_lines is negative (moving backward)
    for (int ln = -distance_lines; ln > 0; ln--) {
      logt("Moving one line up.");
      if (line.num == 0) {
        logt("Move prevented because this is the first line.");
        break;
      }
      line = scan_prev_line(buf, line);
      logt("Scanned prev line: num %d, off %d, len %d",
          line.num, line.off, line.len);
    }
  }
  // Set the cursor column affinity if unset.
  if (buf->col_affinity == -1) {
    logt("Setting col affinity to match cursor col (%d)", buf->cursor_col);
    buf->col_affinity = buf->cursor_col;
  }
  // Set the cursor position.
  buf->cursor_col = buf->col_affinity < line.len
    ? buf->col_affinity
    : line.len - 1;
  buf->cursor_line = line;
  // Move the buffer gap.
  TRE_Buf_move_gap(buf, line.off + buf->cursor_col);
  LOG_CURSOR_POSITION();
}

// Clear the cursor column affinity. This should be done whenever the cursor
// moves in any fashion that's not up/down linewise.
void TRE_Buf_clear_col_affinity(TRE_Buf* buf) {
  if (buf->col_affinity != -1) {
    buf->col_affinity = -1;
  }
}

// Go to an absolute position in the buffer, expressed in bytes. (Ignoring the
// space taken up by the gap.)
// TODO: This needs to account for line numbers.
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
    logt("Moving gap LEFT from %d to %d"
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
    logt("Moving gap RIGHT from %d to %d"
       " (move %d b from %d to %d)",
       buf->gap_start, absolute_pos,
       block_len, move_from, move_to);
    memmove(buf->text.c + move_to, buf->text.c + move_from, block_len);
  }
  else {
    log_info("Moved gap to current position, nothing to do.");
  }
  buf->gap_start = absolute_pos;
  return TRE_SUCC;
}

// TODO: Most of this logic should really be in buffer.c, since it's
// buffer-related logic. Only buffer.c should be dealing with offsets and
// especially with the gap.
void TRE_Buf_draw(TRE_Buf *buf, int winsz_y, int winsz_x,
    int view_start_pos, TRE_Win *win) {
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
