
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
      TRE_RT_err_msg(rt, "Unable to read file.");
      close(fd);
      return NULL;
    }
    n_read_total += n_read;
  } while (n_read_total < (size_t)file_size);
  buf->n_lines = 0;
  buf->encoding = TRE_BUF_ENCODING_ASCII;
  return buf;
}

