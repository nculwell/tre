#include "hdrs.c"
#include "mh_buf_io.h"

#if INTERFACE
typedef struct {
  int line;
  int col;
} line_col_t;
#endif

#define TRE_DEFAULT_CONFIG_DIR ".tre"
#define TRE_MARK_FILE_NAME "marks"
#define TRE_SAVED_POSITIONS_FILENAME "fpos"

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
  // Whatever line number is set here is searched for below and its bounds will
  // be saved in buf->cursor_line.
  line_col_t saved_file_position;
  if (lookup_file_position(filename, &saved_file_position)) {
    logt("Using saved file position: %d, %d", saved_file_position.line,
        saved_file_position.col);
    buf->cursor_line.num = saved_file_position.line;
    buf->cursor_col = saved_file_position.col;
  } else {
    logt("No saved file position.");
    buf->cursor_line.num = 0;
    buf->cursor_col = 0;
  }
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
  // Make sure the cursor line/col are in bounds
  if (buf->cursor_line.num >= buf->n_lines) {
    // If saved cursor line doesn't exist, put the cursor at the start of the
    // buffer.
    buf->cursor_line.num = 0;
    buf->cursor_line.off = 0;
    for (buf->cursor_line.len = 0;
        '\n' != buf->text.c[buf->cursor_line.len];
        buf->cursor_line.len++) {}
    buf->cursor_col = 0;
  } else if (buf->cursor_col >= buf->cursor_line.len) {
    // If the saved cursor column doesn't exist, put the cursor at the end of
    // the line.
    buf->cursor_col = buf->cursor_line.len;
  }
  // Set the gap to the cursor position
  TRE_Buf_move_gap(buf, buf->cursor_line.off + buf->cursor_col);
  // Assume ASCII as the default (only) encoding for now.
  buf->encoding = TRE_BUF_ENCODING_ASCII;
  logt("File loaded: %s", filename);
  return buf;
}

LOCAL TRE_OpResult lookup_file_position(const char* filename,
    line_col_t* pos) {
  char* edited_file_path = realpath(filename, NULL);
  char* fpos_file_path = TRE_find_config_file(TRE_SAVED_POSITIONS_FILENAME);
  TRE_OpResult result = TRE_FAIL;
  if (NULL == edited_file_path) {
    log_err("Unable to resolve path to file '%s': %s", filename,
        strerror(errno));
  } else {
    if (fpos_file_path != NULL) {
      gchar* fpos_file_contents;
      GError *error;
      if (!g_file_get_contents(fpos_file_path, &fpos_file_contents, NULL,
            &error)) {
        log_err("Error opening fpos file '%s': %s", fpos_file_path, error);
      } else {
        if (lookup_fpos_in_cont(edited_file_path, fpos_file_contents, pos)) {
          result = TRE_SUCC;
        }
        g_free(fpos_file_contents);
      }
      g_free(fpos_file_path);
    }
    free(edited_file_path);
  }
  return result;
}

LOCAL TRE_OpResult lookup_fpos_in_cont(const char* filename,
    gchar* fpos_file_contents, line_col_t* file_pos) {
  logt("Searching for saved position for file: '%s'", filename);
  TRE_Line line;
  line.num = -1;
  line.off = -1;
  line.len = 0;
  // Get the first line in the file. It's a file type identifier.
  if (!next_line_in_buffer(fpos_file_contents, &line)) {
    log_err("The fpos file is empty.");
    return TRE_FAIL;
  }
  if (0 != strcmp(fpos_file_contents + line.off, "TRE_FPOS")) {
    log_err("The fpos file lacks the expected header line.");
    return TRE_FAIL;
  }
  // Read the file line-by-line. Any parsing failure means the file isn't
  // valid; we won't attempt to parse it any further.
  logt("Reading fpos file.");
  while (next_line_in_buffer(fpos_file_contents, &line)) {
    char* read_pos = fpos_file_contents + line.off;
    char* tail;
    long f_line = strtol(read_pos, &tail, 10);
    if (f_line == 0 && tail == read_pos) {
      return TRE_FAIL;
    }
    read_pos = tail;
    long f_col = strtol(read_pos, &tail, 10);
    if (f_col == 0 && tail == read_pos) {
      return TRE_FAIL;
    }
    read_pos = tail;
    while (' ' == *read_pos) {
      read_pos++;
    }
    // The remainder of the line should be the filename.
    logt("Scanned saved file position: %d, %d, '%s'", f_line, f_col, read_pos);
    if (0 == strcmp(filename, read_pos)) {
      file_pos->line = f_line;
      file_pos->col = f_col;
      logt("Found saved position for this file: %d, %d", f_line, f_col);
      return TRE_SUCC;
    }
  }
  logt("Found no saved position for this file.");
  return TRE_FAIL;
}

LOCAL int next_line_in_buffer(gchar* b, TRE_Line* line) {
  line->num++;
  line->off += line->len + 1;
  for (int i = line->off; b[i] != '\0'; i++) {
    if (b[i] == '\n') {
      b[i] = '\0';
      line->len = i - line->off;
      return 1;
    }
  }
  line->num = -1;
  return 0;
}

// Return the path to the specified config file, if it exists. Returns NULL if
// the file doesn't exist, otherwise returns a string that must be freed
// afterward.
char* TRE_find_config_file(const char* filename) {
  const char* config_dir = TRE_DEFAULT_CONFIG_DIR;
  const char* home_dir = getenv("HOME");
  char* config_file_path =
    g_strdup_printf("%s/%s/%s", home_dir, config_dir, filename);
  logt("Looking for fpos file: %s", config_file_path);
  // Check that file exists, and is a regular file. If not, return nothing.
  GStatBuf statbuf;
  if (-1 == g_stat(config_file_path, &statbuf)) {
    logt("Unable to stat file: %s", strerror(errno));
    g_free(config_file_path);
    return NULL;
  } else if (!S_ISREG(statbuf.st_mode)) {
    logt("Not a file.");
    g_free(config_file_path);
    return NULL;
  } else {
    logt("File found.");
    return config_file_path;
  }
}

