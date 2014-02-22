#include "hdrs.c"
#include "mh_buf_io.h"

#define TRE_DEFAULT_CONFIG_DIR ".tre"
#define TRE_MARK_FILE_NAME "marks"

// Create a new (empty) buffer
TRE_Buf *TRE_Buf_new(const char *filename) {
  // TODO: It would be much better to attempt to save off data before aborting
  // the program here.
  TRE_Buf *buf = g_new0(TRE_Buf, 1);
  buf->text.c = g_new(char, TRE_BUFFER_BLOCK_SIZE);
  buf->filename = g_strdup(filename);
  buf->buf_size = TRE_BUFFER_BLOCK_SIZE;
  buf->text_len = 0;
  buf->gap_start = 0;
  buf->gap_len = TRE_BUFFER_GAP_SIZE;
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
  int buf_size_blocks =
    (file_size + TRE_BUFFER_GAP_SIZE) / TRE_BUFFER_BLOCK_SIZE + 1;
  int bufsize = buf_size_blocks * TRE_BUFFER_BLOCK_SIZE;
  TRE_Buf *buf = g_new(TRE_Buf, 1);
  buf->text.c = g_new(char, bufsize);
  buf->filename = g_strdup(filename);
  buf->buf_size = bufsize;
  buf->text_len = file_size;
  // Cursor starts at offset 0.
  buf->gap_start = 0;
  buf->gap_len = TRE_BUFFER_GAP_SIZE;
  // For now, assuming start at line 0. Whatever line number is set here is
  // searched for below and its bounds will be saved in buf->cursor_line.
  buf->cursor_line.len = 0;
  buf->cursor_col = 0;
  buf->n_lines = 0; // This is initialized here, but updated below.
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
        // If this ends the line containing the cursor, calculate the line length.
        if (buf->n_lines == buf->cursor_line.num) {
          int next_line_begin_pos = n_read_total + i + 1;
          buf->cursor_line.len = next_line_begin_pos - buf->cursor_line.off;
        }
        // Keep track of total lines in the file.
        buf->n_lines++;
        // If this begins the line containing the cursor, save the offset.
        if (buf->n_lines == buf->cursor_line.num) {
          int line_begin_pos = n_read_total + i + 1;
          buf->cursor_line.off = line_begin_pos;
        }
      }
    }
    // Update counts
    n_read_total += n_read;
    insert_pos += n_read;
  } while (n_read_total < file_size);
  // If the file isn't newline-terminated, add a newline at the end.
  if (buf->text.c[buf->text_len + buf->gap_len - 1] != '\n') {
    buf->text.c[++buf->text_len + buf->gap_len - 1] = '\n';
    buf->n_lines++;
  }
  // Assume ASCII as the default (only) encoding for now.
  buf->encoding = TRE_BUF_ENCODING_ASCII;
  logt("File loaded: %s", filename);
  return buf;
}

#if 0
LOCAL line_col_t read_file_mark(const char *filename) {
  const char *mark_file_name = find_config_file(TRE_MARK_FILE_NAME);
}

const char* TRE_find_config_file(const char* filename) {
  const char* config_dir = TRE_DEFAULT_CONFIG_DIR;
}
#endif

