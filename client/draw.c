
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

