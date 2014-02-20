
#include "hdrs.c"
#include "mh_buffer.h"

#if INTERFACE
typedef struct {
  char c;
} TRE_Opts;

// XXX: Use off_t to define this?
typedef unsigned long bufpos_t;

typedef struct {
  char *filename;
  bufpos_t buf_size;
  bufpos_t text_len;
  bufpos_t gap_start;
  bufpos_t gap_len;
  unsigned int n_lines;
  unsigned int encoding; // determines char width
  // This type supports other char types, but for now only char is used.
  union {
    char *c;
    guint16 *wc;
    guint32 *wc32;
  } text;
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

// TODO: Save last file position.
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
  // TODO: Respect MAX_IN_MEMORY_FILE_SIZE
  int buf_size_blocks = (file_size + BUFFER_GAP_SIZE) / BUFFER_BLOCK_SIZE + 1;
  bufpos_t bufsize = buf_size_blocks * BUFFER_BLOCK_SIZE;
  TRE_Buf *buf = g_new(TRE_Buf, 1);
  buf->text.c = g_new(char, bufsize);
  buf->filename = g_strdup(filename);
  buf->buf_size = bufsize;
  buf->text_len = file_size;
  // Cursor starts at offset 0.
  buf->gap_start = 0;
  buf->gap_len = BUFFER_GAP_SIZE;
  // Load the file contents into the buffer.
  size_t n_read_total = 0;
  size_t insert_pos = buf->gap_start + buf->gap_len;
  do {
    ssize_t n_read = read(fd, buf->text.c + insert_pos, file_size);
    if (n_read == -1) {
      log_err("Unable to read file.");
      TRE_RT_err_msg(rt, "Unable to read file.");
      close(fd);
      return NULL;
    }
    n_read_total += n_read;
    insert_pos += n_read;
  } while (n_read_total < (size_t)file_size);
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
  check_gap(buf, 0);
}

// Delete the last character before the gap.
void TRE_Buf_backspace(TRE_Buf *buf) {
  if (buf->gap_start == 0) {
    log_info("Attempted to backspace at the start of the buffer.");
    return;
  }
  buf->gap_start--;
  buf->gap_len++;
  buf->text_len--;
}

LOCAL void check_gap(TRE_Buf *buf, size_t extra_space) {
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

// Move forward (positive) or backward (negative) in the buffer by a given number of characters.
void TRE_Buf_move(TRE_Buf* buf, ssize_t distance_chars) {
  // This movement would pass the start of the buffer.
  if (distance_chars < 0
      && buf->gap_start < (bufpos_t)(-distance_chars)) {
    log_warn("Movement attempted to pass the start of the buffer.");
    TRE_Buf_goto_byte(buf, 0);
    return;
  }
  // Movement would pass the end of the buffer.
  bufpos_t end = buf->text_len + buf->gap_len;
  if (distance_chars > 0
      && buf->gap_start + buf->gap_len + distance_chars > end) {
    log_warn("Movement attempted to pass the end of the buffer.");
    TRE_Buf_goto_byte(buf, 0);
    return;
  }
  TRE_Buf_goto_byte(buf, buf->gap_start + distance_chars);
}

// Move forward (positive) or backward (negative) in the buffer by a given number of lines.
void TRE_Buf_move_line(TRE_Buf* buf, ssize_t distance_lines) {
  // TODO: Implement this.
  logt("Moving line from (%ld), dist %ld", buf->gap_start, distance_lines);
}

// Go to an absolute position in the buffer, expressed in bytes. (Ignoring the
// space taken up by the gap.)
int TRE_Buf_goto_byte(TRE_Buf* buf, bufpos_t absolute_pos) {
  if (absolute_pos >= buf->text_len) {
    log_warn("Goto position is out of bounds, goto call ignored.");
    return 0;
  }
  // If moving to before the gap, shift the gap up.
  // --------------------------------------------|
  //      |ABSPOS          |  GAP  |             |
  // ->   |  GAP  |                |             |
  // --------------------------------------------|
  if (absolute_pos < buf->gap_start) {
    size_t block_len = buf->gap_start - absolute_pos;
    bufpos_t move_to = absolute_pos + buf->gap_len;
    bufpos_t move_from = absolute_pos;
    logt("Moving cursor LEFT from %lu to %lu"
       " (move %lu b from %lu to %lu)",
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
    size_t block_len =  absolute_pos - buf->gap_start;
    bufpos_t move_to = buf->gap_start;
    bufpos_t move_from = buf->gap_start + buf->gap_len;
    logt("Moving cursor RIGHT from %lu to %lu"
       " (move %lu b from %lu to %lu)",
       buf->gap_start, absolute_pos,
       block_len, move_from, move_to);
    memmove(buf->text.c + move_to, buf->text.c + move_from, block_len);
  }
  else {
    log_info("Moved cursor to current position, nothing to do.");
  }
  buf->gap_start = absolute_pos;
  return 1;
}

